//Este módulo tendrá como fin binarizar los archivos de escenas del motor. Estos se encontraran en la carpeta del proyecto, y luego
//en project_data/scenes.

#include <iostream>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <string>
#include <mutex>
#include <ranges>
#include <algorithm>
#include <json/json.hpp>

#include <ilen/headers/ilen_logs.h>
#include <ilen/core/time.h>
#include <ilen/job_system/job_system.h>
#include <ilen/ecs/types.h>
//#include <ilen/headers/ecs-components.h>
#include <ilen/resources/binary_headers.h>


namespace ilen::core::threading {
    std::thread::id g_MainThreadId;
    thread_local bool t_IsMainThread = false;
    thread_local size_t t_ThreadIndex = UNREGISTERED_THREAD;
}

using json = nlohmann::json;
using namespace ilen::binary::headers;
namespace fs = std::filesystem;


const char* DEBUG_PREFIX = "SCENE_BINARIZATION";

const std::string SCENE_MANIFEST_TOTAL = "totalOfScenes";
const std::string SCENE_MANIFEST_LIST = "scenes";
const std::string SCENE_MANIFEST_ID = "id";
const std::string SCENE_MANIFEST_NAME = "name";
const std::string SCENE_MANIFEST_PATH = "path";

constexpr uint32_t HashComponent(std::string_view str) {
    uint32_t hash = 2166136261u;
    for(char c : str) {
        hash ^= static_cast<uint32_t>(c);
        hash *= 16777619u;
    }
    return hash;
}

ilen::job_system::JobSystem jobSystem;

void binarizeScene(std::filesystem::path projectDataFolder);

enum class SchemaType: uint8_t {
    Bool = 0,
    Float = 1,
    Float2 = 2,
    Float3 = 3,
    Float4 = 4,
    Float4x4 = 5,
    Uint32 = 6,
    Int32 = 7,
    String = 8,
    VectorUint32 = 9,
    Uint64 = 10,
    VectorUint64 = 11,
    Unknown = 255
};

struct SchemaField {
    std::string name;
    SchemaType type;
    size_t offset;
    size_t size;
};

struct ComponentSchema {
    std::string name;
    uint32_t hash;
    size_t totalSizeBytes;
    std::vector<SchemaField> fields;
};

class SchemaManager {
    private:
        std::unordered_map<uint32_t, ComponentSchema> registry;

        SchemaType ParseTypeString(const std::string& typeStr) {
            if(typeStr == "bool") return SchemaType::Bool;
            if(typeStr == "float") return SchemaType::Float;
            if(typeStr == "float2") return SchemaType::Float2;
            if(typeStr == "float3") return SchemaType::Float3;
            if(typeStr == "float4") return SchemaType::Float4;
            if(typeStr == "float4x4") return SchemaType::Float4x4;
            if (typeStr == "uint32_t" || typeStr == "uint32") return SchemaType::Uint32;
            if (typeStr == "int32_t" || typeStr == "int") return SchemaType::Int32;
            if (typeStr == "uint64_t") return SchemaType::Uint64;
            if (typeStr == "string") return SchemaType::String;
            if (typeStr == "vector<uint32_t>" || typeStr == "vector<uint32>") return SchemaType::VectorUint32;
            if (typeStr == "vector<uint64_t>" || typeStr == "vector<uint64>") return SchemaType::VectorUint64;
            return SchemaType::Unknown;
        }

    public:
        bool LoadSchema(const fs::path& schemaPath) {
            std::ifstream file(schemaPath);
            if(!file.is_open()) return false;

            json schemaJson;
            try { file >> schemaJson; }
            catch(...) {return false; }

            for(auto& [compName, compData]: schemaJson.items()) {
                ComponentSchema schema;
                schema.name = compName;
                schema.hash = HashComponent(compName);
                schema.totalSizeBytes = compData.value("size", 0);

                for(const auto& fieldJson : compData["fields"]) {
                    SchemaField field;
                    field.name = fieldJson["name"];
                    field.offset = fieldJson["offset"];
                    field.size = fieldJson["size"];
                    field.type = ParseTypeString(fieldJson["type"]);

                    if(field.type != SchemaType::Unknown) {
                        schema.fields.push_back(field);
                    }
                }
                registry[schema.hash] = schema;
            }
            return true;
        }

        bool PackComponent(uint32_t hash, const json& compJsonData, std::vector<char>& outBuffer){
            auto it = registry.find(hash);
            if(it == registry.end()) return false;

            const ComponentSchema& schema = it->second;

            //Buffer para los datos planos.
            std::vector<char> staticBytes(schema.totalSizeBytes, 0);

            //Buffer de datos dinámicos
            std::vector<char> dynamicPayload;

            for(const auto& field : schema.fields) {
                if(!compJsonData.contains(field.name)) continue;

                const auto& jVal = compJsonData[field.name];
                char* destPtr = staticBytes.data() + field.offset;
                
                //Tipos estáticos
                if(field.type == SchemaType::Bool) {
                    bool val = jVal.get<bool>();
                    std::memcpy(destPtr, &val, sizeof(bool));
                }
                else if(field.type == SchemaType::Float) {
                    float val = jVal.get<float>();
                    std::memcpy(destPtr, &val, sizeof(float));
                }
                else if (field.type == SchemaType::Float2 && jVal.is_array() && jVal.size() >= 2) {
                    float val[2] = { jVal[0], jVal[1] };
                    std::memcpy(destPtr, val, sizeof(val));
                }
                else if (field.type == SchemaType::Float3 && jVal.is_array() && jVal.size() >= 3) {
                    float val[3] = { jVal[0], jVal[1], jVal[2] };
                    std::memcpy(destPtr, val, sizeof(val));
                }
                else if (field.type == SchemaType::Float4x4 && jVal.is_array() && jVal.size() >= 16) {
                    float val[16];
                    for (int i = 0; i < 16; ++i) val[i] = jVal[i];
                    std::memcpy(destPtr, val, sizeof(val));
                }
                else if (field.type == SchemaType::Uint32) {
                    uint32_t val = jVal.get<uint32_t>();
                    std::memcpy(destPtr, &val, sizeof(uint32_t));
                }
                else if (field.type == SchemaType::Uint64) {
                    uint64_t val = jVal.get<uint64_t>();
                    std::memcpy(destPtr, &val, sizeof(uint64_t));
                }
                // --- TIPOS DINÁMICOS (Se dejan en 0 en el staticByte y se añaden al Payload) ---
                else if (field.type == SchemaType::String) {
                    std::string str = jVal.get<std::string>();
                    uint32_t len = str.length();
                    // Escribir Tamaño + Caracteres en la cola dinámica
                    dynamicPayload.insert(dynamicPayload.end(), reinterpret_cast<char*>(&len), reinterpret_cast<char*>(&len) + sizeof(uint32_t));
                    dynamicPayload.insert(dynamicPayload.end(), str.begin(), str.end());
                }
                else if (field.type == SchemaType::VectorUint32 && jVal.is_array()) {
                    uint32_t count = jVal.size();
                    // Escribir Cantidad + Elementos en la cola dinámica
                    dynamicPayload.insert(dynamicPayload.end(), reinterpret_cast<char*>(&count), reinterpret_cast<char*>(&count) + sizeof(uint32_t));
                    for (const auto& item : jVal) {
                        uint32_t val = item.get<uint32_t>();
                        dynamicPayload.insert(dynamicPayload.end(), reinterpret_cast<char*>(&val), reinterpret_cast<char*>(&val) + sizeof(uint32_t));
                    }
                }
                else if (field.type == SchemaType::VectorUint64 && jVal.is_array()) {
                    uint32_t count = jVal.size();
                    dynamicPayload.insert(dynamicPayload.end(), reinterpret_cast<char*>(&count), reinterpret_cast<char*>(&count) + sizeof(uint32_t));
                    for (const auto& item : jVal) {
                        uint64_t val = item.get<uint64_t>();
                        dynamicPayload.insert(dynamicPayload.end(), reinterpret_cast<char*>(&val), reinterpret_cast<char*>(&val) + sizeof(uint64_t));
                    }
                }
            }
            
            outBuffer.insert(outBuffer.end(), staticBytes.begin(), staticBytes.end());
            outBuffer.insert(outBuffer.end(), dynamicPayload.begin(), dynamicPayload.end());
            return true;
        }
};

SchemaManager g_SchemaManager;

int main(int argc, char* argv[]) {
    std::filesystem::path folderPath;

    jobSystem.Initializate();
    ilen::time::Time::Init();

    for(int i = 0; i < argc; i++){
        std::string arg = argv[i];
        if(arg == "--ilen-folder" || arg == "-f") {
            if(i + 1 < argc) {
                folderPath = argv[i + 1];
            }else {
                std::cerr << "Error: El indicador --ilen-folder requiere una ruta a una carpeta." << std::endl;
                return 1;
            }
        }
    }

    if(folderPath.empty()) {
        std::cerr << "Se requiere la ruta del motor. Añádela con el comando --ilen-folder <ruta>." << std::endl;
        return -1;
    }

    std::filesystem::path projectDataFolder = folderPath / "project_data";

    // Cargar la "Receta" universal antes de lanzar los hilos
    fs::path schemaPath = projectDataFolder / "manifests" / "components.meta_ilenschema";
    if (!g_SchemaManager.LoadSchema(schemaPath)) {
        ILEN_ERROR_P(DEBUG_PREFIX, "No se pudo cargar el schema de componentes: %s", schemaPath.string().c_str());
        return -1;
    }

    binarizeScene(projectDataFolder);

    jobSystem.StopJobSystem();
    return 0;
}
 
void binarizeScene(std::filesystem::path projectDataFolder) {
    fs::path sourceScenesFolder = projectDataFolder / "scenes";
    fs::path binaryScenesFolder = projectDataFolder.parent_path() / ".ilen-bin-data" / "scenes";
    
    fs::path manifestJsonPath = sourceScenesFolder.parent_path() / "manifests" / "scenes.meta_ilenmanifest";
    fs::path manifestBinaryPath = binaryScenesFolder.parent_path() / "manifests" / "scenes.ilenmanifest";
    //fs::path globalManifestPath = projectDataFolder.parent_path() / ".ilen-bin-data" / "manifests" / "scenes.ilenmanifest";

    if (!fs::exists(sourceScenesFolder)) {
        ILEN_ERROR_P(DEBUG_PREFIX, "No se encontro la carpeta de escenas en: %s", sourceScenesFolder.string().c_str());
        return;
    }

    ILEN_WARNING_P(DEBUG_PREFIX, "Start of scene binarization: %.4f", ilen::time::Time::getTotalTime());

    // --- FASE 1: INDEXACIÓN DE ESCENAS ---
    json manifestData;
    std::vector<std::string> existingPaths;
    uint32_t maxID = 0;

    if (fs::exists(manifestJsonPath)) {
        std::ifstream file(manifestJsonPath);
        try { file >> manifestData; } catch (...) {}
        maxID = manifestData.value(SCENE_MANIFEST_TOTAL, 0);
        if (manifestData.contains(SCENE_MANIFEST_LIST)) {
            for (const auto& sceneData : manifestData[SCENE_MANIFEST_LIST]) {
                existingPaths.push_back(sceneData.value(SCENE_MANIFEST_PATH, ""));
            }
        }
    } else {
        manifestData[SCENE_MANIFEST_TOTAL] = 0;
        manifestData[SCENE_MANIFEST_LIST] = json::array();
    }

    bool needSaveNewManifest = false;
    std::vector<fs::path> scenesToIndex;

    for (const auto& entry : fs::recursive_directory_iterator(sourceScenesFolder)) {
        if (entry.is_regular_file() && entry.path().extension() == ".meta_ilenscene") {
            fs::path relativePath = fs::relative(entry.path(), sourceScenesFolder);
            scenesToIndex.push_back(entry.path());

            if (std::find(existingPaths.begin(), existingPaths.end(), relativePath.generic_string()) == existingPaths.end()) {
                needSaveNewManifest = true;

                std::string sceneName = entry.path().stem().string();
                std::ifstream tempSceneFile(entry.path());
                if (tempSceneFile.is_open()) {
                    json tempSceneJson;
                    try { tempSceneFile >> tempSceneJson; sceneName = tempSceneJson.value("scene_name", sceneName); } 
                    catch (...) {}
                }

                json newScene;
                newScene[SCENE_MANIFEST_ID] = maxID;
                newScene[SCENE_MANIFEST_NAME] = sceneName;
                newScene[SCENE_MANIFEST_PATH] = relativePath.generic_string();
                manifestData[SCENE_MANIFEST_LIST].push_back(newScene);
                maxID++; //Esto con el fin de que las escenas empiezen en el índice 0.
            }
        }
    }

    if (needSaveNewManifest) {
        manifestData[SCENE_MANIFEST_TOTAL] = maxID;
        if (!fs::exists(manifestJsonPath.parent_path())) fs::create_directories(manifestJsonPath.parent_path());
        std::ofstream outFile(manifestJsonPath);
        outFile << manifestData.dump(4);
    }

    // --- FASE 2: COMPILACIÓN MULTIHILO (.meta_ilenscene -> .ilenscene) ---
    size_t total = scenesToIndex.size();
    if (total == 0) return;

    size_t threads = std::max<size_t>(1, jobSystem.AvailableThreads());
    size_t idealChunkSize = (total + threads - 1) / threads;
    size_t chunkSize = std::max<size_t>(10, std::min<size_t>(50, idealChunkSize));
    size_t numChunks = (total + chunkSize - 1) / chunkSize;

    ilen::job_system::JobGroup group;

    for (size_t i = 0; i < numChunks; i++) {
        size_t start = i * chunkSize;
        size_t end = std::min(start + chunkSize, total);

        jobSystem.SubmitJob([=, &scenesToIndex, &sourceScenesFolder, &binaryScenesFolder]() {
            for (size_t j = start; j < end; j++) {
                fs::path currentScenePath = scenesToIndex[j];
                fs::path relativePath = fs::relative(currentScenePath, sourceScenesFolder);
                fs::path savePath = binaryScenesFolder / relativePath;
                savePath.replace_extension(".ilenscene");

                if (fs::exists(savePath) && fs::last_write_time(currentScenePath) <= fs::last_write_time(savePath)) {
                    continue; 
                }

                std::ifstream inputFile(currentScenePath);
                if (!inputFile.is_open()) continue;

                json sceneJson;
                try { inputFile >> sceneJson; } catch (...) { continue; }

                std::unordered_map<uint32_t, SceneComponentBuffer> componentBuffers;
                std::vector<ilen::ecs::UUID> aliveEntities;
                bool jsonNeedsSave = false;

                if (sceneJson.contains("entities")) {
                    for (auto& entityJson : sceneJson["entities"]) { 
                        ilen::ecs::UUID entityId = 0;

                        // 1. Aseguramos que la entidad tenga el nodo de Metadatos
                        if (!entityJson.contains("components")) entityJson["components"] = json::object();
                        auto& componentsJson = entityJson["components"];
                        
                        if (!componentsJson.contains("EntityMetadataComponent")) {
                            componentsJson["EntityMetadataComponent"] = json::object();
                        }
                        auto& metaJson = componentsJson["EntityMetadataComponent"];

                        // 2. Extracción o Generación del UUID
                        if (metaJson.contains("uuid") && metaJson["uuid"].is_number_unsigned() && metaJson["uuid"].get<uint64_t>() > 0) {
                            // El UUID ya está correctamente asignado en los metadatos
                            entityId = metaJson["uuid"].get<uint64_t>();
                        } 
                        else if (entityJson.contains("uuid") && entityJson["uuid"].is_number_unsigned()) {
                            // MIGRACIÓN: El UUID estaba en la raíz del JSON. Lo movemos al componente.
                            entityId = entityJson["uuid"].get<uint64_t>();
                            metaJson["uuid"] = entityId;
                            entityJson.erase("uuid"); // Limpiamos el JSON
                            jsonNeedsSave = true;
                        }
                        else if (entityJson.contains("id") && entityJson["id"].is_number_unsigned()) {
                            // MIGRACIÓN: Era el formato viejo ("id": 1). Generamos un UUID real.
                            entityId = ilen::ecs::GenerateUUID();
                            metaJson["uuid"] = entityId;
                            entityJson.erase("id"); // Borramos el ID viejo
                            jsonNeedsSave = true;
                        }
                        else {
                            // Entidad completamente nueva
                            entityId = ilen::ecs::GenerateUUID();
                            metaJson["uuid"] = entityId;
                            jsonNeedsSave = true;
                        }

                        aliveEntities.push_back(entityId);

                        // 3. Empaquetado normal de los componentes
                        for (auto& [compName, compData] : componentsJson.items()) {
                            uint32_t hashId = HashComponent(compName);
                            auto& buffer = componentBuffers[hashId];
                            
                            if (g_SchemaManager.PackComponent(hashId, compData, buffer.rawData)) {
                                buffer.componentHash = hashId;
                                buffer.entityCount++;
                                buffer.entityIDs.push_back(entityId);
                            } else {
                                ILEN_WARNING_P(DEBUG_PREFIX, "Ignorado: %s en Entidad %llu (No en schema)", compName.c_str(), entityId);
                            }
                        }
                    }
                }

                // Si se generaron UUIDs o se migró el JSON, guardamos los cambios en el archivo original
                if (jsonNeedsSave) {
                    std::ofstream outSourceFile(currentScenePath);
                    outSourceFile << sceneJson.dump(4);
                    outSourceFile.close();
                }

                if (!fs::exists(savePath.parent_path())) {
                    std::error_code ec; fs::create_directories(savePath.parent_path(), ec);
                }

                std::ofstream outFile(savePath, std::ios::binary);
                ilen::binary::headers::IlenSceneHeader header{};
                strncpy(header.magicNumber, "ENECS_NELI", sizeof(header.magicNumber) - 1);
                header.version = 1;
                header.totalEntities = aliveEntities.size();
                header.totalComponentTypes = componentBuffers.size();

                outFile.write(reinterpret_cast<const char*>(&header), sizeof(ilen::binary::headers::IlenSceneHeader));
                outFile.write(reinterpret_cast<const char*>(aliveEntities.data()), header.totalEntities * sizeof(ilen::ecs::Entity));

                for (const auto& [hash, buffer] : componentBuffers) {
                    outFile.write(reinterpret_cast<const char*>(&buffer.componentHash), sizeof(uint32_t));
                    outFile.write(reinterpret_cast<const char*>(&buffer.entityCount), sizeof(uint32_t));
                    
                    uint32_t dataSizeBytes = buffer.rawData.size();
                    outFile.write(reinterpret_cast<const char*>(&dataSizeBytes), sizeof(uint32_t));
                    outFile.write(reinterpret_cast<const char*>(buffer.entityIDs.data()), buffer.entityCount * sizeof(ilen::ecs::Entity));
                    outFile.write(buffer.rawData.data(), dataSizeBytes);
                }
                outFile.close();
            }
        }, &group);
    }
    group.Wait();

    // --- FASE 3 y 4: MANIFIESTO BINARIO ---
    if (!fs::exists(manifestBinaryPath.parent_path())) fs::create_directories(manifestBinaryPath.parent_path());

    ilen::binary::headers::IlenManifestHeader manifestHeader{};
    strncpy(manifestHeader.magicNumber, "TSEFINAM_NELI", sizeof(manifestHeader.magicNumber) - 1);
    manifestHeader.version = 1;
    manifestHeader.totalOfNamesToID = manifestData[SCENE_MANIFEST_LIST].size();
    
    std::ofstream binaryManifestFile(manifestBinaryPath, std::ios::binary);
    binaryManifestFile.write(reinterpret_cast<const char*>(&manifestHeader), sizeof(ilen::binary::headers::IlenManifestHeader));

    for (const auto& sceneData : manifestData[SCENE_MANIFEST_LIST]) {
        uint32_t id = sceneData[SCENE_MANIFEST_ID];
        std::string name = sceneData[SCENE_MANIFEST_NAME];
        uint32_t nameSize = name.length();
        binaryManifestFile.write(reinterpret_cast<const char*>(&id), sizeof(uint32_t));
        binaryManifestFile.write(reinterpret_cast<const char*>(&nameSize), sizeof(uint32_t));
        binaryManifestFile.write(name.c_str(), nameSize);
    }

    for (const auto& sceneData : manifestData[SCENE_MANIFEST_LIST]) {
        uint32_t id = sceneData[SCENE_MANIFEST_ID];
        fs::path originalPath = sceneData[SCENE_MANIFEST_PATH].get<std::string>();
        std::string compiledPath = originalPath.replace_extension(".ilenscene").generic_string();
        uint32_t pathSize = compiledPath.length();
        binaryManifestFile.write(reinterpret_cast<const char*>(&id), sizeof(uint32_t));
        binaryManifestFile.write(reinterpret_cast<const char*>(&pathSize), sizeof(uint32_t));
        binaryManifestFile.write(compiledPath.c_str(), pathSize);
    }
    binaryManifestFile.close();

    //if (!fs::exists(manifestBinaryPath.parent_path())) fs::create_directories(manifestBinaryPath.parent_path());
   // fs::copy_file(manifestBinaryPath, globalManifestPath, fs::copy_options::overwrite_existing);
    
    ILEN_WARNING_P(DEBUG_PREFIX, "End of scene binarization: %.4f", ilen::time::Time::getTotalTime());
}