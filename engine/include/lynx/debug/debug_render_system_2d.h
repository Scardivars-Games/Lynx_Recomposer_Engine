#pragma once

#include <cstddef>
#include <lynx/graphics/color.h>
#if LYNX_DEBUG
#include <lynx/debug/debug_draw_2d.h>
#include <lynx/ecs/system.h>
#include <lynx/ecs/system_registry.h>
#include <lynx/graphics/meshes.h>
#include <lynx/graphics/texture.h>
#include <lynx/physics/components/physics_components.h>
#include <lynx/render/gpu_interfaces/buffers.h>
#include <lynx/render/gpu_interfaces/render_pipelines.h>
#include <lynx/core/runtime_configuration.h>

namespace lynx::resources::texture {
    class TextureManager;
}

namespace lynx::ecs::systems {
    using namespace lynx::debug;
    class CameraSystem;
    class PhysicsSystem2D;

    class DebugRenderSystem2D final : public ISystem {
        public:
            void Init(const EngineContext* engineContext) override;
            void Update() override;

            struct Settings {
                bool showColliders = false;
                DebugShapeMode2D colliderMode = DebugShapeMode2D::Wireframe;

                float outlineThickness = 0.025f;

                float fillOpacity = 0.28;
                float fillDarkness = 0.55f;
                float wireBrightness = 0.35f;
                
                bool showTriangulation = true;
                bool showContactPoints = true;
                bool showContactNormals = true;
                bool showImpactForces = true;
                bool showActiveVectors = true;

                float contactPointSize = 0.08f;
                float contactNormalLength = 0.45f;
                float impactForceScale = 0.001f;
                float impactLineThickness = 0.035f;
                float impactArrowLength = 0.16f;
                float activeVelocityScale = 0.15f;
                float activeForceScale = 0.001f;
                float activeVectorThickness = 0.035f;
                float activeVectorArrowLength = 0.16f;

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

                graphics::Color contactPointColor {
                    1.0f, 0.9f, 0.0f, 1.0f
                };

                graphics::Color contactNormalColor {
                    0.0f, 1.0f, 1.0f, 1.0f
                };

                graphics::Color impactForceColor {
                    1.0f, 0.9f, 0.0f, 1.0f
                };

                graphics::Color activeVelocityColor {
                    0.1f, 1.0f, 0.15f, 1.0f
                };

                graphics::Color activeForceColor {
                    0.35f, 1.0f, 0.0f, 1.0f
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

                lynx::core::RuntimeConfiguration* runtimeOptions = nullptr;
                render::IRenderer* m_renderer = nullptr;
                CameraSystem* m_cameraSystem = nullptr;
                PhysicsSystem2D* m_physicsSystem = nullptr;
                lynx::resources::texture::TextureManager* m_textureManager = nullptr;

                std::shared_ptr<render::IGPURenderPipeline> m_pipeline;
                std::weak_ptr<graphics::TextureGPULink> m_whiteTexture;

                std::vector<FrameBuffers> m_frameBuffers;

                std::vector<render::buffers::VertexBufferData> m_vertices;
                std::vector<uint32_t> m_indices;
    };
}

REGISTER_NATIVE_SYSTEM(lynx::ecs::systems::DebugRenderSystem2D, lynx::ecs::SystemPhase::Render, 2)
REQUIRED_COMPONENTS_FOR_SYSTEM(lynx::ecs::systems::DebugRenderSystem2D, lynx::ecs::components::physics::Rigidbody2D)

#endif
