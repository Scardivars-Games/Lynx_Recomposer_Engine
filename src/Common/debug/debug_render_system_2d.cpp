#include <cstdint>
#include <cstdlib>
#include <ilen/debug/debug_render_system_2d.h>
#include "ilen/debug/debug_draw_2d.h"
#include "ilen/ecs/types.h"
#include "ilen/graphics/color.h"
#include "ilen/graphics/meshes.h"
#include "ilen/physics/components/specification/collider_components.h"

#if ILEN_DEBUG

#include <ilen/camera/camera_system.h>
#include <ilen/core/engine_context.h>
#include <ilen/core/runtime_options.h>
#include <ilen/core/time.h>
#include <ilen/ecs/ecs.h>
#include <ilen/physics/components/physics_components.h>
#include <ilen/physics/physics_2d_system.h>
#include <ilen/render/render_constants.h>
#include <ilen/resources/materials_manager.h>
#include <ilen/resources/texture_manager.h>
#include <ilen/transform/transform_component.h>
#include <ilen/physics/collider_proxy_builder_2d.h>

#include <cmath>
#include <algorithm>

namespace ilen::ecs::systems {
    using namespace ilen::maths::vector;
    using namespace ilen::ecs::components;
    using namespace ilen::ecs::components::physics;
    using namespace ilen::render;
    using namespace ilen::core::options;
    using namespace ilen::debug;

    void DebugRenderSystem2D::Init(const EngineContext* context) {
        p_coordinator = context->p_coordinator.get();
        m_renderer = context->p_gameRenderer.get();
        m_cameraSystem = p_coordinator->getSystem<CameraSystem>().get();
        m_physicsSystem = p_coordinator->getSystem<PhysicsSystem2D>().get();
        m_textureManager = context->p_textureManager.get();

        m_vertices.reserve(MaxVertices);
        m_indices.reserve(MaxIndices);

        //Reutilizar shader Unlit transparente para el Debug
        const auto& material = context->p_materialManager->getMaterial(
            PredefinedAssetsID::DefaultUnlitTransparentMatID
        );

        IlenRenderPipelineDescriptor descriptor(material);
        descriptor.pipelineName = "Debug Draw 2D";
        descriptor.renderQueue = ILEN_GPU_RENDER_QUEUE_TYPE::ILEN_GPU_RENDER_QUEUE_OVERLAY;

        // El debug se codifica dentro del render pass principal. Si ese pass
        // contiene un attachment de profundidad, Metal exige que el pipeline
        // declare el mismo pixelFormat aunque no quiera comprobar ni escribir
        // depth. CompareAlways conserva el comportamiento de overlay.
        descriptor.depthCompareFunction =
            ILEN_GPU_COMPARE_FUNCTION::ILEN_GPU_COMPARE_ALWAYS;

        descriptor.renderPipelineOptionFlags = 
            ILEN_RENDER_PIPELINES_OPTIONS_DONT_USES_STENCIL_BUFFER |
            ILEN_RENDER_PIPELINE_OPTIONS_DONT_WRITE_DEPTH_BUFFER |
            ILEN_RENDER_PIPELINE_OPTIONS_DONT_CULL_ENABLED;

        m_pipeline = m_renderer->createRenderPipeline(descriptor, context->p_shaderManager.get());
        
        m_whiteTexture = context->p_textureManager->getTextureHandler(PredefinedAssetsID::DefaultTextureID);

        const uint8_t framesInFlight = m_renderer->getActualNumOfFramesInFlight();

        m_frameBuffers.resize(framesInFlight);

        for(uint8_t i = 0; i < framesInFlight; i++) {
            IlenBufferDescriptor vertexDescriptor{};
            vertexDescriptor.bufferType = ILEN_RENDER_GPU_BUFFER_VERTEX;
            vertexDescriptor.bufferSize = MaxVertices * sizeof(render::buffers::VertexBufferData);
            vertexDescriptor.count = MaxVertices;
            vertexDescriptor.stride = sizeof(render::buffers::VertexBufferData);
            vertexDescriptor.bufferLabelName = "Debug 2D Vertex Buffer " + std::to_string(i);

            m_frameBuffers[i].vertices = m_renderer->createBuffer(vertexDescriptor, nullptr);

            IlenBufferDescriptor indexDescriptor{};
            indexDescriptor.bufferType = ILEN_RENDER_GPU_BUFFER_INDEX;
            indexDescriptor.bufferSize = MaxIndices * sizeof(uint32_t);
            indexDescriptor.count = MaxIndices;
            indexDescriptor.stride = sizeof(uint32_t);
            indexDescriptor.bufferLabelName = "Debug 2D Index Buffer " + std::to_string(i);

            m_frameBuffers[i].indices = m_renderer->createBuffer(indexDescriptor, nullptr);
        }
    }

    void DebugRenderSystem2D::Update() {
        m_vertices.clear();
        m_indices.clear();

        const Settings& setting = GetSettings();

        /* Colliders automáticos.
        *
        * Es importante usar transform.position y no localToWorldMatrix:
        * PhysicsSystem2D construye sus AABB usando exactamente position,
        * scale y collider.offset. El debug debe mostrar lo que física usa,
        * incluso si más adelante descubres un error con padres o rotación.
        */

        if(setting.showColliders) {
            auto* transformArray = p_coordinator->getComponentArray<TransformComponent>();
            auto* boxColliderArray = p_coordinator->getComponentArray<BoxCollider2D>();
            auto* polygonColliderArray =
                p_coordinator->getComponentArray<ConvexPolygonCollider2D>();
            auto* rbArray = p_coordinator->getComponentArray<Rigidbody2D>();
            auto* collisionArray = p_coordinator->getComponentArray<CollisionComponent2D>();
            auto* metadataArray = p_coordinator->getComponentArray<EntityMetadataComponent>();

            for(Entity entity: entities) {
                auto* transform = transformArray->getData(entity);
                auto* boxCollider = boxColliderArray->getData(entity);
                auto* polygonCollider = polygonColliderArray->getData(entity);
                auto* rigidbody = rbArray->getData(entity);
                auto* metadata = metadataArray->getData(entity);

                if(
                    !transform ||
                    !rigidbody ||
                    !metadata ||
                    ((boxCollider == nullptr) == (polygonCollider == nullptr))
                ) {
                    continue;
                }

                std::optional<ilen::physics::ColliderProxy2D> proxy;
                bool isTrigger = false;

                if(boxCollider) {
                    proxy = ilen::physics::ColliderProxyBuilder2D::FromBox(
                        entity,
                        *transform,
                        *rigidbody,
                        *boxCollider,
                        *metadata
                    );
                    isTrigger = boxCollider->isTrigger;
                } else {
                    proxy = ilen::physics::ColliderProxyBuilder2D::FromPolygon(
                        entity,
                        *transform,
                        *rigidbody,
                        *polygonCollider,
                        *metadata
                    );
                    isTrigger = polygonCollider->isTrigger;
                }

                if(!proxy.has_value()) {
                    continue;
                }

                graphics::Color color = setting.dynamicColor;

                if(rigidbody->isStatic) {
                    color = setting.staticColor;
                }

                if(isTrigger) {
                    color = setting.triggerColor;
                }

                auto* collision = collisionArray->getData(entity);
                const bool hasActiveContact = collision && std::any_of(
                    collision->contacts.begin(),
                    collision->contacts.end(),
                    [](const ContactManifold2D& contact) {
                        return contact.phase != CollisionPhase2D::Exit;
                    }
                );

                if(hasActiveContact) {
                    color = setting.collidingColor;
                }

                if(setting.colliderMode == DebugShapeMode2D::Solid){
                    AppendSolidPolygon(
                        proxy->worldVertices,
                        transform->position.z,
                        color,
                        setting.outlineThickness
                    );
                } else {
                    AppendWirePolygon(
                        proxy->worldVertices,
                        transform->position.z,
                        color,
                        setting.outlineThickness
                    );
                }

                if(!collision) {
                    continue;
                }

                for(const auto& contact : collision->contacts) {
                    if(contact.phase == CollisionPhase2D::Exit) {
                        continue;
                    }

                    // El contacto está espejado en ambas entidades. Dibujarlo
                    // una sola vez evita duplicar punto y normal.
                    if(entity > contact.entityB || !contact.hasContactPoint) {
                        continue;
                    }

                    if(setting.showContactPoints) {
                        const vec2 halfPoint{
                            setting.contactPointSize * 0.5f
                        };

                        AppendQuad(
                            contact.point - halfPoint,
                            contact.point + vec2{halfPoint.x, -halfPoint.y},
                            contact.point + halfPoint,
                            contact.point + vec2{-halfPoint.x, halfPoint.y},
                            transform->position.z,
                            setting.contactPointColor
                        );
                    }

                    if(setting.showContactNormals) {
                        AppendLine(
                            contact.point,
                            contact.point +
                                contact.normal * setting.contactNormalLength,
                            transform->position.z,
                            setting.contactNormalColor,
                            setting.outlineThickness
                        );
                    }
                }
            }
        }

        if(
            PhysicsOptions::showDebugArrows &&
            setting.showImpactForces &&
            m_physicsSystem
        ) {
            for(const auto& impact :
                m_physicsSystem->GetPersistentDebugImpacts()
            ) {
                const float directionLength = std::sqrt(
                    impact.direction.x * impact.direction.x +
                    impact.direction.y * impact.direction.y
                );

                if(directionLength <= 0.00001f || impact.force <= 0.0f) {
                    continue;
                }

                const vec2 direction =
                    impact.direction / directionLength;
                const float lineLength =
                    impact.force * setting.impactForceScale;
                const vec2 end = impact.point + direction * lineLength;

                AppendLine(
                    impact.point,
                    end,
                    impact.z,
                    setting.impactForceColor,
                    setting.impactLineThickness
                );

                const float arrowLength = std::min(
                    setting.impactArrowLength,
                    lineLength * 0.35f
                );
                const vec2 perpendicular{-direction.y, direction.x};
                const vec2 arrowBase = end - direction * arrowLength;
                const vec2 arrowOffset =
                    perpendicular * (arrowLength * 0.5f);

                AppendLine(
                    end,
                    arrowBase + arrowOffset,
                    impact.z,
                    setting.impactForceColor,
                    setting.impactLineThickness
                );
                AppendLine(
                    end,
                    arrowBase - arrowOffset,
                    impact.z,
                    setting.impactForceColor,
                    setting.impactLineThickness
                );
            }
        }

        if(
            PhysicsOptions::showDebugArrows &&
            setting.showActiveVectors &&
            m_physicsSystem
        ) {
            for(const auto& activeVector :
                m_physicsSystem->GetDebugActiveVectors()
            ) {
                const float valueLength = std::sqrt(
                    activeVector.value.x * activeVector.value.x +
                    activeVector.value.y * activeVector.value.y
                );

                if(valueLength <= 0.00001f) {
                    continue;
                }

                const bool isForce = activeVector.kind ==
                    PhysicsSystem2D::DebugActiveVectorKind2D::Force;
                const float scale = isForce
                    ? setting.activeForceScale
                    : setting.activeVelocityScale;
                const graphics::Color& color = isForce
                    ? setting.activeForceColor
                    : setting.activeVelocityColor;
                const float lineLength = valueLength * scale;

                if(lineLength <= 0.00001f) {
                    continue;
                }

                const vec2 direction = activeVector.value / valueLength;
                const vec2 end =
                    activeVector.point + direction * lineLength;

                AppendLine(
                    activeVector.point,
                    end,
                    activeVector.z,
                    color,
                    setting.activeVectorThickness
                );

                const float arrowLength = std::min(
                    setting.activeVectorArrowLength,
                    lineLength * 0.35f
                );
                const vec2 perpendicular{-direction.y, direction.x};
                const vec2 arrowBase = end - direction * arrowLength;
                const vec2 arrowOffset =
                    perpendicular * (arrowLength * 0.5f);

                AppendLine(
                    end,
                    arrowBase + arrowOffset,
                    activeVector.z,
                    color,
                    setting.activeVectorThickness
                );
                AppendLine(
                    end,
                    arrowBase - arrowOffset,
                    activeVector.z,
                    color,
                    setting.activeVectorThickness
                );
            }
        }

        auto primitives = DebugDraw2D::Consume (ilen::time::Time::getDeltaTime());

        for(const auto& primitive : primitives) {
            switch (primitive.type) {
                case DebugPrimitiveType2D::Line:
                    AppendLine(
                        primitive.a,
                        primitive.b,
                        primitive.z,
                        primitive.color,
                        primitive.thickness
                    );
                    break;
                case DebugPrimitiveType2D::Box:
                    const vec2 center = primitive.a;
                    const vec2 half = primitive.b;

                    const float left = center.x - std::abs(half.x);
                    const float right = center.x + std::abs(half.x);
                    const float bottom = center.y - std::abs(half.y);
                    const float top = center.y + std::abs(half.y);

                    const std::vector<vec2> vertices {
                        vec2{left, bottom},
                        vec2{right, bottom},
                        vec2{right, top},
                        vec2{left, top}
                    };

                    if(primitive.mode == DebugShapeMode2D::Solid) {
                        AppendSolidPolygon(
                            vertices,
                            primitive.z,
                            primitive.color,
                            primitive.thickness
                        );
                    }
                    break;
            }
        }

        if(m_vertices.empty() || m_indices.empty()) {
            return;
        }

        auto camera = m_cameraSystem->getMainCamera();

        auto texture = m_whiteTexture.lock();

        if(!texture) {
            //Solicitando al manager de textura un nuevo puntero a la textura necesaria por el Debugger.
            m_whiteTexture = m_textureManager->getTextureHandler(PredefinedAssetsID::DefaultTextureID);

            texture = m_whiteTexture.lock();

            if(!texture){
                //Aun no se ha cargado del todo, pero ya esta gestionandose
                return;
            }
        }

        if(!camera || !m_pipeline){
            return;
        }

        //ILEN_LOG_P("DEBUG_RENDER", "Paso la comprobacion");

        const uint8_t frame = m_renderer->getActualFrameInFlight();
        FrameBuffers& buffers = m_frameBuffers[frame];

        buffers.vertices->writeDataToBuffer(
            m_vertices.data(), 
            m_vertices.size() * sizeof(render::buffers::VertexBufferData), 
            0);
        
        buffers.indices->writeDataToBuffer(
            m_indices.data(),
            m_indices.size() * sizeof(uint32_t),
            0);
        
        auto parallelEncoder = m_renderer->getParallelCommandEncoder();

        if(!parallelEncoder) {
            return;
        }

        parallelEncoder->SetGlobalBuffers(
            slots::GlobalBufferSlot_Camera,
            camera->cameraBuffer.get()
        );

        auto* commandEncoder = parallelEncoder->GetThreadEncoder();

        commandEncoder->SetPipelineState(m_pipeline.get());
        commandEncoder->SetVertexBuffer(slots::RenderBufferSlot_VertexMain, buffers.vertices.get());
        commandEncoder->SetIndexBuffer(buffers.indices.get());
        commandEncoder->SetFragmentTexture(0, texture->texture.get());
        commandEncoder->SetFragmentSampler(0, texture->sampler.get());

        //Los vertices ya esta en WorldEspace, por lo cual la matrix identidad es suficiente
        structs::InstanceData instance{};
        instance.modelMatrix = maths::matrix::mat4::identity();
        instance.uvRect = vec4{0.0f, 0.0f, 1.0f, 1.0f};

        auto* instanceBuffer = m_renderer->getInstanceBuffer();
        const uint32_t instanceOffset = instanceBuffer->UploadInstanceData(&instance, sizeof(instance));

        commandEncoder->SetInstancedBuffer(slots::RenderBufferSlot_InstanceModel, instanceBuffer->GetBuffer());
        commandEncoder->DrawIndexedInstanced(m_indices.size(), 1, instanceOffset, sizeof(instance));
        commandEncoder->EndEncoding();
        m_renderer->returnParallelCommandEncoder(std::move(parallelEncoder));
    }

    /*void DebugRenderSystem2D::AppendSolidBox(
        vec2 center,
        vec2 half,
        float z,
        const graphics::Color& color,
        float thickness
    ) {
        const float left = center.x - std::abs(half.x);
        const float right = center.x + std::abs(half.x);
        const float bottom = center.y - std::abs(half.y);
        const float top = center.y + std::abs(half.y);

        AppendQuad(
            vec2{left, bottom},
            vec2{right, bottom},
            vec2{right, top},
            vec2{left, top},
            z,
            color
        );
    }

    void DebugRenderSystem2D::AppendWireBox(
        vec2 center,
        vec2 half,
        float z,
        const graphics::Color& color,
        float thickness
    ) {
        const float left = center.x - std::abs(half.x);
        const float right = center.x + std::abs(half.x);
        const float bottom = center.y - std::abs(half.y);
        const float top = center.y + std::abs(half.y);

        AppendLine(
            vec2{left, bottom},
            vec2{right, bottom},
            z, color, thickness
        );

        AppendLine(
            vec2{right, bottom},
            vec2{right, top},
            z, color, thickness
        );

        AppendLine(
            vec2{right, top},
            vec2{left, top},
            z, color, thickness
        );

        AppendLine(
            vec2{left, top},
            vec2{left, bottom},
            z, color, thickness
        );
    }*/

    void DebugRenderSystem2D::AppendLine(
        vec2 start,
        vec2 end,
        float z,
        const graphics::Color& color,
        float thickness
    ) {
        const vec2 delta = end - start;

        const float lenght = std::sqrt(
            delta.x * delta.x + delta.y * delta.y
        );

        if(lenght <= 0.00001f) {
            return;
        }

        const vec2 normal {
            -delta.y / lenght, delta.x / lenght
        };

        const vec2 offset = normal * (thickness * 0.5f);

        AppendQuad(
            start - offset,
            end - offset,
            end + offset,
            start + offset,
            z,
            color
        );
    }

    void DebugRenderSystem2D::AppendQuad(
        vec2 p0,
        vec2 p1,
        vec2 p2,
        vec2 p3,
        float z,
        const graphics::Color& color
    ) {
        if(m_vertices.size() + 4 > MaxVertices || m_indices.size() + 6 > MaxIndices) {
             return;
        }

        const uint32_t base = static_cast<uint32_t>(m_vertices.size());

        auto makeVertex = [z, &color](vec2 position) {
            render::buffers::VertexBufferData vertex{};
            vertex.vertexPosition = vec4{position.x, position.y, z, 1.0f};
            vertex.vertexColor = color;
            vertex.UV = vec2{0.5f, 0.5f};
            return vertex;
        };

        
        m_vertices.push_back(makeVertex(p0));
        m_vertices.push_back(makeVertex(p1));
        m_vertices.push_back(makeVertex(p2));
        m_vertices.push_back(makeVertex(p3));

        m_indices.push_back(base + 0);
        m_indices.push_back(base + 1);
        m_indices.push_back(base + 2);

        m_indices.push_back(base + 0);
        m_indices.push_back(base + 2);
        m_indices.push_back(base + 3);
    }

    void DebugRenderSystem2D::AppendTriangle(
        vec2 p0,
        vec2 p1,
        vec2 p2,
        float z,
        const graphics::Color& color
    ) {
        if(m_vertices.size() + 3 > MaxVertices || m_indices.size() + 3 > MaxIndices) {
            return;
        }

        const uint32_t base = static_cast<uint32_t>(m_vertices.size());
        auto makeVertex = [z, &color](vec2 position) {
            render::buffers::VertexBufferData vertex{};

            vertex.vertexPosition = vec4 {
                position.x,
                position.y,
                z,
                1.0f
            };
            
            vertex.vertexColor = color;
            vertex.UV = vec2{0.5f,0.5f};
            return vertex;
        };

        m_vertices.push_back(makeVertex(p0));
        m_vertices.push_back(makeVertex(p1));
        m_vertices.push_back(makeVertex(p2));

        m_indices.push_back(base);
        m_indices.push_back(base + 1);
        m_indices.push_back(base + 2);
    }

    void DebugRenderSystem2D::AppendWirePolygon(
        const std::vector<vec2>& vertices,
        float z,
        const graphics::Color& color,
        float thickness
    ) {
        if(vertices.size() < 3) {
            return;
        }

        const Settings& settings = GetSettings();

        const graphics::Color wireColor =
            MakeWireColor(color, settings);

        /*
        * Contorno completo.
        */
        for(size_t i = 0; i < vertices.size(); i++) {
            const size_t next =
                (i + 1) % vertices.size();

            AppendLine(
                vertices[i],
                vertices[next],
                z,
                wireColor,
                thickness
            );
        }

        if(!settings.showTriangulation) {
            return;
        }

        /*
        * Los polígonos convexos se triangulan como un fan:
        *
        * 0-1-2
        * 0-2-3
        * 0-3-4
        *
        * Solo dibujamos las diagonales internas. El resto de
        * los lados ya forma parte del contorno.
        */
        for(size_t i = 2; i + 1 < vertices.size(); i++) {
            AppendLine(
                vertices[0],
                vertices[i],
                z,
                wireColor,
                thickness
            );
        }
    }

    void DebugRenderSystem2D::AppendSolidPolygon(
        const std::vector<vec2>& vertices,
        float z,
        const graphics::Color& color,
        float thickness
    ) {
        if(vertices.size() < 3) {
            return;
        }

        const Settings& settings = GetSettings();

        const graphics::Color fillColor =
            MakeFillColor(color, settings);

        /*
        * Triangulación fan válida porque los colliders que
        * acepta física son convexos y antihorarios.
        */
        for(size_t i = 1; i + 1 < vertices.size(); i++) {
            AppendTriangle(
                vertices[0],
                vertices[i],
                vertices[i + 1],
                z,
                fillColor
            );
        }

        /*
        * Se añade después del relleno para que las líneas se
        * dibujen encima dentro del mismo command encoder.
        */
        AppendWirePolygon(
            vertices,
            z,
            color,
            thickness
        );
    }

    graphics::Color DebugRenderSystem2D::MakeFillColor(
        const graphics::Color& color,
        const Settings& settings
    ) {
        return graphics::Color {
            color.r * settings.fillDarkness,
            color.g * settings.fillDarkness,
            color.b * settings.fillDarkness,
            color.a * settings.fillOpacity
        };
    }

    graphics::Color DebugRenderSystem2D::MakeWireColor(
        const graphics::Color& color,
        const Settings& settings
    ) {
        return graphics::Color{
            std::min(1.0f, color.r + (1.0f - color.r) * settings.wireBrightness),
            std::min(1.0f, color.g + (1.0f - color.g) * settings.wireBrightness),
            std::min(1.0f, color.b + (1.0f - color.b) * settings.wireBrightness),
            color.a
        };
    }
}

#endif
