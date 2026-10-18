#pragma once

#include <lynx/resources/assets_manager.h>
#include <lynx/resources/materials_manager.h>
#include <lynx/resources/texture_manager.h>
#include <lynx/ecs/types.h>
#include <lynx/headers/enumerations.h>
#include <cstddef>
#include <lynx/render/render_component.h>
#include <lynx/ecs/system.h>
#include <lynx/ecs/ecs.h>
#include <vector>
#include <lynx/resources/mesh_manager.h>
#include <lynx/render/gpu_interfaces/render_pipelines.h>
#include "lynx/core/runtime_configuration.h"

namespace lynx::resources {
    using namespace lynx::enumerations;

    template <LYNX_ASSET_TYPE T>
    struct AssetElement;

    using ModelAsset = AssetElement<LYNX_ASSET_TYPE_MODEL>;
}

namespace lynx::ecs::systems {
    using namespace lynx::resources::texture;
    using namespace lynx::resources::mesh;
    using namespace lynx::resources::materials;
    using namespace lynx::resources::render_pipeline;
    using namespace lynx::ecs::components; 
    using namespace lynx::job_system;    
    using namespace lynx::core;    

    class RenderSystem final: public lynx::ecs::ISystem {
        public:
            void Init(const EngineContext* engineContext) override;
            void Update() override;
        private:
            MeshManager* p_meshManager = nullptr;
            PipelineManager* p_renderPipelineManager = nullptr;
            MaterialManager* p_materialManager = nullptr;
            TextureManager* p_textureManager = nullptr;
            PredefinedAssetsID m_predefinedAssets{};
            [[nodiscard]] static lynx::resources::AssetID ResolveAssetID(lynx::resources::AssetID requested, lynx::resources::AssetID fallback) noexcept {
                return requested == lynx::resources::InvalidAssetID ? fallback : requested;
            }

            //Métodos privados
            inline void SetVisibilityValue(const RenderComponent& render, const Entity& entity, ComponentArray<VisibilityComponent>* componentArray, bool newValue);
            inline void CheckMeshStatus(const RenderComponent& render, const Entity& entity, ComponentArray<StaticRenderingData>* componentArray) const;
            inline void CheckRenderPipelineStatus(const RenderComponent& render, const Entity& entity, ComponentArray<StaticMaterialRenderingData>* componentArray) const;
            inline void CheckTextureStatus(const RenderComponent& render, const Entity& entity, ComponentArray<StaticTextureRenderingData>* componentArray) const;

            const char* DEBUG_PREFIX = "RENDER_SYSTEM";
    };
}