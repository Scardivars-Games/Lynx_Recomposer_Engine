#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <atomic>
#include <memory>
#include <cstdint>
#include <future>

#include <lynx/ecs/ecs.h>
#include <lynx/ecs/types.h>
#include <lynx/headers/maths.h>
#include <lynx/resources/binary_headers.h>
#include <lynx/headers/lynx_logs.h>

namespace lynx::resources::scene {

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
        Done,
        Failed
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
            bool isDone() const { return phase == AsyncLoadingPhase::Done || phase == AsyncLoadingPhase::Failed; }
    };

    using AsyncOperationPtr = std::shared_ptr<AsyncOperation>;
    using SceneID = uint32_t;
    using namespace lynx::ecs;

    class SceneManager {
        public:
            using DeserializationCallback = std::function<void(
                lynx::ecs::Entity,
                char*&,
                const std::unordered_map<UUID, Entity>&,
                EngineContext*
            )>;

            void RegisterDeserializer(std::uint32_t componentHash, DeserializationCallback callback);

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
            std::vector<Entity> LoadPrefab(const std::string& name, lynx::maths::vector::vec3 prefabPosition);

        private:
            void LoadManifest();
            std::vector<Entity> ParseSceneFile(const std::string& binaryFilePath);
            struct ComponentBlock {
                uint32_t hash;
                std::vector<UUID> owners;
                std::vector<char> data;
            };
            struct ParsedScene {
                std::vector<UUID> uuids;
                std::vector<ComponentBlock> blocks;
            };
            static ParsedScene ReadSceneFile(const std::string& path);
            std::vector<Entity> InstantiateScene(ParsedScene& scene);
            void BeginLoad(SceneID id, LoadSceneMode mode, AsyncOperationPtr operation);
            std::future<ParsedScene> m_pendingScene;
            LoadSceneMode m_loadingMode = LoadSceneMode::Single;
            
            //Registros de parseadores
            std::unordered_map<uint32_t, DeserializationCallback> m_deserializers;

            lynx::EngineContext* p_context;

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