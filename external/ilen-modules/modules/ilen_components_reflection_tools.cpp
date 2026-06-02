#include <iostream>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <json/json.hpp>

#include <ilen/headers/ilen_logs.h>

using json = nlohmann::json;
namespace fs = std::filesystem;

const char* DEBUG_PREFIX = "HEADER_TOOL";

struct TypeInfo {
    std::string jsonType;
    size_t size;
    size_t alignment;
};

TypeInfo GetTypeInfo(std::string cppType, const std::string& arraySizeStr) {
     auto contains = [](const std::string& str, const std::string& sub) {
        return str.find(sub) != std::string::npos;
    };
    
    //Tipos dinamicos, como vector o string (se guarda en punteros, y no en la memoria del struct.)
    if(contains(cppType, "vector<")) return {"vector<uint32_t>", 24, 8};
    if(contains(cppType, "string")) return {"string", 32, 8};
    
    //Funciones de la biblio de mates
    if(contains(cppType, "vec2")) return {"float2", 8, 16};
    if(contains(cppType, "vec3")) return {"float3", 12, 16};
    if(contains(cppType, "vec4")) return {"float4", 16, 16};
    if(contains(cppType, "mat4")) return {"float4x4", 64, 16};

    //Primitives
    if(cppType == "bool") return {"bool", 1, 1};
    if(cppType == "float") return {"float", 4, 4};
    if(cppType == "int32_t" || cppType == "int") return {"int", 4, 4};
    if(cppType == "Entity" || cppType == "uint32_t" ||
        contains(cppType, "ILEN_CAMERA_PROJECTION_TYPE") || contains(cppType, "ENUM") || 
        contains(cppType, "TYPE") || contains(cppType, "TagSystem") || 
        contains(cppType, "LayerSystem")) return {"uint32_t", 4, 4};
    if(cppType == "UUID") return {"uint64_t", 8, 8};

    //Arreglos fijos
    if(cppType == "char" && !arraySizeStr.empty()) {
        size_t size = std::stoull(arraySizeStr);
        return {"char[" + arraySizeStr + "]", size, 1};
    }
    if(cppType == "uint8_t" && !arraySizeStr.empty()){
        size_t size = std::stoull(arraySizeStr);
        return {"byte[" + arraySizeStr + "]", size, 1};
    }
    if(cppType == "float" && arraySizeStr == "3") return {"float3", 12, 4};

    return {"unknown", 0, 1};
};

json ScanDirectoryForComponents(const fs::path& searchDir) {
    json schemaJson = json::object();

    if(!fs::exists(searchDir)) {
        ILEN_ERROR_P(DEBUG_PREFIX, "Error: El directorio no existe -> %s", searchDir.c_str());
        return schemaJson;
    }

    std::regex componentRegex(R"(ILEN_COMPONENT\s+(?:struct|class)\s+(?:alignas\((\d+)\)\s+)?(\w+))");
    std::regex propertyRegex(R"(ILEN_PROPERTY\s+([\w:<>\[\]*&]+)\s+(\w+)(?:\[(\d+)\])?[^;]*;)");
        //Iterar para encontrar los componentes
    for(const auto& entry : fs::recursive_directory_iterator(searchDir)) {
        if(entry.is_regular_file() && (entry.path().extension() == ".h" || entry.path().extension() == ".hpp")) {
            std::ifstream file(entry.path());
            if(!file.is_open()) continue;

            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

            auto compBegin = std::sregex_iterator(content.begin(), content.end(), componentRegex);
            auto compEnd = std::sregex_iterator();

            for(std::sregex_iterator i = compBegin; i != compEnd; i++) {
                std::string alignasStr = (*i)[1].str();
                std::string compName = (*i)[2].str();
                
                size_t structMaxAlignment = 1;
                if(!alignasStr.empty()){
                    structMaxAlignment = std::stoull(alignasStr);
                }
                
                json fieldsArray = json::array();
                size_t currentOffset = 0;

                size_t startPos = i->position();
                // Buscamos el inicio del próximo componente para delimitar el bloque.
                // Si no hay otro componente en el archivo, tomamos el resto del archivo entero.

                //OJO: Las propiedades se definen por el ILEN_PROPERTY. Si esto se coloca en una variable FUERA del ILEN_COMPONET, se asignaría al actual ILEN_COMPONENT, lo cual es problematico.
                size_t endPos = content.find("ILEN_COMPONENT", startPos + 10); 
                if(endPos == std::string::npos) {
                    endPos = content.length();
                }

                std::string structBody = content.substr(startPos, endPos - startPos);

                auto propBegin = std::sregex_iterator(structBody.begin(), structBody.end(), propertyRegex);
                auto propEnd = std::sregex_iterator();

                for(std::sregex_iterator j = propBegin; j != propEnd; j++) {
                    std::string type = (*j)[1].str();
                    std::string name = (*j)[2].str();
                    std::string arraySize = (*j)[3].str();

                    //Limpieza para dejar el tipo limpio
                    type.erase(0, type.find_first_not_of(" \t"));
                    type.erase(type.find_last_not_of(" \t") + 1);

                    // Descartamos punteros crudos y memoria de GPU
                    if(type.find("shared_ptr") != std::string::npos || type.find("*") != std::string::npos){
                        continue;
                    }

                    TypeInfo info = GetTypeInfo(type, arraySize);

                    if(info.jsonType == "unknown") continue;

                    size_t padding = (info.alignment - (currentOffset % info.alignment));
                    currentOffset += padding;

                    json field;
                    field["name"] = name;
                    field["type"] = info.jsonType;
                    field["offset"] = currentOffset;
                    field["size"] = info.size;

                    fieldsArray.push_back(field);

                    currentOffset += info.size;

                    if(alignasStr.empty()){
                        structMaxAlignment = std::max(structMaxAlignment, info.alignment);
                    }
                }

                if(!fieldsArray.empty()) {
                    size_t finalPadding = (structMaxAlignment - (currentOffset % structMaxAlignment)) % structMaxAlignment;
                    currentOffset += finalPadding;

                    json compNode;
                    compNode["size"] = currentOffset;
                    compNode["fields"] = fieldsArray;

                    schemaJson[compName] = compNode;
                }
            }
        }
    }

    return schemaJson;
}

int main(int argc, char* argv[]) {
    std::vector<std::string_view> args(argv, argv + argc);

    fs::path basePath;
    bool isEngineMode = false;

    for(size_t i = 1; i < args.size(); i++){
        if((args[i] == "--ilen-folder" || args[i] == "-f") && i + 1 < args.size()) {
            basePath = args[i + 1];
        } else if(args[i] == "--engine-mode") {
            isEngineMode = true;
        }
    }

    if(basePath.empty()){
        ILEN_ERROR_P(DEBUG_PREFIX, "Error: Se requiere la ruta base. Usa --ilen-folder <ruta> ");
        return -1;
    }

    if(isEngineMode) {
        ILEN_LOG_P(DEBUG_PREFIX, "Indexando los componentes base del motor");

        fs::path engineIncludeDir = basePath / "include" / "ilen";
        ILEN_LOG_P(DEBUG_PREFIX, "Escaneando la carpeta %s", engineIncludeDir.c_str());
        json engineSchema = ScanDirectoryForComponents(engineIncludeDir);

        fs::path outputDir = basePath / "project_data" / "manifests";
        if(!fs::exists(outputDir)) fs::create_directories(outputDir);

        fs::path outputPath = outputDir / "engine_defaults.meta_ilenschema";
        std::ofstream outFile(outputPath);
        outFile << engineSchema.dump(4);

        ILEN_LOG_P(DEBUG_PREFIX, "Esquema del motor completo. Indexados %i componentes", engineSchema.size());
    } else{
        ILEN_LOG_P(DEBUG_PREFIX, "Iniciando indexacion de componentes del USUARIO");        
        fs::path userSrcDir = basePath / "project_data" / "src";
        json finalSchema = json::object();

        // A. Intentar cargar el esquema predefinido del motor primero
        fs::path defaultSchemaPath = basePath / "project_data" / "manifests" / "engine_defaults.meta_ilenschema";
        if (fs::exists(defaultSchemaPath)) {
            std::ifstream defaultFile(defaultSchemaPath);
            try {
                defaultFile >> finalSchema;
                ILEN_LOG_P(DEBUG_PREFIX, "Esquema base del motor cargado exitosamente.");
            } catch (...) {
                ILEN_WARNING_P(DEBUG_PREFIX, "Advertencia: El esquema default del motor esta corrupto.");
            }
        } else {
            ILEN_ERROR_P(DEBUG_PREFIX, "Advertencia: No se encontro el esquema base del motor en %s", defaultSchemaPath.c_str());
        }

        // B. Escanear componentes del usuario
        json userSchema = ScanDirectoryForComponents(userSrcDir);
        ILEN_LOG_P(DEBUG_PREFIX, "Componentes de usuario encontrados: %s", userSchema.size());
        // C. Fusionar esquemas (El usuario sobreescribe/añade al motor)
        finalSchema.update(userSchema);

        // D. Guardar el meta_ilenschemas final listo para el Scene Binarizer
        fs::path outputDir = basePath / "project_data" / "manifests";
        if (!fs::exists(outputDir)) fs::create_directories(outputDir);

        fs::path outputPath = outputDir / "components.meta_ilenschema";
        std::ofstream outFile(outputPath);
        outFile << finalSchema.dump(4);

        ILEN_LOG_P(DEBUG_PREFIX, "Esquema final generado en: %s", outputPath.c_str());
    }

    return 0;
}