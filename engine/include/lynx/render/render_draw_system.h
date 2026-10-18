#pragma once

#include <lynx/ecs/system.h>
#include <lynx/render/gpu_interfaces/render_pipelines.h>
#include <lynx/job_system/job_system.h>
#include <lynx/ecs/system_registry.h>

namespace lynx {
    struct EngineContext;
}

namespace lynx::resources {
    namespace materials {
        class MaterialManager;
    }
    namespace mesh {
        class MeshManager;
    }
}

namespace lynx::render {
    class IGPURenderPipeline;
}

namespace lynx::ecs {
    class ECSCoordinator;
}

namespace lynx::ecs::components {
    struct RenderItem;
    struct RenderComponent;
}

namespace lynx::ecs::systems {
    class CameraSystem;

    using namespace lynx::job_system;
    using namespace lynx::resources::materials;
    using namespace lynx::resources::mesh;
    using namespace lynx::render;
    //using namespace lynx::resources::render_pipeline;


    class RenderDrawSystem final: public lynx::ecs::ISystem {
        public:
            void Init(const lynx::EngineContext* engineContext) override;
            void Update() override;
            const std::vector<components::RenderItem>& getLastUsedRenderItemList() const { return lastRenderItems; }
        private:
            static uint64_t GenerateSortKey(const std::shared_ptr<IGPURenderPipeline> &pso, float depth, uint16_t materialRuntimeID, uint16_t textureRuntimeID, uint8_t drawIndex, uint16_t meshID);

            lynx::render::IRenderer* p_Renderer = nullptr;
            CameraSystem* p_cameraSystem = nullptr;
            MaterialManager* p_matManager = nullptr;
            MeshManager* p_meshManager = nullptr;

            std::vector<components::RenderItem> lastRenderItems;

            uint32_t m_minInstanceRenderingElements = 5; //Si un elemento se repite más de 5 veces se realiza Instance Rendering.

            static inline const char* DEBUG_PREFIX = "RENDER_DRAW_SYSTEM";
    };
}
