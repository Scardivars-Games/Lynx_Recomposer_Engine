#pragma once
#include <lynx/ecs/system.h>
#include <lynx/core/lifetime.h>
#include <lynx/render/gpu_interfaces/parallel_command_encoder.h>
#include <lynx/render/gpu_interfaces/instance_buffer.h>
#include <lynx/maths/matrix.h>
#include <lynx/transform/transform_component.h>
#include "lynx/core/runtime_configuration.h"

namespace lynx {
    struct EngineContext;
}

namespace lynx::render {
    class IRenderer;
    class IGPURenderPipeline;
    struct LynxRenderPipelineDescriptor;
    struct LynxParallelEncoderDescriptor;
    class IGPURenderPipeline;
}

namespace lynx::resources {
    namespace shaders {
        class ShaderManager;
    }
    namespace mesh {
        class MeshManager;
    }
}

namespace  lynx::ecs::systems {
    class PostProcessSystem final: public ISystem {
    public:
        void Init(const EngineContext * engineContext) override;
        void Update() override;
    private:
        static std::vector<render::LynxRenderPipelineDescriptor> GenerateNewPSO(); //Retorna los datos para generar el PSO de calculo y de dibujo final.
        render::IRenderer* renderer = nullptr;
        systems::RenderDrawSystem* drawSystem = nullptr;
        resources::shaders::ShaderManager* shaderManager = nullptr;
        resources::mesh::MeshManager* meshManager = nullptr;
        CameraSystem* cameraSystem = nullptr;
        lynx::core::RuntimeConfiguration* runtimeConfiguration = nullptr;

        std::pair<render::LynxParallelEncoderDescriptor, std::shared_ptr<render::IGPURenderPipeline>> encoderToPostProcessPSO; //Este PSO es el que se ejecuta por cada objeto y su objetivo es poblar con datos las texturas
        std::shared_ptr<render::IGPURenderPipeline> postProcessPresentPSO; //Este PSO es el que se usa para dibujar el present en pantalla al final.
        //std::weak_ptr<components::MeshGPULink> quadGPUMesh; // Por optimización, usamos directamente el mesh que tenemos definido, evitando tener 2 buffers idénticos en la GPU.
        std::vector<std::unique_ptr<render::IGPUInstanceBuffer>> postProcessInstanceBuffer;

        //std::unordered_map<Entity, maths::matrix::mat4> previousTransforms;
        //std::mutex previousTransformMutex;
        std::vector<std::pair<Entity, maths::matrix::mat4>> previousTransforms;

        const char* DEBUG_PREFIX = "POST_PROCESSING_SYSTEM";
    };
}
