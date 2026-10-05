//Este módulo tendrá como fin binarizar los archivos de escenas del motor. Estos se encontraran en la carpeta del proyecto, y luego
//en project_data/scenes.

#include <cstdint>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <sys/types.h>
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
#include <ilen/resources/scene_manager.h>


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
    VectorFloat2 = 12,
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
            if(typeStr == "vector<float2>") return SchemaType::VectorFloat2;
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
                schema.hash = ilen::resources::scene::HashComponent(compName);
                schema.totalSizeBytes = compData.value("size", 0);

                for(const auto& fieldJson : compData["fields"]) {
                    SchemaField field;
                    field.name = fieldJson["name"];
                    field.offset = fieldJson["offset"];
                    field.size = fieldJson["size"];
                    field.type = ParseTypeString(fieldJson["type"]);

                    schema.fields.push_back(field);
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

            uint64_t presenceMask = 0;
            int fieldIdx = 0;

            for(const auto& field : schema.fields) {
                if(compJsonData.contains(field.name)){

                    presenceMask |= (1ULL << fieldIdx);

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
                    else if (field.type == SchemaType::Unknown) {
                        if (jVal.is_number_integer()) {
                            if (field.size == 1) { uint8_t v = jVal.get<uint8_t>(); std::memcpy(destPtr, &v, 1); }
                            else if (field.size == 2) { uint16_t v = jVal.get<uint16_t>(); std::memcpy(destPtr, &v, 2); }
                            else if (field.size == 4) { uint32_t v = jVal.get<uint32_t>(); std::memcpy(destPtr, &v, 4); }
                            else if (field.size == 8) { uint64_t v = jVal.get<uint64_t>(); std::memcpy(destPtr, &v, 8); }
                        }
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
                    } else if (field.type == SchemaType::VectorFloat2 && jVal.is_array()) {
                        const uint32_t count = static_cast<uint32_t>(jVal.size());
                        dynamicPayload.insert(dynamicPayload.end(),
                            reinterpret_cast<const char*>(&count),
                            reinterpret_cast<const char*>(&count) + sizeof(uint32_t)
                        );

                        for(const auto& item : jVal) {
                            if(!item.is_array() || item.size() < 2) {
                                ILEN_ERROR_P(DEBUG_PREFIX, "Un elemento de vector<float2> no contiene 2 valores.");
                                return false;
                            }

                            const float x = item[0].get<float>();
                            const float y = item[1].get<float>();

                            dynamicPayload.insert(
                                dynamicPayload.end(),
                                reinterpret_cast<const char*>(&x),
                                reinterpret_cast<const char*>(&x) + sizeof(float)
                            );

                            dynamicPayload.insert(
                                dynamicPayload.end(),
                                reinterpret_cast<const char*>(&y),
                                reinterpret_cast<const char*>(&y) + sizeof(float)
                            );
                        }
                    }
                } else {
                    if (field.type == SchemaType::String || 
                        field.type == SchemaType::VectorUint32 || 
                        field.type == SchemaType::VectorUint64 ||
                        field.type == SchemaType::VectorFloat2
                    ) {
                            uint32_t emptyLen = 0;
                            dynamicPayload.insert(dynamicPayload.end(), reinterpret_cast<char*>(&emptyLen), reinterpret_cast<char*>(&emptyLen) + sizeof(uint32_t));
                    }
                }

                fieldIdx++;
            }
            
            outBuffer.insert(outBuffer.end(), reinterpret_cast<char*>(&presenceMask), reinterpret_cast<char*>(&presenceMask) + sizeof(uint64_t));
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

void dataBinarization(std::filesystem::path archivePath, std::filesystem::path savePath) {
    std::ifstream inputFile(archivePath);
    if (!inputFile.is_open()) return;

    json sceneJson;
    try { inputFile >> sceneJson; } catch (...) { return; }

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
                uint32_t hashId = ilen::resources::scene::HashComponent(compName);
                ILEN_LOG("Hash guardado %u", hashId);
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
        std::ofstream outSourceFile(archivePath);
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
    outFile.write(reinterpret_cast<const char*>(aliveEntities.data()), header.totalEntities * sizeof(ilen::ecs::UUID));

    for (const auto& [hash, buffer] : componentBuffers) {
        outFile.write(reinterpret_cast<const char*>(&buffer.componentHash), sizeof(uint32_t));
        outFile.write(reinterpret_cast<const char*>(&buffer.entityCount), sizeof(uint32_t));
        
        uint32_t dataSizeBytes = buffer.rawData.size();
        outFile.write(reinterpret_cast<const char*>(&dataSizeBytes), sizeof(uint32_t));
        outFile.write(reinterpret_cast<const char*>(buffer.entityIDs.data()), buffer.entityCount * sizeof(ilen::ecs::UUID));
        outFile.write(buffer.rawData.data(), dataSizeBytes);
    }
    outFile.close();
}
 
void binarizeScene(std::filesystem::path projectDataFolder) {
    // Definición de las rutas para Escenas
    fs::path sourceScenesFolder = projectDataFolder / "scenes";
    fs::path binaryScenesFolder = projectDataFolder.parent_path() / ".ilen_bin_data" / "scenes";
    
    // Definición de las rutas para Prefabs
    fs::path sourcePrefabsFolder = projectDataFolder / "assets/prefabs";
    fs::path binaryPrefabsFolder = projectDataFolder.parent_path() / ".ilen_bin_data" / "prefabs";
    
    // Rutas del manifiesto compartido
    fs::path manifestJsonPath = sourceScenesFolder.parent_path() / "manifests" / "scenes.meta_ilenmanifest";
    fs::path manifestBinaryPath = binaryScenesFolder.parent_path() / "manifests" / "scenes.ilenmanifest";

    if (!fs::exists(sourceScenesFolder) && !fs::exists(sourcePrefabsFolder)) {
        ILEN_ERROR_P(DEBUG_PREFIX, "No se encontraron las carpetas de escenas ni de prefabs.");
        return;
    }

    ILEN_WARNING_P(DEBUG_PREFIX, "Start of data binarization: %.4f", ilen::time::Time::getTotalTime());

    // --- FASE 1: INDEXACIÓN DE ESCENAS Y PREFABS ---
    json manifestData;
    std::vector<std::string> existingPaths;
    
    // Empezamos en 1, porque el ID 0 ahora es exclusivo de los Prefabs
    uint32_t maxID = 1; 

    if (fs::exists(manifestJsonPath)) {
        std::ifstream file(manifestJsonPath);
        try { file >> manifestData; } catch (...) {}
        
        maxID = manifestData.value(SCENE_MANIFEST_TOTAL, 1);
        if (maxID == 0) maxID = 1; // Protección en caso de leer un manifiesto viejo
        
        if (manifestData.contains(SCENE_MANIFEST_LIST)) {
            for (const auto& sceneData : manifestData[SCENE_MANIFEST_LIST]) {
                existingPaths.push_back(sceneData.value(SCENE_MANIFEST_PATH, ""));
            }
        }
    } else {
        manifestData[SCENE_MANIFEST_TOTAL] = 1;
        manifestData[SCENE_MANIFEST_LIST] = json::array();
    }

    bool needSaveNewManifest = false;
    
    // Estructura para almacenar todos los trabajos y enviarlos juntos al JobSystem
    struct FileTask {
        fs::path inputPath;
        fs::path savePath;
    };
    std::vector<FileTask> filesToCompile;

    // Lambda para procesar cualquier carpeta, detectar archivos nuevos y añadirlos al manifiesto
    auto indexFolder = [&](const fs::path& srcFolder, const fs::path& binFolder, const std::string& ext, const std::string& binExt, bool isPrefab) {
        if (!fs::exists(srcFolder)) return;
        
        for (const auto& entry : fs::recursive_directory_iterator(srcFolder)) {
            if (entry.is_regular_file() && entry.path().extension() == ext) {
                fs::path relativePath = fs::relative(entry.path(), srcFolder);
                fs::path savePath = binFolder / relativePath;
                savePath.replace_extension(binExt);
                
                // Añadimos el archivo a la cola de compilación
                filesToCompile.push_back({entry.path(), savePath});

                // Si no está en el manifiesto, lo añadimos
                if (std::find(existingPaths.begin(), existingPaths.end(), relativePath.generic_string()) == existingPaths.end()) {
                    needSaveNewManifest = true;

                    std::string itemName = entry.path().stem().string();
                    std::ifstream tempFile(entry.path());
                    if (tempFile.is_open()) {
                        json tempJson;
                        // Funciona igual ya que el nodo raíz puede llamarse scene_name o omitirse para usar el archivo
                        try { tempFile >> tempJson; itemName = tempJson.value("scene_name", itemName); } 
                        catch (...) {}
                    }

                    json newItem;
                    // REGLA MAESTRA: Si es prefab asigna 0. Si es escena, usa y aumenta el maxID.
                    newItem[SCENE_MANIFEST_ID] = isPrefab ? 0 : maxID;
                    newItem[SCENE_MANIFEST_NAME] = itemName;
                    newItem[SCENE_MANIFEST_PATH] = relativePath.generic_string();
                    manifestData[SCENE_MANIFEST_LIST].push_back(newItem);
                    
                    if (!isPrefab) {
                        maxID++; 
                    }
                }
            }
        }
    };

    // Procesamos ambas carpetas
    indexFolder(sourceScenesFolder, binaryScenesFolder, ".meta_ilenscene", ".ilenscene", false);
    indexFolder(sourcePrefabsFolder, binaryPrefabsFolder, ".meta_ilenprefab", ".ilenprefab", true);

    if (needSaveNewManifest) {
        manifestData[SCENE_MANIFEST_TOTAL] = maxID;
        if (!fs::exists(manifestJsonPath.parent_path())) fs::create_directories(manifestJsonPath.parent_path());
        std::ofstream outFile(manifestJsonPath);
        outFile << manifestData.dump(4);
    }

    // --- FASE 2: COMPILACIÓN MULTIHILO (Escenas + Prefabs) ---
    size_t total = filesToCompile.size();
    if (total == 0) return;

    size_t threads = std::max<size_t>(1, jobSystem.AvailableThreads());
    size_t idealChunkSize = (total + threads - 1) / threads;
    size_t chunkSize = std::max<size_t>(10, std::min<size_t>(50, idealChunkSize));
    size_t numChunks = (total + chunkSize - 1) / chunkSize;

    ilen::job_system::JobGroup group;

    for (size_t i = 0; i < numChunks; i++) {
        size_t start = i * chunkSize;
        size_t end = std::min(start + chunkSize, total);

        jobSystem.SubmitJob([=, &filesToCompile]() {
            for (size_t j = start; j < end; j++) {
                // Llamamos a tu función modular con las rutas preparadas
                dataBinarization(filesToCompile[j].inputPath, filesToCompile[j].savePath);
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
        
        // INTERCEPCIÓN: Reemplazamos la extensión dependiendo de si el ID indica que es Prefab o Escena
        std::string compiledPath;
        if (id == 0) {
            compiledPath = originalPath.replace_extension(".ilenprefab").generic_string();
        } else {
            compiledPath = originalPath.replace_extension(".ilenscene").generic_string();
        }

        uint32_t pathSize = compiledPath.length();
        binaryManifestFile.write(reinterpret_cast<const char*>(&id), sizeof(uint32_t));
        binaryManifestFile.write(reinterpret_cast<const char*>(&pathSize), sizeof(uint32_t));
        binaryManifestFile.write(compiledPath.c_str(), pathSize);
    }
    binaryManifestFile.close();
    
    ILEN_WARNING_P(DEBUG_PREFIX, "End of data binarization: %.4f", ilen::time::Time::getTotalTime());
}