#pragma once
#include <ilen/ecs/system.h>
#include <ilen/core/lifetime.h>
#include <ilen/render/gpu_interfaces/parallel_command_encoder.h>
#include <ilen/render/gpu_interfaces/instance_buffer.h>
#include <ilen/maths/matrix.h>
#include <ilen/transform/transform_component.h>

namespace ilen {
    struct EngineContext;
}

namespace ilen::render {
    class IRenderer;
    class IGPURenderPipeline;
    struct IlenRenderPipelineDescriptor;
    struct IlenParallelEncoderDescriptor;
    class IGPURenderPipeline;
}

namespace ilen::resources {
    namespace shaders {
        class ShaderManager;
    }
    namespace mesh {
        class MeshManager;
    }
}

namespace  ilen::ecs::systems {
    class PostProcessSystem final: public ISystem {
    public:
        void Init(const EngineContext * engineContext) override;
        void Update() override;
    private:
        static std::vector<render::IlenRenderPipelineDescriptor> GenerateNewPSO(); //Retorna los datos para generar el PSO de calculo y de dibujo final.
        render::IRenderer* renderer = nullptr;
        systems::RenderDrawSystem* drawSystem = nullptr;
        resources::shaders::ShaderManager* shaderManager = nullptr;
        resources::mesh::MeshManager* meshManager = nullptr;
        CameraSystem* cameraSystem = nullptr;

        std::pair<render::IlenParallelEncoderDescriptor, std::shared_ptr<render::IGPURenderPipeline>> encoderToPostProcessPSO; //Este PSO es el que se ejecuta por cada objeto y su objetivo es poblar con datos las texturas
        std::shared_ptr<render::IGPURenderPipeline> postProcessPresentPSO; //Este PSO es el que se usa para dibujar el present en pantalla al final.
        //std::weak_ptr<components::MeshGPULink> quadGPUMesh; // Por optimización, usamos directamente el mesh que tenemos definido, evitando tener 2 buffers idénticos en la GPU.
        std::vector<std::unique_ptr<render::IGPUInstanceBuffer>> postProcessInstanceBuffer;

        //std::unordered_map<Entity, maths::matrix::mat4> previousTransforms;
        //std::mutex previousTransformMutex;
        std::vector<std::pair<Entity, maths::matrix::mat4>> previousTransforms;

        const char* DEBUG_PREFIX = "POST_PROCESSING_SYSTEM";
    };
}

REGISTER_NATIVE_SYSTEM(ilen::ecs::systems::PostProcessSystem, ilen::ecs::SystemPhase::Render, 2);
REQUIRED_COMPONENTS_FOR_SYSTEM(ilen::ecs::systems::PostProcessSystem, ilen::ecs::components::RenderComponent);