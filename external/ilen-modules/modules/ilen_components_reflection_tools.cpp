#include <iostream>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <json/json.hpp>

#include <ilen/headers/ilen-logs.h>

using json = nlohmann::json;
namespace fs = std::filesystem;

const char* DEBUG_PREFIX = "HEADER_TOOL";

json ScanDirectoryForComponents(const fs::path& searchDir) {
    json schemaJson;

    if(!fs::exists(searchDir)) {
        ILEN_ERROR_P(DEBUG_PREFIX, "Error: El directorio no existe -> %s", searchDir.c_str());
        return schemaJson;
    }

    std::regex componentRegex(R"(ILEN_COMPONENT\s+struct\s+(?:alignas\([^)]+\)\s+)?(\w+))");
    std::regex propertyReges(R"(ILEN_PROPERTY\s+([\w:<>\[\]]+)\s+(\w+)(?:\[(\d+)\])?s*;)");

    //Iterar para encontrar los componentes
    for(const auto& entry : fs::recursive_directory_iterator(searchDir)) {
        if(entry.is_regular_file() && (entry.path().extension() == ".h" || entry.path().extension() == ".hpp")) {
            std::ifstream file(entry.path());
            if(!file.is_open()) continue;

            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

            auto compBegin = std::sregex_iterator(content.begin(), content.end(), componentRegex);
            auto compEnd = std::sregex_iterator();

            for(std::sregex_iterator i = compBegin; i != compEnd; i++) {
                std::string compName = (*i)[1].str();
                json fieldsArray = json::array();

                size_t startPos = i->position();
                size_t endPos = content.find("};", startPos);

                if(endPos == std::string::npos) continue;

                std::string structBody = content.substr(startPos, endPos - startPos);

                auto propBegin = std::sregex_iterator(structBody.begin(), structBody.end(), propertyReges);
                auto propEnd = std::sregex_iterator();

                for(std::sregex_iterator j = propBegin; j != propEnd; j++) {
                    std::string type = (*j)[1].str();
                    std::string name = (*j)[2].str();
                    std::string arraySize = (*j)[3].str();

                    //Limpieza para dejar el tipo limpio
                    type.erase(0, type.find_first_not_of(" \t"));
                    type.erase(type.find_last_not_of(" \t") + 1);

                    auto contains = [](const std::string& str, const std::string& sub) {
                        return str.find(sub) != std::string::npos;
                    };

                    json field;
                    field["name"] = name;

                    //Parseo de datos propios a una version friendly al binarizador
                    if(contains(type, "shared_ptr") || contains(type, "*")){
                        //Punteros no se deben binarizar
                        continue;
                    }
                    else if(type == "bool") field["type"] = "bool";
                    else if(contains(type, "vec2")) field["type"] = "float2";
                    else if(contains(type, "vec3")) field["type"] = "float3";
                    else if(contains(type, "vec4")) field["type"] = "float4";
                    else if(contains(type, "mat4")) field["type"] = "float4x4"; //matriz 4x4
                    else if(type == "Entity") field["type"] = "uint32_t";
                    else if(contains(type, "ILEN_CAMERA_PROJECTION_TYPE") || contains(type, "ENUM") || contains(type, "TYPE"))
                        field["type"] = "uint32_t";
                    else if(type == "float" && arraySize == "3") field["type"] = "float3";
                    else if(type == "float") field["type"] = "float";
                    else if(type == "int32_t") field["type"] = "int";
                    else if(type == "uint32_t") field["type"] = "uint32_t";
                    else if(type == "char" && !arraySize.empty()) field["type"] = "char[" + arraySize + "]";
                    else if (type == "uint8_t" && !arraySize.empty()) field["type"] = "byte[" + arraySize + "]";
                    else field["type"] = "unknown";

                    fieldsArray.push_back(field);
                }

                if(!fieldsArray.empty()) {
                    schemaJson[compName] = fieldsArray;
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
        fs::path defaultSchemaPath = basePath.parent_path() / "project_data" / "manifests" / "engine_defaults.meta_ilenschema";
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