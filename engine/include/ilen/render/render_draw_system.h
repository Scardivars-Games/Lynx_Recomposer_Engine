#pragma once

#include <ilen/ecs/system.h>
#include <ilen/render/gpu_interfaces/render_pipelines.h>
#include <ilen/job_system/job_system.h>
#include <ilen/ecs/system_registry.h>

namespace ilen {
    struct EngineContext;
}

namespace ilen::resources {
    namespace materials {
        class MaterialManager;
    }
    namespace mesh {
        class MeshManager;
    }
}

namespace ilen::render {
    class IGPURenderPipeline;
}

namespace ilen::ecs {
    class ECSCoordinator;
}

namespace ilen::ecs::components {
    struct RenderItem;
    struct RenderComponent;
}

namespace ilen::ecs::systems {
    class CameraSystem;

    using namespace ilen::job_system;
    using namespace ilen::resources::materials;
    using namespace ilen::resources::mesh;
    using namespace ilen::render;
    //using namespace ilen::resources::render_pipeline;


    class RenderDrawSystem final: public ilen::ecs::ISystem {
        public:
            void Init(const ilen::EngineContext* engineContext) override;
            void Update() override;
            const std::vector<components::RenderItem>& getLastUsedRenderItemList() const { return lastRenderItems; }
        private:
            static uint64_t GenerateSortKey(const std::shared_ptr<IGPURenderPipeline> &pso, float depth, uint16_t materialRuntimeID, uint16_t textureRuntimeID, uint8_t drawIndex, uint16_t meshID);

            ilen::render::IRenderer* p_Renderer = nullptr;
            CameraSystem* p_cameraSystem = nullptr;
            MaterialManager* p_matManager = nullptr;
            MeshManager* p_meshManager = nullptr;

            std::vector<components::RenderItem> lastRenderItems;

            uint32_t m_minInstanceRenderingElements = 5; //Si un elemento se repite más de 5 veces se realiza Instance Rendering.

            static inline const char* DEBUG_PREFIX = "RENDER_DRAW_SYSTEM";
    };
}

REGISTER_NATIVE_SYSTEM(ilen::ecs::systems::RenderDrawSystem, ilen::ecs::SystemPhase::Render, 1)
REQUIRED_COMPONENTS_FOR_SYSTEM(ilen::ecs::systems::RenderDrawSystem, ilen::ecs::components::RenderComponent)