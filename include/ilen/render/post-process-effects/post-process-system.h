#pragma once
#include <ilen/ecs/system.h>
#include <ilen/core/lifetime.h>
#include <ilen/render/gpu-interfaces/parallel-command-encoder.h>
#include <ilen/maths/matrix.h>
#include <ilen/transform/transform-component.h>

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
    class PostProcessSystem final: public System {
    public:
        void Init(const EngineContext * engineContext);
        void Update();
    private:
        static std::vector<IlenRenderPipelineDescriptor> GenerateNewPSO(); //Retorna los datos para generar el PSO de calculo y de dibujo final.
        ECSCoordinator* coordinator = nullptr;
        job_system::JobSystem* jobSystem = nullptr;
        render::IRenderer* renderer = nullptr;
        systems::RenderDrawSystem* drawSystem = nullptr;
        resources::shaders::ShaderManager* shaderManager = nullptr;
        resources::mesh::MeshManager* meshManager = nullptr;
        CameraSystem* cameraSystem = nullptr;

        std::pair<render::IlenParallelEncoderDescriptor, std::shared_ptr<render::IGPURenderPipeline>> encoderToPostProcessPSO; //Este PSO es el que se ejecuta por cada objeto y su objetivo es poblar con datos las texturas
        std::shared_ptr<render::IGPURenderPipeline> postProcessPresentPSO; //Este PSO es el que se usa para dibujar el present en pantalla al final.
        std::weak_ptr<MeshGPULink> quadGPUMesh; // Por optimización, usamos directamente el mesh que tenemos definido, evitando tener 2 buffers idénticos en la GPU.
        std::vector<std::unique_ptr<IGPUInstanceBuffer>> postProcessInstanceBuffer;

        std::unordered_map<Entity, components::TransformComponent> previousTransforms;

        const char* DEBUG_PREFIX = "POST_PROCESSING_SYSTEM";
    };
}
