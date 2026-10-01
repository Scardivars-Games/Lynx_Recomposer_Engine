#pragma once

#include "ilen/graphics/color.h"
#if ILEN_DEBUG
#include <ilen/debug/debug_draw_2d.h>
#include <ilen/ecs/system.h>
#include <ilen/ecs/system_registry.h>
#include <ilen/graphics/meshes.h>
#include <ilen/graphics/texture.h>
#include <ilen/physics/components/physics_components.h>
#include <ilen/render/gpu_interfaces/buffers.h>
#include <ilen/render/gpu_interfaces/render_pipelines.h>

namespace ilen::resources::texture {
    class TextureManager;
}

namespace ilen::ecs::systems {
    using namespace ilen::debug;
    class CameraSystem;

    class DebugRenderSystem2D final : public ISystem {
        public:
            void Init(const EngineContext* engineContext) override;
            void Update() override;

            struct Settings {
                bool showColliders = true;
                DebugShapeMode2D colliderMode = DebugShapeMode2D::Wireframe;

                float outlineThickness = 0.025f;

                float fillOpacity = 0.28;
                float fillDarkness = 0.55f;
                float wireBrightness = 0.35f;
                
                bool showTriangulation = true;

                graphics::Color dynamicColor {
                    0.0f, 1.0f, 0.0f, 1.0f
                };

                graphics::Color staticColor {
                    0.0f, 0.5f, 1.0f, 1.0f
                };

                graphics::Color triggerColor {
                    1.0f, 0.4f, 1.0f, 1.0f
                };

                graphics::Color collidingColor {
                    1.0f, 0.15f, 0.1f, 1.0f
                };
            };

            static Settings& GetSettings() {
                static Settings settings;
                return settings;
            }

            private:
                struct FrameBuffers {
                    std::shared_ptr<render::IGPUBuffer> vertices;
                    std::shared_ptr<render::IGPUBuffer> indices;
                };

                static constexpr size_t MaxVertices = 65536;
                static constexpr size_t MaxIndices = 98304;

                /*void AppendSolidBox(
                    maths::vector::vec2 center,
                    maths::vector::vec2 halfExtents,
                    float z,
                    const graphics::Color& color,
                    float thickness
                );

                void AppendWireBox(
                    maths::vector::vec2 center,
                    maths::vector::vec2 halfExtents,
                    float z,
                    const graphics::Color& color,
                    float thickness
                );*/

                void AppendLine(
                    maths::vector::vec2 start,
                    maths::vector::vec2 end,
                    float z,
                    const graphics::Color& color,
                    float thickness
                );

                void AppendQuad(
                    maths::vector::vec2 p0,
                    maths::vector::vec2 p1,
                    maths::vector::vec2 p2,
                    maths::vector::vec2 p3,
                    float z,
                    const graphics::Color& color
                );

                void AppendSolidPolygon(
                    const std::vector<maths::vector::vec2>& vertices,
                    float z,
                    const graphics::Color& color,
                    float thickness
                );

                void AppendWirePolygon(
                    const std::vector<maths::vector::vec2>& vertioces,
                    float z,
                    const graphics::Color& color,
                    float thickness
                );

                void AppendTriangle(
                    maths::vector::vec2 p0,
                    maths::vector::vec2 p1,
                    maths::vector::vec2 p2,
                    float z,
                    const graphics::Color& color
                );

                static graphics::Color MakeFillColor(
                    const graphics::Color& color,
                    const Settings& settings
                );

                static graphics::Color MakeWireColor(
                    const graphics::Color& color,
                    const Settings& settings
                );

                render::IRenderer* m_renderer = nullptr;
                CameraSystem* m_cameraSystem = nullptr;
                ilen::resources::texture::TextureManager* m_textureManager = nullptr;

                std::shared_ptr<render::IGPURenderPipeline> m_pipeline;
                std::weak_ptr<graphics::TextureGPULink> m_whiteTexture;

                std::vector<FrameBuffers> m_frameBuffers;

                std::vector<render::buffers::VertexBufferData> m_vertices;
                std::vector<uint32_t> m_indices;
    };
}

REGISTER_NATIVE_SYSTEM(ilen::ecs::systems::DebugRenderSystem2D, ilen::ecs::SystemPhase::Render, 2)
REQUIRED_COMPONENTS_FOR_SYSTEM(ilen::ecs::systems::DebugRenderSystem2D, ilen::ecs::components::physics::BoxCollider2D)

#endif