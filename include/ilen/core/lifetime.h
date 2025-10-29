//PUNTO DE ENTRADA DEL MOTOR. ESTE ENCABEZADO CONTINE LA LóGICA DE TODO EL MOTOR. AQUI SE INICIALIZA DESDE EL ENTRY POINT DEL OS.
#pragma once
#include <ilen/headers/visual.h>
#include <ilen/headers/core.h>
#include <ilen/job-system/job-system.h>
#include <ilen/resources/shaders-manager.h>
#include <ilen/resources/materials-manager.h>
#include <ilen/resources/render-pipeline-manager.h>
#include <memory>


namespace ilen::ecs::systems {
    class RenderSystem;
    class TransformSystem;
    class ScriptingSystem;
}

namespace ilen::resources {
    class AssetsManager;
    namespace mesh {
        class MeshManager;
    }
    namespace shaders {
        class ShaderManager;
    }
    namespace materials {
        class MaterialManager;
    }
    namespace render_pipeline {
        class PipelineManager;
    }
}


namespace ilen {
    //Almacena todos los punteros necesarios para el funcionamiento correcto del motor. Se debe crear al lado de los metadatos del motor
    struct EngineContext{
        std::unique_ptr<ilen::job_system::JobSystem> p_jobSystem;
        std::unique_ptr<window::IWindow> p_gameWindow;
        std::unique_ptr<render::IRenderer> p_gameRenderer;
        std::unique_ptr<ilen::ecs::ECSCoordinator> p_coordinator;

        //Resources Managers
        std::unique_ptr<ilen::resources::AssetsManager> p_assetsManager;
        std::unique_ptr<ilen::resources::mesh::MeshManager> p_meshManager;
        std::unique_ptr<ilen::resources::shaders::ShaderManager> p_shaderManager;
        std::unique_ptr<ilen::resources::materials::MaterialManager> p_materialManager;
        std::unique_ptr<ilen::resources::render_pipeline::PipelineManager> p_renderPipelineManager;

        //ECS Systems (shared ptrs)
        std::shared_ptr<ilen::ecs::systems::RenderSystem> p_renderSystem;
        std::shared_ptr<ilen::ecs::systems::TransformSystem> p_transformSystem;
        std::shared_ptr<ilen::ecs::systems::ScriptingSystem> p_scriptingSystem;
    };

    class AppRuntime {
        public:
            static AppRuntime& getInstance();
            void init(const application::ApplicationMetadata &metadata); //Inicia el motor
            void update(); //Ejecuta el loop del motor
            void shutdown(); //Detiene el motor
            bool isRunning();
        private:
            AppRuntime() = default;
            const static inline char* DEBUG_PREFIX = "APP_RUNTIME";
            std::unique_ptr<EngineContext> engineContext;
    };
}