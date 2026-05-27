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

#include <ilen/headers/ilen-logs.h>
#include <ilen/core/time.h>
#include <ilen/job-system/job-system.h>
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
    binarizeScene(projectDataFolder);

    jobSystem.StopJobSystem();
    return 0;
}
 
void binarizeScene(std::filesystem::path projectDataFolder) {
    std::filesystem::path sourceSceneFolder = projectDataFolder / "scenes";
    std::filesystem::path ilenEngineFolder = projectDataFolder.parent_path();
    std::filesystem::path binaryScenesFolder = ilenEngineFolder / ".ilen-bin-data" / "scenes";

    //Ruta para el manifiesto
    std::filesystem::path manifestJsonPath = sourceSceneFolder / "manifests" / "scenes.meta_ilenmanifest";
    std::filesystem::path manifestBinaryPath = binaryScenesFolder.parent_path() / "manifests" / "scenes.ilenmanifest";

    if(!std::filesystem::exists(sourceSceneFolder)){
        ILEN_ERROR_P(DEBUG_PREFIX, "No se encontro la carpeta de escenas en %s", sourceSceneFolder.c_str());
        return;
    }

    if(!std::filesystem::exists(binaryScenesFolder)) {
        std::filesystem::create_directories(binaryScenesFolder);
    }

    ILEN_WARNING_P(DEBUG_PREFIX, "Start of scene binarization: %.4f", ilen::time::Time::getTotalTime());

    //Creación del json
    json manifestData;
    std::vector<std::string> existingPaths;
    uint32_t maxID = 0;

    if(std::filesystem::exists(manifestJsonPath)) {
        std::ifstream file(manifestJsonPath);
        file >> manifestData;
        maxID = manifestData.value(SCENE_MANIFEST_TOTAL, 0);

        if(manifestData.contains(SCENE_MANIFEST_LIST)) {
            for(const auto& sceneData : manifestData[SCENE_MANIFEST_LIST]) {
                existingPaths.push_back(sceneData.value(SCENE_MANIFEST_PATH, ""));
            }
        }
    }else {
        manifestData[SCENE_MANIFEST_TOTAL] = 0;
        manifestData[SCENE_MANIFEST_LIST] = json::array();
    }

    bool needSaveNewManifest = false;
    std::vector<std::filesystem::path> scenesToIndex;

    for(const auto& entry : std::filesystem::recursive_directory_iterator(sourceSceneFolder)) {
        if(entry.is_regular_file() && entry.path().extension() == ".meta_ilenscene") {
            std::filesystem::path relativePath = std::filesystem::relative(entry.path(), sourceSceneFolder);
            scenesToIndex.push_back(entry.path());

            auto result = std::find(existingPaths.begin(), existingPaths.end(), relativePath.generic_string());
            if(result == existingPaths.end()){
                //No existe
                needSaveNewManifest = true;
                maxID++;

                std::string sceneName = entry.path().stem().string();
                std::ifstream tempSceneFile(entry.path());
                if(tempSceneFile.is_open()){
                    json tempScenJson;
                    try {
                        tempSceneFile >> tempScenJson;
                        sceneName = tempScenJson.value("scene_name", sceneName);
                    }catch(...) {}
                }

                json newScene;
                newScene[SCENE_MANIFEST_ID] = maxID;
                newScene[SCENE_MANIFEST_NAME] = sceneName;
                newScene[SCENE_MANIFEST_PATH] = relativePath.generic_string();

                manifestData[SCENE_MANIFEST_LIST].push_back(newScene);
            }           
        }
    }

    if(needSaveNewManifest) {
        manifestData[SCENE_MANIFEST_TOTAL] = maxID;
        if(!std::filesystem::exists(manifestJsonPath.parent_path())) {
            std::filesystem::create_directories(manifestJsonPath.parent_path());
        }

        std::ofstream outFile(manifestJsonPath);
        outFile << manifestData.dump(4);
        outFile.close();
        ILEN_LOG_P(DEBUG_PREFIX, "Manifiesto JSON de escenas actualizado.");
    }

    //Binarización de los metadatos (.meta_ilenscene -> .ilenscene)
    size_t total = scenesToIndex.size();
    if(total == 0) {
        ILEN_WARNING_P(DEBUG_PREFIX, "No se encontraron escenas para binarizar");
        return;
    }

    size_t threads = std::max<size_t>(1, jobSystem.AvailableThreads());//Esto es para asegurar al menos un worker.
    size_t idealChunkSize = (total + threads - 1) / threads;
    size_t chunkSize = std::clamp<size_t>(idealChunkSize, 10, 50);
    size_t numChunks = (total + chunkSize - 1) / chunkSize;

    ilen::job_system::JobGroup group;
    for(size_t i = 0; i < numChunks; i++) {
        size_t start = i * chunkSize;
        size_t end = std::min(start + chunkSize, total);

        jobSystem.SubmitJob([=, &scenesToIndex, &sourceSceneFolder, &binaryScenesFolder](){
            for(size_t j = start; j < end; j++) {
                std::filesystem::path currentScenePath = scenesToIndex[j];
                std::filesystem::path relativePath = std::filesystem::relative(currentScenePath, sourceSceneFolder);
                std::filesystem::path savePath = binaryScenesFolder / relativePath; 
                savePath.replace_extension(".ilenscene");
                
                if(std::filesystem::exists(savePath)) {
                    if(std::filesystem::last_write_time(currentScenePath) <= std::filesystem::last_write_time(savePath)) {
                        continue;
                    }
                }

                std::ifstream inputFile(currentScenePath);
                if(!inputFile.is_open()) {
                    ILEN_ERROR_P(DEBUG_PREFIX, "Fallo al abrir JSON de escena: %s", currentScenePath.string().c_str());
                    continue;
                }

                json sceneJson;
                try {
                    inputFile >> sceneJson;
                }catch(json::parse_error& e) {
                    ILEN_ERROR_P(DEBUG_PREFIX, "Error parseando %s: %s", currentScenePath.string().c_str(), e.what());
                    continue;
                }

                std::unordered_map<uint32_t, SceneComponentBuffer> componentBuffers;
                std::vector<ilen::ecs::Entity> aliveEntities;

                if(sceneJson.contains("entities")) {
                    for(const auto& entityJson : sceneJson["entities"]) {
                        ilen::ecs::Entity entityID = entityJson["id"];
                        aliveEntities.push_back(entityID);

                        for(auto& [compName, compData] : entityJson["components"].items()) {
                           uint32_t hashID = HashComponent(compName);
                           auto& buffer = componentBuffers[hashID];
                           buffer.componentHash = hashID;
                           buffer.entityCount++;
                           buffer.entityIDs.push_back(entityID);
                           
                        }
                    }
                }
            }
        }, &group);
    }
}