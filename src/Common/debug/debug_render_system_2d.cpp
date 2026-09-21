#include <ilen/debug/debug_render_system_2d.h>

#if ILEN_DEBUG

#include <ilen/camera/camera_system.h>
#include <ilen/core/engine_context.h>
#include <ilen/core/runtime_options.h>
#include <ilen/core/time.h>
#include <ilen/ecs/ecs.h>
#include <ilen/physics/components/physics_components.h>
#include <ilen/render/render_constants.h>
#include <ilen/resources/materials_manager.h>
#include <ilen/resources/texture_manager.h>
#include <ilen/transform/transform_component.h>

#include <cmath>

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

        descriptor.renderPipelineOptionFlags = 
            ILEN_RENDER_PIPELINES_OPTIONS_DONT_USES_DEPTH_BUFFER |
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
            auto* colliderArray = p_coordinator->getComponentArray<BoxCollider2D>();
            auto* rbArray = p_coordinator->getComponentArray<Rigidbody2D>();
            auto* collisionArray = p_coordinator->getComponentArray<CollisionComponent2D>();

            for(Entity entity: entities) {
                auto* transform = transformArray->getData(entity);
                auto* collider = colliderArray->getData(entity);

                if(!transform || !collider){
                    continue;
                }

                const vec2 center {
                    transform->position.x + collider->offset.x,
                    transform->position.y + collider->offset.y
                };
                
                const vec2 halfExtents {
                    collider->halfExtents.x * transform->scale.x,
                    collider->halfExtents.y * transform->scale.y
                };

                graphics::Color color = setting.dynamicColor;

                if(auto* rb = rbArray->getData(entity); rb && rb->isStatic) {
                    color = setting.staticColor;
                }
                
                if(collider->isTrigger) {
                    color = setting.triggerColor;
                }

                if(auto* collision = collisionArray->getData(entity); collision && !collision->contacts.empty()) {
                    color = setting.collidingColor;
                }

                if(setting.colliderMode == DebugShapeMode2D::Solid){
                    AppendSolidBox(
                        center, halfExtents, transform->position.z, color, setting.outlineThickness
                    );
                } else {
                    AppendWireBox(
                        center, halfExtents, transform->position.z, color, setting.outlineThickness
                    );
                }
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
                    if(primitive.mode == DebugShapeMode2D::Solid) {
                        AppendSolidBox(
                            primitive.a,
                            primitive.b,
                            primitive.z,
                            primitive.color,
                            primitive.thickness
                        );
                    } else {
                        AppendWireBox(
                            primitive.a,
                            primitive.b,
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

        ILEN_LOG_P("DEBUG_RENDER", "Paso la comprobacion");

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

    void DebugRenderSystem2D::AppendSolidBox(
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
    }

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
}

#endif