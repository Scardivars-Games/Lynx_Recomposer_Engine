#pragma once

#include <ilen/ecs/system.h>
#include <ilen/core/lifetime.h>
#include <ilen/render/gpu-interfaces/render-pipelines.h>
#include <ilen/job-system/job-system.h>

namespace ilen::ecs::systems {
    using namespace ilen::resources::render_pipeline;
    using namespace ilen::job_system;
    using namespace ilen::resources::materials;
    using namespace ilen::resources::mesh;

    class RenderDrawSystem: public ilen::ecs::System {
        public:
            void Init(const EngineContext* engineContext);
            void Update();

        private:
            static uint64_t GenerateSortKey(const std::shared_ptr<IGPURenderPipeline> &pso, float depth, uint16_t materialRuntimeID, uint8_t drawIndex, uint16_t meshID);

            ECSCoordinator* p_Coordinator = nullptr;
            JobSystem* p_JobSystem = nullptr;
            IRenderer* p_Renderer = nullptr;
            CameraSystem* p_cameraSystem = nullptr;
            MaterialManager* p_matManager = nullptr;
            MeshManager* p_meshManager = nullptr;
    };
}
