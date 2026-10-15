#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <json/json.hpp>

#include <lynx/headers/lynx_logs.h>
#include <lynx/resources/scene_manager.h>

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
    if(contains(cppType, "vector<")) {
        if(contains(cppType, "vec2") || contains(cppType, "Vector2") || contains(cppType, "float2")) {
            return {"vector<float2>", 24, 8};
        }
        if(contains(cppType, "uint64_t") || contains(cppType, "UUID")) {
            return {"vector<uint64_t>", 24, 8};
        }

        if(contains(cppType, "uint32_t") ||
            contains(cppType, "uint32") || 
            contains(cppType, "Entity")
        ){
            return {"vector<uint32_t>", 24, 8};
        }

        return {"unknown", 0, 1};
    } 
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
        contains(cppType, "LYNX_CAMERA_PROJECTION_TYPE") || contains(cppType, "ENUM") || 
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
        LYNX_ERROR_P(DEBUG_PREFIX, "Error: El directorio no existe -> %s", searchDir.c_str());
        return schemaJson;
    }

   // REGEX MEJORADO
    std::regex componentRegex(R"(LYNX_COMPONENT\s+(?:struct|class)\s+(?:alignas\((\d+)\)\s+)?(\w+))");
    std::regex propertyRegex(R"(LYNX_PROPERTY\s+([\w:<>\[\]*&]+)\s+(\w+)(?:\[(\d+)\])?[^;]*;)");
    
    // NUEVO: Regex para atrapar namespaces (ej. namespace lynx::ecs::physics { )
    std::regex namespaceRegex(R"(namespace\s+([a-zA-Z0-9_:]+)\s*\{)");

    for(const auto& entry : fs::recursive_directory_iterator(searchDir)) {
        if(entry.is_regular_file() && (entry.path().extension() == ".h" || entry.path().extension() == ".hpp")) {
            std::ifstream file(entry.path());
            if(!file.is_open()) continue;

            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

            // --- NUEVO: Mapeo de namespaces en este archivo ---
            std::map<size_t, std::string> activeNamespaces;
            auto nsBegin = std::sregex_iterator(content.begin(), content.end(), namespaceRegex);
            auto nsEnd = std::sregex_iterator();
            for (std::sregex_iterator k = nsBegin; k != nsEnd; k++) {
                activeNamespaces[k->position()] = (*k)[1].str();
            }
            // -------------------------------------------------

            auto compBegin = std::sregex_iterator(content.begin(), content.end(), componentRegex);
            auto compEnd = std::sregex_iterator();

            for(std::sregex_iterator i = compBegin; i != compEnd; i++) {
                
                std::string alignasStr = (*i)[1].str();
                std::string compName = (*i)[2].str();
                size_t startPos = i->position();

                // --- NUEVO: Encontrar a qué namespace pertenece este componente ---
                std::string currentNamespace = "";
                // Iteramos en reversa para encontrar el último namespace declarado ANTES del componente
                for (auto it = activeNamespaces.rbegin(); it != activeNamespaces.rend(); ++it) {
                    if (it->first < startPos) {
                        currentNamespace = it->second;
                        break;
                    }
                }
                
                size_t structMaxAlignment = 1;
                if(!alignasStr.empty()){
                    structMaxAlignment = std::stoull(alignasStr);
                }
                
                json fieldsArray = json::array();
                size_t currentOffset = 0;

                //size_t startPos = i->position();
                // Buscamos el inicio del próximo componente para delimitar el bloque.
                // Si no hay otro componente en el archivo, tomamos el resto del archivo entero.

                //OJO: Las propiedades se definen por el LYNX_PROPERTY. Si esto se coloca en una variable FUERA del LYNX_COMPONET, se asignaría al actual LYNX_COMPONENT, lo cual es problematico.
                size_t endPos = content.find("LYNX_COMPONENT", startPos + 10); 
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
                    compNode["namespace"] = currentNamespace;
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
        if((args[i] == "--lynx-folder" || args[i] == "-f") && i + 1 < args.size()) {
            basePath = args[i + 1];
        } else if(args[i] == "--engine-mode") {
            isEngineMode = true;
        }
    }

    if(basePath.empty()){
        LYNX_ERROR_P(DEBUG_PREFIX, "Error: Se requiere la ruta base. Usa --lynx-folder <ruta> ");
        return -1;
    }

    if(isEngineMode) {
        LYNX_LOG_P(DEBUG_PREFIX, "Indexando los componentes base del motor");

        fs::path engineIncludeDir = basePath / "include" / "lynx";
        LYNX_LOG_P(DEBUG_PREFIX, "Escaneando la carpeta %s", engineIncludeDir.c_str());
        json engineSchema = ScanDirectoryForComponents(engineIncludeDir);

        fs::path outputDir = basePath / "project_data" / "manifests";
        if(!fs::exists(outputDir)) fs::create_directories(outputDir);

        fs::path outputPath = outputDir / "engine_defaults.meta_lynxschema";
        std::ofstream outFile(outputPath);
        outFile << engineSchema.dump(4);

        LYNX_LOG_P(DEBUG_PREFIX, "Esquema del motor completo. Indexados %i componentes", engineSchema.size());
    } else{
        LYNX_LOG_P(DEBUG_PREFIX, "Iniciando indexacion de componentes del USUARIO");        
        fs::path userSrcDir = basePath / "project_data" / "src";
        json finalSchema = json::object();

        // A. Intentar cargar el esquema predefinido del motor primero
        fs::path defaultSchemaPath = basePath / "project_data" / "manifests" / "engine_defaults.meta_lynxschema";
        if (fs::exists(defaultSchemaPath)) {
            std::ifstream defaultFile(defaultSchemaPath);
            try {
                defaultFile >> finalSchema;
                LYNX_LOG_P(DEBUG_PREFIX, "Esquema base del motor cargado exitosamente.");
            } catch (...) {
                LYNX_WARNING_P(DEBUG_PREFIX, "Advertencia: El esquema default del motor esta corrupto.");
            }
        } else {
            LYNX_ERROR_P(DEBUG_PREFIX, "Advertencia: No se encontro el esquema base del motor en %s", defaultSchemaPath.c_str());
        }

        // B. Escanear componentes del usuario
        json userSchema = ScanDirectoryForComponents(userSrcDir);
        LYNX_LOG_P(DEBUG_PREFIX, "Componentes de usuario encontrados: %s", userSchema.size());
        // C. Fusionar esquemas (El usuario sobreescribe/añade al motor)
        finalSchema.update(userSchema);

        // D. Guardar el meta_lynxschemas final listo para el Scene Binarizer
        fs::path outputDir = basePath / "project_data" / "manifests";
        if (!fs::exists(outputDir)) fs::create_directories(outputDir);

        fs::path outputPath = outputDir / "components.meta_lynxschema";
        std::ofstream outFile(outputPath);
        outFile << finalSchema.dump(4);

        LYNX_LOG_P(DEBUG_PREFIX, "Esquema final generado en: %s", outputPath.c_str());
    
        // =========================================================================
        // FASE 3: GENERACIÓN AUTOMÁTICA DE CÓDIGO C++ (NUEVO)
        // =========================================================================
        LYNX_LOG_P(DEBUG_PREFIX, "Generando C++ Bindings automaticos...");

        std::string cppCode = "#pragma once\n";
        cppCode += "// ======================================================================\n";
        cppCode += "// ARCHIVO AUTOGENERADO POR Lynx REcomposer Engine. NO EDITAR MANUALMENTE.\n";
        cppCode += "// ======================================================================\n\n";
        
        cppCode += "#include <cstring>\n";
        cppCode += "#include <unordered_map>\n";
        cppCode += "#include <utility>\n\n";
        cppCode += "#include <lynx/headers/ecs_components.h>\n";
        cppCode += "#include <lynx/core/engine_context.h>\n";
        cppCode += "#include <lynx/resources/scene_manager.h>\n\n";

        cppCode += "namespace lynx::resources::scene {\n";
        cppCode += "    inline void RegisterGeneratedDeserializers(SceneManager& manager) {\n";

        for (const auto& [compName, compNode] : finalSchema.items()) {
            uint32_t hash = lynx::resources::scene::HashComponent(compName);
            size_t staticSize = compNode["size"];

            cppCode += "        manager.RegisterDeserializer(" + std::to_string(hash) + "u,\n";
            cppCode += "            [](lynx::ecs::Entity entity, char*& stream, const std::unordered_map<UUID, lynx::ecs::Entity>& uuidMap, lynx::EngineContext* ctx) {\n";
            
            std::string ns = compNode.value("namespace", "");
            if (!ns.empty()) {
                cppCode += "                " + ns + "::" + compName + " comp;\n";
            } else {
                cppCode += "                " + compName + " comp;\n";
            }

            cppCode += "                uint64_t presenceMask = *reinterpret_cast<uint64_t*>(stream);\n";
            cppCode += "                stream += sizeof(uint64_t);\n";
            cppCode += "                char* staticStart = stream;\n\n";

            cppCode += "                int fieldIdx = 0;\n";

            // ===================================================
            // 1. BLOQUE ESTÁTICO (POD y UUIDs Base)
            // ===================================================
            for (const auto& field : compNode["fields"]) {
                std::string name = field["name"];
                std::string type = field["type"];
                size_t offset = field["offset"];
                size_t size = field["size"];

                bool isDynamic = (type == "string" || type.find("vector") != std::string::npos);
                bool isEntity = (name == "parent" || name == "toFollowEntity" || name == "toLookAtEntity" || name == "cameraEntity");

                if (!isDynamic) {
                    // Solo copiamos si el bit correspondiente en la máscara está activo
                    cppCode += "                if (presenceMask & (1ULL << fieldIdx)) {\n";
                    cppCode += "                    std::memcpy(&comp." + name + ", staticStart + " + std::to_string(offset) + ", " + std::to_string(size) + ");\n";
                    
                    // Parcheo de UUID integrado en la copia segura
                    if (isEntity) {
                        cppCode += "                    if (uuidMap.contains(comp." + name + ")) comp." + name + " = uuidMap.at(comp." + name + ");\n";
                        cppCode += "                    else comp." + name + " = lynx::ecs::NULL_ENTITY;\n";
                    }
                    cppCode += "                }\n";
                }
                cppCode += "                fieldIdx++;\n";
            }

            cppCode += "                stream += " + std::to_string(staticSize) + "; // Avanzar bloque estatico\n\n";

            // ===================================================
            // 2. BLOQUE DINÁMICO (Vectores y Strings)
            // ===================================================
            cppCode += "                fieldIdx = 0;\n";
            for (const auto& field : compNode["fields"]) {
                std::string name = field["name"];
                std::string type = field["type"];

                if (type == "string") {
                    cppCode += "                uint32_t " + name + "_len = *reinterpret_cast<uint32_t*>(stream);\n";
                    cppCode += "                stream += sizeof(uint32_t);\n";
                    cppCode += "                if (presenceMask & (1ULL << fieldIdx)) {\n";
                    cppCode += "                    comp." + name + " = std::string(stream, " + name + "_len);\n";
                    cppCode += "                }\n";
                    cppCode += "                stream += " + name + "_len;\n";
                } 
                // FIX: Soporte dinámico para diferentes vectores para no desfasar el puntero
                else if (type.find("vector") != std::string::npos) {
                    cppCode += "                uint32_t " + name + "_count = *reinterpret_cast<uint32_t*>(stream);\n";
                    cppCode += "                stream += sizeof(uint32_t);\n";
                    cppCode += "                if (presenceMask & (1ULL << fieldIdx)) {\n";
                    cppCode += "                    comp." + name + ".reserve(" + name + "_count);\n";
                    cppCode += "                    for (uint32_t i = 0; i < " + name + "_count; ++i) {\n";
                    
                    if (type == "vector<uint64_t>" || type == "vector<UUID>") {
                        cppCode += "                        UUID id = *reinterpret_cast<UUID*>(stream);\n";
                        cppCode += "                        if (uuidMap.contains(id)) comp." + name + ".push_back(uuidMap.at(id));\n";
                        cppCode += "                        stream += sizeof(UUID);\n";
                    } else if (type == "vector<uint32_t>" || type == "vector<uint32>") {
                        cppCode += "                        uint32_t val = *reinterpret_cast<uint32_t*>(stream);\n";
                        cppCode += "                        comp." + name + ".push_back(val);\n";
                        cppCode += "                        stream += sizeof(uint32_t);\n";
                    }else if (type == "vector<float2>") {
                        cppCode += "                        float x = 0.0f;\n";
                        cppCode += "                        float y = 0.0f;\n";
                        cppCode += "                        std::memcpy(&x, stream, sizeof(float));\n";
                        cppCode += "                        stream += sizeof(float);\n";
                        cppCode += "                        std::memcpy(&y, stream, sizeof(float));\n";
                        cppCode += "                        stream += sizeof(float);\n";
                        cppCode += "                        comp." + name + ".emplace_back(x, y);\n";
                    }
                    cppCode += "                    }\n";
                    cppCode += "                } else {\n";
                    
                    // Si el componente usa valores por defecto, DEBEMOS avanzar el stream para no corromper la memoria
                    if (type == "vector<uint64_t>" || type == "vector<UUID>") {
                        cppCode += "                    stream += (" + name + "_count * sizeof(UUID));\n";
                    }
                    else if (type == "vector<uint32_t>" || type == "vector<uint32>") {
                        cppCode += "                    stream += (" + name + "_count * sizeof(uint32_t));\n";
                    }
                    else if (type == "vector<float2>") {
                        cppCode += "                    stream += (" + name + "_count * sizeof(float) * 2);\n";
                    }
                    cppCode += "                }\n";
                }
                cppCode += "                fieldIdx++;\n";
            }

            // Se usa addComponent para actualizar la firma (bitmask) del ECS
            cppCode += "                ctx->p_coordinator->setComponent(entity, std::move(comp));\n";
            cppCode += "            }\n";
            cppCode += "        );\n\n";
        }

        cppCode += "    }\n}\n";

        fs::path cppOutPath = outputDir / "generated_serializers.inl";
        std::ofstream outCppFile(cppOutPath);
        outCppFile << cppCode;
        outCppFile.close();

        LYNX_LOG_P(DEBUG_PREFIX, "Codigo C++ autogenerado en: %s", cppOutPath.string().c_str());
    }

    return 0;
}