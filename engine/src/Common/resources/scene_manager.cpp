#include <lynx/resources/scene_manager.h>
#include <lynx/headers/ecs_components.h>
#include <lynx/core/filemanager.h>
//#include <manifests/generated_serializers.inl>
#include <fstream>
#include <algorithm>
#include <thread>
#include <stdexcept>
#include <unordered_set>
#include <lynx/core/application.h>
#include <utility>

using namespace lynx::resources::scene;

SceneManager::SceneManager(EngineContext* context) : p_context(context) {
    LoadManifest();
    //AutoRegisterDeserializers(this, p_context);
}

void SceneManager::RegisterDeserializer(std::uint32_t componentHash, DeserializationCallback callback) {
    if(!callback) {
        LYNX_EXECUTE_ERROR_P("SCENE_MANAGER", "No se puede registrar un deserializador vacío. ENGINE ERROR");
    }

    m_deserializers.insert_or_assign(componentHash, std::move(callback));
}

void SceneManager::LoadManifest() {
    auto sceneManifestPath = lynx::core::FileManager::fileRoute("scenes.lynxmanifest", LYNX_FILE_TYPE::LYNX_FILE_TYPE_ASSETS_MANIFEST);

    std::ifstream file(sceneManifestPath, std::ios::binary);
    if(!file.is_open()){
        LYNX_WARNING_P(DEBUG_PREFIX, "No se logro encontrar un manifiesto de escenas");
        return;
    }

    lynx::binary::headers::LynxManifestHeader header{};
    file.read(reinterpret_cast<char*>(&header), sizeof(header));

    std::vector<std::string> orderedName(header.totalOfNamesToID);

    for(size_t i = 0; i < header.totalOfNamesToID; i++){
        SceneID id;
        uint32_t nameSize;
        file.read(reinterpret_cast<char*>(&id), sizeof(uint32_t));
        file.read(reinterpret_cast<char*>(&nameSize), sizeof(uint32_t));
        std::string name(nameSize, '\0');
        file.read(name.data(), nameSize);

        orderedName[i] = name;

        if(id != 0) {
            m_nameToID[name] = id;  
        }
    }

    for(size_t i = 0; i < header.totalOfNamesToID; i++){
        SceneID id;
        uint32_t pathSize;
        file.read(reinterpret_cast<char*>(&id), sizeof(uint32_t));
        file.read(reinterpret_cast<char*>(&pathSize), sizeof(uint32_t));
        std::string path(pathSize, '\0');
        file.read(path.data(), pathSize);

        //Todos los prefabs tienen como ID de escena el 0.
        if(id == 0) {
            m_prefabNameToPath[orderedName[i]] = path;
        }else {
            m_idToPath[id] = path;
        }
    }
}

SceneManager::ParsedScene SceneManager::ReadSceneFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("No se pudo abrir escena: " + path);
    auto remaining = file.tellg();
    file.seekg(0);
    auto read = [&](void* destination, size_t bytes) {
        if (bytes > static_cast<size_t>(remaining)) throw std::runtime_error("Escena truncada: " + path);
        if (bytes && !file.read(static_cast<char*>(destination), bytes)) throw std::runtime_error("Error leyendo escena: " + path);
        remaining -= static_cast<std::streamoff>(bytes);
    };
    lynx::binary::headers::LynxSceneHeader header{};
    read(&header, sizeof(header));
    if (std::string_view(header.magicNumber, 10) != "ENECS_NELI" || header.version != 1 || header.totalEntities > application::Application::getEngineContext()->runtimeConfiguration->ecs.initialEntityCapacity)
        throw std::runtime_error("Cabecera de escena inválida: " + path);
    ParsedScene scene;
    scene.uuids.resize(header.totalEntities);
    read(scene.uuids.data(), scene.uuids.size() * sizeof(UUID));
    std::unordered_set<UUID> ids(scene.uuids.begin(), scene.uuids.end());
    if (ids.size() != scene.uuids.size() || ids.contains(0)) 
        throw std::runtime_error("UUIDs inválidos en escena");
    for (uint32_t i = 0; i < header.totalComponentTypes; ++i) {
        uint32_t hash, count, size;
        read(&hash, sizeof(hash)); read(&count, sizeof(count)); read(&size, sizeof(size));
        if (count > header.totalEntities || static_cast<uint64_t>(count) * sizeof(UUID) + size > static_cast<uint64_t>(remaining))
            throw std::runtime_error("Bloque de componentes inválido");
        ComponentBlock block{hash, std::vector<UUID>(count), std::vector<char>(size)};
        read(block.owners.data(), count * sizeof(UUID));
        read(block.data.data(), size);
        for (auto id : block.owners) if (!ids.contains(id)) throw std::runtime_error("Componente sin entidad válida");
        scene.blocks.push_back(std::move(block));
    }
    return scene;
}

std::vector<Entity> SceneManager::InstantiateScene(ParsedScene& scene) {
    for (const auto& block : scene.blocks)
        if (!m_deserializers.contains(block.hash)) throw std::runtime_error("Componente desconocido en escena");
    std::vector<Entity> entities;
    std::unordered_map<UUID, Entity> mapping;
    try {
        for (auto id : scene.uuids) {
            auto entity = p_context->p_coordinator->createEntity();
            mapping.emplace(id, entity);
            entities.push_back(entity);
        }
        for (auto& block : scene.blocks) {
            char* stream = block.data.data();
            for (auto id : block.owners)
                m_deserializers.at(block.hash)(mapping.at(id), stream, mapping, p_context);
        }
    } catch (...) {
        for (auto entity : entities) p_context->p_coordinator->destroyEntity(entity);
        throw;
    }
    return entities;
}

std::vector<Entity> SceneManager::ParseSceneFile(const std::string& path) {
    try {
        auto scene = ReadSceneFile(path);
        return InstantiateScene(scene);
    } catch (const std::exception& error) {
        LYNX_ERROR_P(DEBUG_PREFIX, "Error cargando prefab: %s", error.what());
        return {};
    }
}

void SceneManager::DontDestroyOnLoad(Entity entity){
    std::lock_guard<std::mutex> lock(m_mutex);
    if(std::find(m_persistentEntities.begin(), m_persistentEntities.end(), entity) == m_persistentEntities.end()) {
        m_persistentEntities.push_back(entity);
    }
}

void SceneManager::LoadScene(SceneID id, LoadSceneMode mode) {
    if (!lynx::core::threading::t_IsMainThread) {
        LYNX_ERROR_P(DEBUG_PREFIX, "LoadScene síncrono requiere el hilo principal; use LoadSceneAsync");
        return;
    }
    AsyncOperationPtr op = LoadSceneAsync(id, mode);
    if(!op) return;

    while (!op->isDone())
    {
        //Al no ser una carga asyncrona, bloqueamos este hilo hasta cargar los datos
        Update(lynx::time::Time::getDeltaTime());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

AsyncOperationPtr SceneManager::LoadSceneAsync(SceneID id, LoadSceneMode mode) {
    if(!m_idToPath.contains(id)) {
        LYNX_ERROR_P(DEBUG_PREFIX, "ID de escena no encontrado: %u", id);
        return nullptr;
    }

    if(id == 0) {
        LYNX_ERROR_P(DEBUG_PREFIX, "ID 0 es exclusivo de un prefab. Use la función LoadPrefab para cargar un prefab desde la función interna, o desde EntityAPI::LoadPrefab si estas en la API pública");
        return nullptr;
    }

    auto operation = std::make_shared<AsyncOperation>();
    operation->phase = AsyncLoadingPhase::ReadingDisk;
    if (!lynx::core::threading::t_IsMainThread) {
        p_context->p_commandDispatcher->getBuffer(lynx::core::threading::t_ThreadIndex).push(
            [this, id, mode, operation] { BeginLoad(id, mode, operation); });
    } else {
        BeginLoad(id, mode, operation);
    }
    return operation;
}

void SceneManager::BeginLoad(SceneID id, LoadSceneMode mode, AsyncOperationPtr operation) {
    if (m_currentAsyncOp) {
        LYNX_ERROR_P(DEBUG_PREFIX, "Ya hay una carga de escena en curso");
        operation->phase = AsyncLoadingPhase::Failed;
        return;
    }
    m_loadingSceneID = id;
    m_loadingMode = mode;
    m_currentAsyncOp = operation;
    auto promise = std::make_shared<std::promise<ParsedScene>>();
    m_pendingScene = promise->get_future();
    auto path = lynx::core::FileManager::fileRoute(m_idToPath.at(id), LYNX_FILE_TYPE_SCENE);
    // El trabajador solo lee datos. No toca entidades, componentes ni colas del ECS.
    p_context->p_jobSystem->SubmitJob([path, promise] {
        try { promise->set_value(ReadSceneFile(path)); }
        catch (...) { promise->set_exception(std::current_exception()); }
    });
}

void SceneManager::Update(float dt) {
    if (!lynx::core::threading::t_IsMainThread) return;
    p_context->p_commandDispatcher->ExecuteAll();
    if (!m_currentAsyncOp) return;
    if (m_currentAsyncOp->phase == AsyncLoadingPhase::ReadingDisk) {
        if (m_pendingScene.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
        try {
            auto scene = m_pendingScene.get();
            for (const auto& block : scene.blocks)
                if (!m_deserializers.contains(block.hash)) throw std::runtime_error("Componente desconocido en escena");
            if (m_loadingMode == LoadSceneMode::Single) {
                std::lock_guard lock(m_mutex);
                for (const auto& [id, entities] : m_loadedScenes)
                    for (auto entity : entities)
                        if (std::find(m_persistentEntities.begin(), m_persistentEntities.end(), entity) == m_persistentEntities.end())
                            p_context->p_coordinator->destroyEntity(entity);
                m_loadedScenes.clear();
                m_activeSceneID = 0;
            }
            m_pendingEntities = InstantiateScene(scene);
            m_currentAsyncOp->phase = AsyncLoadingPhase::WaitingForSystems;
        } catch (const std::exception& error) {
            LYNX_ERROR_P(DEBUG_PREFIX, "Error cargando escena: %s", error.what());
            m_currentAsyncOp->phase = AsyncLoadingPhase::Failed;
            m_currentAsyncOp.reset();
            return;
        }
    }

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

                m_activeSceneID = m_loadingSceneID;
            }

            m_currentAsyncOp->progress = 1.0f;
            m_currentAsyncOp->phase = AsyncLoadingPhase::Done;
            m_currentAsyncOp = nullptr;

            LYNX_LOG_P(DEBUG_PREFIX, "Escena instanciada y activa");
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
        LYNX_ERROR_P(DEBUG_PREFIX, "Escena no encontrada: %s", name.c_str());
        return;
    }
    LoadScene(m_nameToID[name], mode);
}

AsyncOperationPtr SceneManager::LoadSceneAsync(const std::string& name, LoadSceneMode mode) {
    if(!m_nameToID.contains(name)){
        LYNX_ERROR_P(DEBUG_PREFIX, "Escena no encontrada: %s", name.c_str());
        return nullptr;
    }
    return LoadSceneAsync(m_nameToID[name], mode);
}

void SceneManager::UnloadScene(const std::string& name) {
    if(m_nameToID.contains(name)){
        UnloadScene(m_nameToID[name]);
    }
}

std::vector<Entity> SceneManager::LoadPrefab(const std::string& name) {
    if (!lynx::core::threading::t_IsMainThread) {
        p_context->p_commandDispatcher->getBuffer(lynx::core::threading::t_ThreadIndex).push(
            [this, name] { LoadPrefab(name); });
        return {}; // Desde scripts trabajadores se instancia al vaciar la cola principal.
    }
    if(!m_prefabNameToPath.contains(name)) {
        LYNX_ERROR_P(DEBUG_PREFIX, "Prefab no encontrado en el manifiesto: %s", name.c_str());
        return {};
    }
    
    if (m_activeSceneID == 0) {
        LYNX_ERROR_P(DEBUG_PREFIX, "Intento de instanciar el prefab '%s' sin una escena activa. Abortando para evitar memory leaks.", name.c_str());
        return {};
    }

    std::string prefabPath = lynx::core::FileManager::fileRoute(m_prefabNameToPath[name], LYNX_FILE_TYPE_PREFAB);

    std::vector<Entity> instancedEntities = ParseSceneFile(prefabPath);

    if (!instancedEntities.empty() && m_activeSceneID != 0) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto& activeSceneList = m_loadedScenes[m_activeSceneID];
        activeSceneList.insert(activeSceneList.end(), instancedEntities.begin(), instancedEntities.end());
    }
    return instancedEntities;
}

std::vector<Entity> SceneManager::LoadPrefab(const std::string& name, lynx::maths::vector::vec3 prefabPosition) {
    if(!lynx::core::threading::t_IsMainThread) {
        auto& buffer = p_context->p_commandDispatcher->getBuffer(
            lynx::core::threading::t_ThreadIndex
        );

        buffer.push([this, name, prefabPosition] {
            LoadPrefab(name, prefabPosition);
        });
        return {};
    }

    auto entities = LoadPrefab(name);

    for(Entity entity: entities) {
        auto* transform = p_context->p_coordinator->getComponent<lynx::ecs::components::TransformComponent>(entity);

        if(!transform){
            continue;
        }

        if(transform->parent != NULL_ENTITY) {
            continue;
        }

        transform->position += prefabPosition;
    }

    return entities;
}

void SceneManager::SetActiveScene(SceneID id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_loadedScenes.contains(id)) {
        m_activeSceneID = id;
    } else {
        LYNX_WARNING_P(DEBUG_PREFIX, "No se puede establecer como principal la escena %u porque no esta cargada.", id);
    }
}

void SceneManager::SetActiveScene(const std::string& name) {
    if (m_nameToID.contains(name)) {
        SetActiveScene(m_nameToID[name]);
    } else {
        LYNX_WARNING_P(DEBUG_PREFIX, "Escena no encontrada para hacerla principal: %s", name.c_str());
    }
}