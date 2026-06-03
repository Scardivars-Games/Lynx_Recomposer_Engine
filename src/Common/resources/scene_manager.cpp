#include <ilen/resources/scene_manager.h>
#include <ilen/headers/ecs_components.h>
#include <ilen/core/filemanager.h>
#include <manifests/generated_serializers.inl>
#include <fstream>
#include <algorithm>
#include <thread>

using namespace ilen::resources::scene;

SceneManager::SceneManager(EngineContext* context) : p_context(context) {
    LoadManifest();
    AutoRegisterDeserializers(this, p_context);
}

void SceneManager::LoadManifest() {
    auto sceneManifestPath = ilen::core::FileManager::fileRoute("scenes.ilenmanifest", ILEN_FILE_TYPE::ILEN_FILE_TYPE_ASSETS_MANIFEST);

    std::ifstream file(sceneManifestPath, std::ios::binary);
    if(!file.is_open()){
        ILEN_WARNING_P(DEBUG_PREFIX, "No se logro encontrar un manifiesto de escenas");
        return;
    }

    ilen::binary::headers::IlenManifestHeader header{};
    file.read(reinterpret_cast<char*>(&header), sizeof(header));

    for(size_t i = 0; i < header.totalOfNamesToID; i++){
        SceneID id;
        uint32_t nameSize;
        file.read(reinterpret_cast<char*>(&id), sizeof(uint32_t));
        file.read(reinterpret_cast<char*>(&nameSize), sizeof(uint32_t));
        std::string name(nameSize, '\0');
        file.read(name.data(), nameSize);
        m_nameToID[name] = id;
    }

    for(size_t i = 0; i < header.totalOfNamesToID; i++){
        SceneID id;
        uint32_t pathSize;
        file.read(reinterpret_cast<char*>(&id), sizeof(uint32_t));
        file.read(reinterpret_cast<char*>(&pathSize), sizeof(uint32_t));
        std::string path(pathSize, '\0');
        file.read(path.data(), pathSize);
        m_idToPath[id] = path;
    }
}

std::vector<Entity> SceneManager::ParseSceneFile(const std::string& binaryFilePath){
    std::vector<Entity> runtimeEntities;

    std::ifstream file(binaryFilePath, std::ios::binary);
    if(!file.is_open()) {
        ILEN_ERROR_P(DEBUG_PREFIX, "Fallo al abrir escena: %s", binaryFilePath.c_str());
        return runtimeEntities;
    }

    ilen::binary::headers::IlenSceneHeader header{};
    file.read(reinterpret_cast<char*>(&header), sizeof(header));

    std::vector<UUID> savedUUIDs(header.totalEntities);
    file.read(reinterpret_cast<char*>(savedUUIDs.data()), header.totalEntities * sizeof(UUID));

    std::unordered_map<UUID, Entity> uuidToRuntimeID;
    runtimeEntities.reserve(header.totalEntities);

    for(uint32_t i = 0; i < header.totalEntities; i++) {
        Entity newEntity = p_context->p_coordinator->createEntity();
        uuidToRuntimeID[savedUUIDs[i]] = newEntity;
        runtimeEntities.push_back(newEntity);
    }

    for(uint32_t i = 0; i < header.totalComponentTypes; i++) {
        uint32_t compHash, entityCount, dataSizeBytes;
        file.read(reinterpret_cast<char*>(&compHash), sizeof(uint32_t));
        file.read(reinterpret_cast<char*>(&entityCount), sizeof(uint32_t));
        file.read(reinterpret_cast<char*>(&dataSizeBytes), sizeof(uint32_t));

        std::vector<UUID> componentOwnerUUIDs(entityCount);
        file.read(reinterpret_cast<char*>(componentOwnerUUIDs.data()), sizeof(UUID) * entityCount);
        
        std::vector<char> rawData(dataSizeBytes);
        file.read(rawData.data(), dataSizeBytes);

        auto deserializerIt = m_deserializers.find(compHash);
        if(deserializerIt != m_deserializers.end()) {
            char* currentStream = rawData.data();

            for(uint32_t j = 0; j < entityCount; j++)  {
                Entity realEntity = uuidToRuntimeID[componentOwnerUUIDs[j]];
                deserializerIt->second(realEntity, currentStream, uuidToRuntimeID, p_context);
            }
        } else {
            ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "Se ha tratado de cargar una escena con datos no válidos para esta aplicacion");
            return runtimeEntities;
        }   
    }

    return runtimeEntities;
}

void SceneManager::DontDestroyOnLoad(Entity entity){
    std::lock_guard<std::mutex> lock(m_mutex);
    if(std::find(m_persistentEntities.begin(), m_persistentEntities.end(), entity) == m_persistentEntities.end()) {
        m_persistentEntities.push_back(entity);
    }
}

void SceneManager::LoadScene(SceneID id, LoadSceneMode mode) {
    AsyncOperationPtr op = LoadSceneAsync(id, mode);
    if(!op) return;

    while (!op->isDone())
    {
        //Al no ser una carga asyncrona, bloqueamos este hilo hasta cargar los datos
        Update(ilen::time::Time::getDeltaTime());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

AsyncOperationPtr SceneManager::LoadSceneAsync(SceneID id, LoadSceneMode mode) {
    if(!m_idToPath.contains(id)) {
        ILEN_ERROR_P(DEBUG_PREFIX, "ID de escena no encontrado: %u", id);
        return nullptr;
    }

    m_loadingSceneID = id;
    m_currentAsyncOp = std::make_shared<AsyncOperation>();
    m_currentAsyncOp->phase = AsyncLoadingPhase::ReadingDisk;
    m_currentAsyncOp->progress = 0.0f;

    p_context->p_jobSystem->SubmitJob([this, id, mode, op = m_currentAsyncOp]() {
        LoadSceneAsyncInternal(id, mode, op);
    });

    return m_currentAsyncOp;
}

void SceneManager::LoadSceneAsyncInternal(SceneID id, LoadSceneMode mode, AsyncOperationPtr operation) {
    operation->progress = 0.1f;

    if(mode == LoadSceneMode::Single) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& [sceneID, entities] : m_loadedScenes) {
            for(auto entity: entities){
                if(std::find(m_persistentEntities.begin(), m_persistentEntities.end(), entity) == m_persistentEntities.end()) {
                    p_context->p_coordinator->destroyEntity(entity);
                }
            }
        }
        m_loadedScenes.clear();
    }

    operation->progress = 0.3f;

    std::string scenePath = ilen::core::FileManager::fileRoute(m_idToPath[id], ILEN_FILE_TYPE_SCENE);
    m_pendingEntities = ParseSceneFile(scenePath);

    operation->progress = 0.5f;
    operation->phase = AsyncLoadingPhase::WaitingForSystems;
}

void SceneManager::Update(float dt) {
    if(!m_currentAsyncOp) return;
    p_context->p_commandDispatcher->ExecuteAll();

    if(m_currentAsyncOp->phase == AsyncLoadingPhase::WaitingForSystems) {
        auto status = p_context->p_assetsManager->getAssetLoadStatus();
        if(status >= 0.99) {
            //La carga se ha completado
            m_currentAsyncOp->progress = 0.95f;
            m_currentAsyncOp->phase = AsyncLoadingPhase::PendingActivation;
        }else{
            float newProg = 0.45f * status;
            m_currentAsyncOp->progress = 0.5f + newProg;
        }
    }

    if(m_currentAsyncOp->phase == AsyncLoadingPhase::PendingActivation) {
        if(m_currentAsyncOp->allowSceneActivation) {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto& sceneList = m_loadedScenes[m_loadingSceneID];
                sceneList.insert(sceneList.end(), m_pendingEntities.begin(), m_pendingEntities.end());
                m_pendingEntities.clear();
            }

            m_currentAsyncOp->progress = 1.0f;
            m_currentAsyncOp->phase = AsyncLoadingPhase::Done;
            m_currentAsyncOp = nullptr;

            ILEN_LOG_P(DEBUG_PREFIX, "Escena instanciada y activa");
        }
    }
}

void SceneManager::UnloadScene(SceneID id) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_loadedScenes.find(id);
    if(it != m_loadedScenes.end()) {
        for(auto entity: it->second) {
            if(std::find(m_persistentEntities.begin(), m_persistentEntities.end(), entity) == m_persistentEntities.end()) {
                p_context->p_coordinator->destroyEntity(entity);
            }
        }

        m_loadedScenes.erase(it);
    }
}

void SceneManager::LoadScene(const std::string& name, LoadSceneMode mode) {
    if(!m_nameToID.contains(name)) {
        ILEN_ERROR_P(DEBUG_PREFIX, "Escena no encontrada: %s", name.c_str());
        return;
    }
    LoadScene(m_nameToID[name], mode);
}

AsyncOperationPtr SceneManager::LoadSceneAsync(const std::string& name, LoadSceneMode mode) {
    if(!m_nameToID.contains(name)){
        ILEN_ERROR_P(DEBUG_PREFIX, "Escena no encontrada: %s", name.c_str());
        return nullptr;
    }
    return LoadSceneAsync(m_nameToID[name], mode);
}

void SceneManager::UnloadScene(const std::string& name) {
    if(m_nameToID.contains(name)){
        UnloadScene(m_nameToID[name]);
    }
}
