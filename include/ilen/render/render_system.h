#pragma once

#include <ilen/resources/assets_manager.h>
#include <ilen/resources/materials_manager.h>
#include <ilen/resources/texture_manager.h>
#include <ilen/ecs/types.h>
#include <ilen/headers/enumerations.h>
#include <cstddef>
#include <ilen/render/render_component.h>
#include <ilen/ecs/system.h>
#include <ilen/ecs/ecs.h>
#include <vector>
#include <ilen/resources/mesh_manager.h>
#include <ilen/render/gpu_interfaces/render_pipelines.h>

namespace ilen::resources {
    using namespace ilen::enumerations;

    template <ILEN_ASSET_TYPE T>
    struct AssetElement;

    using ModelAsset = AssetElement<ILEN_ASSET_TYPE_MODEL>;
}

namespace ilen::ecs::systems {
    using namespace ilen::resources::texture;
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
            MeshManager* p_meshManager = nullptr;
            PipelineManager* p_renderPipelineManager = nullptr;
            MaterialManager* p_materialManager = nullptr;
            TextureManager* p_textureManager = nullptr;

            //Métodos privados
            inline void SetVisibilityValue(const RenderComponent& render, const Entity& entity, ComponentArray<VisibilityComponent>* componentArray, bool newValue);
            inline void CheckMeshStatus(const RenderComponent& render, const Entity& entity, ComponentArray<StaticRenderingData>* componentArray) const;
            inline void CheckRenderPipelineStatus(const RenderComponent& render, const Entity& entity, ComponentArray<StaticMaterialRenderingData>* componentArray) const;
            inline void CheckTextureStatus(const RenderComponent& render, const Entity& entity, ComponentArray<StaticTextureRenderingData>* componentArray) const;

            const char* DEBUG_PREFIX = "RENDER_SYSTEM";
    };
}

REGISTER_NATIVE_SYSTEM(ilen::ecs::systems::RenderSystem, ilen::ecs::SystemPhase::PreRender, 3)
REQUIRED_COMPONENTS_FOR_SYSTEM(ilen::ecs::systems::RenderSystem, ilen::ecs::components::RenderComponent)