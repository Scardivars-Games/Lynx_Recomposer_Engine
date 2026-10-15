#include <lynx/camera/camera_system.h>
#include <lynx/transform/transform_system.h>
#include <lynx/camera/camera_components.h>
#include <lynx/headers/lynx_logs.h>
#include <lynx/core/engine_context.h>
#include <lynx/render/render_constants.h>
#include <lynx/render/renderer.h>
#include <lynx/window/window.h>


namespace lynx::ecs::systems {
    using namespace lynx::ecs::components;

    void CameraSystem::Init(const EngineContext* engineContext) {
        p_coordinator = engineContext->p_coordinator.get();
        p_Renderer = engineContext->p_gameRenderer.get();

        m_frameData.clear();
        m_mainCamera = nullptr;

        // Los buffers se crean al subir el primer frame de cada cámara.
        for (auto const& entity: entities) {
            auto* cameraComponent = p_coordinator->getComponent<CameraComponent>(entity);


            if (cameraComponent->isPrimary == true) {
                m_mainCamera = cameraComponent;
            }

            if(cameraComponent->cameraEntity == NULL_ENTITY){
                cameraComponent->cameraEntity = entity;
            }
        }
    }

    void CameraSystem::Update() {
        m_mainCamera = nullptr;
        std::erase_if(m_frameData, [this](const auto& entry) {
            return !entities.contains(entry.first);
        });
        {
            lynx::time::ScopedTimer timer("Camera system update");

            auto actualGraphicsAPI = lynx::render::IRenderer::getActualGraphicsAPI();
            
            auto viewportHeight = lynx::window::IWindow::getHeight();
            auto viewportWidth = lynx::window::IWindow::getWidth();
            if (viewportHeight == 0|| viewportWidth == 0) {
                return; //Si el tamaño de la ventana es 0, no se puede hacer ningún calculo para la cámara.
            }

            const float aspectRatio = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
            auto* cameraComponentArray = p_coordinator->getComponentArray<CameraComponent>();
            auto* transformComponentArray = p_coordinator->getComponentArray<TransformComponent>();

            for (auto const& entity: entities) {
                //auto* cameraComponent = p_coordinator->getComponent<CameraComponent>(entity);
                //auto* transformComponent = p_coordinator->getComponent<TransformComponent>(entity);
                auto* cameraComponent = cameraComponentArray->getData(entity);
                auto* transformComponent = transformComponentArray->getData(entity);

                if (!cameraComponent || !transformComponent) {
                    continue; //Esta cámara no es válida
                }

                if (cameraComponent->isPrimary == true) {
                    m_mainCamera = cameraComponent;
                }
                //LYNX_LOG_P("A", matrix::to_string(transformComponent->getWorldMatrix()).c_str());
                //Esto es temporal, a futuro habra un sistema de cámara especiales que hara esta chamba, ejecutandose antes del transform system, para que no se tenga que recalcular la matriz
                /*if(cameraComponent->toFollowEntity != NULL_ENTITY) {
                    //Necesita seguir a un objeto
                    auto* toFollowTransform = transformComponentArray->getData(cameraComponent->toFollowEntity);
                    transformComponent->position = toFollowTransform->position + cameraComponent->cameraOffsetWithFollowEntity;

                    constexpr float DEG_TO_RAD = 3.14159265359f / 180.0f;
                    vec3 rotRadians = transformComponent->rotation * DEG_TO_RAD;
                    
                    // Re-generamos la matriz TRS solo para la cámara
                    matrix::mat4 localMatrix = matrix::createTRS(transformComponent->position, rotRadians, transformComponent->scale);
                    
                    // Si la cámara tiene padre, multiplicamos. Si es raíz (lo más común), se queda así.
                    if(transformComponent->parent != NULL_ENTITY) {
                        auto* parentTransform = transformComponentArray->getData(transformComponent->parent);
                        transformComponent->localToWorldMatrix = parentTransform->localToWorldMatrix * localMatrix;
                    } else {
                        transformComponent->localToWorldMatrix = localMatrix;
                    }
                }*/

                cameraComponent->viewMatrix = matrix::inverse(transformComponent->localToWorldMatrix);
                //LYNX_LOG_P("A", matrix::to_string(cameraComponent->viewMatrix).c_str());

                if (cameraComponent->projectionType == LYNX_CAMERA_PROJECTION_TYPE::LYNX_CAMERA_PROJECTION_PERSPECTIVE) {
                    cameraComponent->projectionMatrix = matrix::perspective(Degrees(cameraComponent->fovDegrees), aspectRatio, cameraComponent->nearClip, cameraComponent->farClip, actualGraphicsAPI);
                }else {
                    float halfWidth = (cameraComponent->orthoSize * aspectRatio) * 0.5f;
                    float halfHeight = cameraComponent->orthoSize * 0.5f;

                    cameraComponent->projectionMatrix = matrix::orthographic(-halfWidth, halfWidth, -halfHeight, halfHeight, cameraComponent->nearClip, cameraComponent->farClip, actualGraphicsAPI);
                }

                auto viewProjectionMatrix = cameraComponent->projectionMatrix * cameraComponent->viewMatrix;
                auto previousProjectionMatrix = cameraComponent->viewProjectionMatrix;
                cameraComponent->viewProjectionMatrix = viewProjectionMatrix;
                //LYNX_LOG_P("A", matrix::to_string(cameraComponent->viewProjectionMatrix).c_str());

                // PreRender solo prepara CPU: la GPU puede seguir leyendo el frame anterior.
                auto& cameraData = m_frameData[entity].data;
                cameraData.currentViewProjectionMatrix = viewProjectionMatrix;
                cameraData.previousViewProjectionMatrix = previousProjectionMatrix;
            }
        }
    }

    void CameraSystem::UploadFrameData() {
        if (!p_Renderer) return;
        const auto frame = p_Renderer->getActualFrameInFlight();
        const auto frameCount = p_Renderer->getActualNumOfFramesInFlight();
        for (auto& [entity, state] : m_frameData) {
            auto* camera = p_coordinator->getComponent<CameraComponent>(entity);
            if (!camera) continue;
            if (state.buffers.empty()) {
                state.buffers.resize(frameCount);
                for (size_t slot = 0; slot < frameCount; ++slot) {
                    render::LynxBufferDescriptor descriptor{};
                    descriptor.bufferLabelName = "Camera " + std::to_string(entity) + " frame " + std::to_string(slot);
                    descriptor.bufferSize = sizeof(render::structs::CameraData);
                    descriptor.bufferType = LYNX_RENDER_GPU_BUFFER_UNIFORM;
                    descriptor.count = 1;
                    state.buffers[slot] = p_Renderer->createBuffer(descriptor, nullptr);
                }
            }
            // beginFrame ya esperó a que la GPU liberase este slot.
            camera->cameraBuffer = state.buffers[frame];
            camera->cameraBuffer->writeDataToBuffer(&state.data, sizeof(state.data), 0);
        }
    }

    components::CameraComponent* CameraSystem::getMainCamera() const {
        return m_mainCamera;
    }
}
