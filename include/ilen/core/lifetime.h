//PUNTO DE ENTRADA DEL MOTOR. ESTE ENCABEZADO CONTINE LA LóGICA DE TODO EL MOTOR. AQUI SE INICIALIZA DESDE EL ENTRY POINT DEL OS.
#pragma once
#include <ilen/headers/visual.h>
#include <ilen/headers/core.h>
#include <ilen/job-system/job-system.h>
#include <memory>

namespace ilen::ecs::systems {
    class RenderSystem;
    class TransformSystem;
    class ScriptingSystem;
}

namespace ilen::resources {
    class AssetsManager;
}

namespace ilen::resources::mesh {
    class MeshManager;
}

namespace ilen {
    //Almacena todos los punteros necesarios para el funcionamiento correcto del motor. Se debe crear al lado de los metadatos del motor
    struct EngineContext{
        std::unique_ptr<ilen::job_system::JobSystem> p_jobSystem;
        std::unique_ptr<window::GameWindow> p_gameWindow;
        std::unique_ptr<render::Renderer> p_gameRenderer;
        std::unique_ptr<ilen::ecs::ECSCoordinator> p_coordinator;
        std::unique_ptr<ilen::resources::mesh::MeshManager> p_meshManager;
        std::unique_ptr<ilen::resources::AssetsManager> p_assetsManager;

        //ECS Systems (shared ptrs)
        std::shared_ptr<ilen::ecs::systems::RenderSystem> p_renderSystem;
        std::shared_ptr<ilen::ecs::systems::TransformSystem> p_transformSystem;
        std::shared_ptr<ilen::ecs::systems::ScriptingSystem> p_scriptingSystem;
    };

    class AppRuntime {
        public:
            static AppRuntime& getInstance();
            void init(application::ApplicationMetadata metadata); //Inicia el motor
            void update(); //Ejecuta el loop del motor
            void shutdown(); //Detiene el motor
            bool isRunning();
        private:
            AppRuntime() = default;
            const static inline char* DEBUG_PREFIX = "APP_RUNTIME";
            std::unique_ptr<EngineContext> engineContext;

    };
}