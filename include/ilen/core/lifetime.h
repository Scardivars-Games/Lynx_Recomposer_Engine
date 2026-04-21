//PUNTO DE ENTRADA DEL MOTOR. ESTE ENCABEZADO CONTINE LA LóGICA DE TODO EL MOTOR. AQUI SE INICIALIZA DESDE EL ENTRY POINT DEL OS.
#pragma once
#include <ilen/headers/visual.h>
#include <ilen/headers/core.h>
#include <ilen/job-system/job-system.h>
#include <ilen/resources/shaders-manager.h>
#include <ilen/resources/materials-manager.h>
#include <ilen/resources/render-pipeline-manager.h>
#include <ilen/camera/camera-system.h>
#include <memory>


namespace ilen::ecs::systems {
    class RenderSystem;
    class TransformSystem;
    class ScriptingSystem;
    class CameraSystem;
    class RenderDrawSystem;
    class PostProcessSystem;
}


namespace ilen {
    class AppRuntime final{
        public:
            static AppRuntime& getInstance();
            void init(const application::ApplicationMetadata &metadata); //Inicia el motor
            void update() const; //Ejecuta el loop del motor
            void shutdown() const; //Detiene el motor
            bool isRunning() const;
            EngineContext* getEngineContext() const { return engineContext.get(); }
        private:
            AppRuntime() = default;
            const static inline char* DEBUG_PREFIX = "APP_RUNTIME";
            std::unique_ptr<EngineContext> engineContext;
#if !ILEN_ONLY_ECS
            std::shared_ptr<ilen::ecs::systems::ScriptingSystem> scriptingSystemPtr = nullptr;
#endif
    };
}