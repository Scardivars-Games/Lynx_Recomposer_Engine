#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <atomic>
#include <memory>
#include <cstdint>

#include <ilen/ecs/ecs.h>
#include <ilen/ecs/types.h>
#include <ilen/resources/binary_headers.h>
#include <ilen/headers/ilen_logs.h>

namespace ilen::resources::scene {

    //Función para el hashing
    static constexpr uint32_t HashComponent(std::string_view str) {
        uint32_t hash = 2166136261u;
        for(char c : str) {
            hash ^= static_cast<uint32_t>(c);
            hash *= 16777619u;
        }
        return hash;
    }

    //Para poder conocer el estado de la carga de las escenas
    enum class AsyncLoadingPhase {
        None,
        ReadingDisk, //Leyendo escena
        WaitingForSystems, //Componentes en el ecs, esperando por los sistemas a cargar
        PendingActivation, //Escena lista, esperando orden para mostrar
        Done
    };

    enum class LoadSceneMode {
        Single, 
        Additive
    };

    class AsyncOperation {
        public:
            AsyncOperation() : progress(0.0f), phase(AsyncLoadingPhase::None), allowSceneActivation(true) {};

            std::atomic<float> progress;
            std::atomic<AsyncLoadingPhase> phase;
            std::atomic<bool> allowSceneActivation;
            bool isDone() const { return phase == AsyncLoadingPhase::Done; }
    };

    using AsyncOperationPtr = std::shared_ptr<AsyncOperation>;
    using SceneID = uint32_t;
    using namespace ilen::ecs;

    class SceneManager {
        public:
            using DeseralizationCallback = std::function<void(
                ilen::ecs::Entity,
                char*&,
                const std::unordered_map<UUID, Entity>&,
                EngineContext*
            )>;

            explicit SceneManager(EngineContext* context);
            ~SceneManager() = default;

            void Update(float dt);

            void LoadScene(const std::string& name, LoadSceneMode mode = LoadSceneMode::Single);
            void LoadScene(SceneID id, LoadSceneMode mode = LoadSceneMode::Single);

            AsyncOperationPtr LoadSceneAsync(const std::string& name, LoadSceneMode mode = LoadSceneMode::Single);
            AsyncOperationPtr LoadSceneAsync(SceneID id, LoadSceneMode mode = LoadSceneMode::Single);

            void UnloadScene(const std::string& name);
            void UnloadScene(SceneID id);

            void DontDestroyOnLoad(Entity entity);

            //Control de escena activas
            void SetActiveScene(const std::string& name);
            void SetActiveScene(SceneID id);
            SceneID GetActiveScene() const { return m_activeSceneID; }

            //Prefabs
            std::vector<Entity> LoadPrefab(const std::string& name);

            //Registros de parseadores
            std::unordered_map<uint32_t, DeseralizationCallback> m_deserializers;
        private:
            void LoadManifest();
            std::vector<Entity> ParseSceneFile(const std::string& binaryFilePath);
            void LoadSceneAsyncInternal(SceneID id, LoadSceneMode mode, AsyncOperationPtr operation);

            ilen::EngineContext* p_context;

            //Datos obtenidos del manifiesto
            std::unordered_map<std::string, SceneID> m_nameToID;
            std::unordered_map<SceneID, std::string> m_idToPath;
            std::unordered_map<std::string, std::string> m_prefabNameToPath;

            //Manejo de las entidades por escena
            std::unordered_map<SceneID, std::vector<Entity>> m_loadedScenes;
            std::vector<Entity> m_persistentEntities;

            //Funcionamiento async
            AsyncOperationPtr m_currentAsyncOp;
            SceneID m_loadingSceneID;
            std::vector<Entity> m_pendingEntities;

            //Escena activa
            SceneID m_activeSceneID = 0;

            std::mutex m_mutex;
            const char* DEBUG_PREFIX = "SCENE_MANAGER";
    };
}