#pragma once

#include <ilen/resources/assets-manager.h>
#include <ilen/resources/materials-manager.h>
#include <ilen/ecs/types.h>
#include <ilen/headers/enumerations.h>
#include <cstddef>
#include <ilen/render/render-component.h>
#include <ilen/ecs/system.h>
#include <ilen/ecs/ecs.h>
#include <vector>
#include <ilen/resources/mesh-manager.h>
#include <ilen/render/gpu-interfaces/render-pipelines.h>

namespace ilen::resources {
    using namespace ilen::enumerations;

    template <ILEN_ASSET_TYPE T>
    struct AssetElement;

    using ModelAsset = AssetElement<ILEN_ASSET_TYPE_MODEL>;
}

namespace ilen::ecs::systems {

    using namespace ilen::resources::mesh;
    using namespace ilen::resources::materials;
    using namespace ilen::resources::render_pipeline;
    using namespace ilen::ecs::components; 
    using namespace ilen::job_system;    
    using namespace ilen::core;    

    class RenderSystem final: public ilen::ecs::ISystem {
        public:
            void Init(const EngineContext* engineContext) override;
            void Update() override;
        private:
            ECSCoordinator* p_Coordinator = nullptr;
            JobSystem* p_jobSystemInstance = nullptr;
            MeshManager* p_meshManager = nullptr;
            PipelineManager* p_renderPipelineManager = nullptr;
            MaterialManager* p_materialManager = nullptr;

            //Métodos privados
            void HaveVisibilityDefined(const RenderComponent& render, const Entity& entity);
            void CheckMeshStatus(const RenderComponent& render, const Entity& entity) const;
            void CheckRenderPipelineStatus(const RenderComponent& render, const Entity& entity) const;

            const char* DEBUG_PREFIX = "RENDER_SYSTEM";
    };
}

REGISTER_NATIVE_SYSTEM(ilen::ecs::systems::RenderSystem, ilen::ecs::SystemPhase::PreRender, 0)
REQUIRED_COMPONENTS_FOR_SYSTEM(ilen::ecs::systems::RenderSystem, ilen::ecs::components::RenderComponent)