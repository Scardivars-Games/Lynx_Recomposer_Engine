//PUNTO DE ENTRADA DEL MOTOR. ESTE ENCABEZADO CONTINE LA LóGICA DE TODO EL MOTOR. AQUI SE INICIALIZA DESDE EL ENTRY POINT DEL OS.
#pragma once
#include <lynx/headers/visual.h>
#include <lynx/headers/core.h>
#include <lynx/job_system/job_system.h>
#include <lynx/resources/shaders_manager.h>
#include <lynx/resources/materials_manager.h>
#include <lynx/resources/render_pipeline_manager.h>
#include <lynx/camera/camera_system.h>
#include <lynx/core/project_bootstrap.h>
#include <memory>


namespace lynx::ecs::systems {
    class RenderSystem;
    class TransformSystem;
    class ScriptingSystem;
    class CameraSystem;
    class RenderDrawSystem;
    class PostProcessSystem;
}


namespace lynx {
    class AppRuntime final{
        public:
            static AppRuntime& getInstance();
            void init(core::ProjectBootsrap bootsrap); //Inicia el motor
            void update() const; //Ejecuta el loop del motor
            void shutdown() const; //Detiene el motor
            bool isRunning() const;
            EngineContext* getEngineContext() const { return engineContext.get(); }
        private:
            AppRuntime() = default;
            const static inline char* DEBUG_PREFIX = "APP_RUNTIME";
            std::unique_ptr<EngineContext> engineContext;
#if !ILEN_ONLY_ECS
            std::shared_ptr<lynx::ecs::systems::ScriptingSystem> scriptingSystemPtr = nullptr;
#endif
    };
}