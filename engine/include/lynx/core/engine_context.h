#pragma once
//Esta cabecera define el contexto del motor.
#include <memory>
#include <lynx/job_system/job_system.h>
#include <lynx/job_system/command_buffers.h>
#include <lynx/core/runtime_configuration.h>

namespace lynx::resources {
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
    namespace scene {
        class SceneManager;
    }
}

namespace lynx::window{
    class IWindow;
}

namespace lynx::render{
    class IRenderer;
}

namespace lynx::ecs{
    class ECSCoordinator;
}

namespace lynx {
    //Almacena todos los punteros necesarios para el funcionamiento correcto del motor. Se debe crear al lado de los metadatos del motor
    struct EngineContext{
        //Runtime options
        std::unique_ptr<lynx::core::RuntimeConfiguration> runtimeConfiguration;

        //Punteros claves
        std::unique_ptr<lynx::job_system::JobSystem> p_jobSystem;
        std::unique_ptr<window::IWindow> p_gameWindow;
        std::unique_ptr<render::IRenderer> p_gameRenderer;
        std::unique_ptr<lynx::ecs::ECSCoordinator> p_coordinator;
        std::unique_ptr<lynx::job_system::CommandBufferDispatcher> p_commandDispatcher;

        //Resources Managers
        std::unique_ptr<lynx::resources::AssetsManager> p_assetsManager;
        std::unique_ptr<lynx::resources::mesh::MeshManager> p_meshManager;
        std::unique_ptr<lynx::resources::shaders::ShaderManager> p_shaderManager;
        std::unique_ptr<lynx::resources::materials::MaterialManager> p_materialManager;
        std::unique_ptr<lynx::resources::render_pipeline::PipelineManager> p_renderPipelineManager;
        std::unique_ptr<lynx::resources::texture::TextureManager> p_textureManager;
        std::unique_ptr<lynx::resources::scene::SceneManager> p_sceneManager;


        //Ahora es responsabilidad del coordinador entregar los sistemas :)
        /*//ECS Systems (shared ptrs)
        std::shared_ptr<lynx::ecs::systems::RenderSystem> p_renderSystem;
        std::shared_ptr<lynx::ecs::systems::RenderDrawSystem> p_renderDrawSystem;
        std::shared_ptr<lynx::ecs::systems::TransformSystem> p_transformSystem;
        std::shared_ptr<lynx::ecs::systems::ScriptingSystem> p_scriptingSystem;
        std::shared_ptr<lynx::ecs::systems::CameraSystem> p_cameraSystem;
        std::shared_ptr<lynx::ecs::systems::PostProcessSystem> p_postProcessSystem; //Al final es sistema, porque usa shaders para calcular los nuevos buffers.*/
    };
}