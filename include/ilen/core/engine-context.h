#pragma once
//Esta cabecera define el contexto del motor.
#include <memory>
#include <ilen/job-system/job-system.h>
#include <ilen/job-system/command-buffers.h>

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
    namespace texture {
        class TextureManager;
    }
}

namespace ilen::window{
    class IWindow;
}

namespace ilen::render{
    class IRenderer;
}

namespace ilen::ecs{
    class ECSCoordinator;
}

namespace ilen {
    //Almacena todos los punteros necesarios para el funcionamiento correcto del motor. Se debe crear al lado de los metadatos del motor
    struct EngineContext{
        std::unique_ptr<ilen::job_system::JobSystem> p_jobSystem;
        std::unique_ptr<window::IWindow> p_gameWindow;
        std::unique_ptr<render::IRenderer> p_gameRenderer;
        std::unique_ptr<ilen::ecs::ECSCoordinator> p_coordinator;
        std::unique_ptr<ilen::job_system::CommandBufferDispatcher> p_commandDispatcher;

        //Resources Managers
        std::unique_ptr<ilen::resources::AssetsManager> p_assetsManager;
        std::unique_ptr<ilen::resources::mesh::MeshManager> p_meshManager;
        std::unique_ptr<ilen::resources::shaders::ShaderManager> p_shaderManager;
        std::unique_ptr<ilen::resources::materials::MaterialManager> p_materialManager;
        std::unique_ptr<ilen::resources::render_pipeline::PipelineManager> p_renderPipelineManager;
        std::unique_ptr<ilen::resources::texture::TextureManager> p_textureManager;


        //Ahora es responsabilidad del coordinador entregar los sistemas :)
        /*//ECS Systems (shared ptrs)
        std::shared_ptr<ilen::ecs::systems::RenderSystem> p_renderSystem;
        std::shared_ptr<ilen::ecs::systems::RenderDrawSystem> p_renderDrawSystem;
        std::shared_ptr<ilen::ecs::systems::TransformSystem> p_transformSystem;
        std::shared_ptr<ilen::ecs::systems::ScriptingSystem> p_scriptingSystem;
        std::shared_ptr<ilen::ecs::systems::CameraSystem> p_cameraSystem;
        std::shared_ptr<ilen::ecs::systems::PostProcessSystem> p_postProcessSystem; //Al final es sistema, porque usa shaders para calcular los nuevos buffers.*/
    };
}