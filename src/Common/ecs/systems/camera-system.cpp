#include <ilen/camera/camera-system.h>
#include <ilen/transform/transform-system.h>
#include <ilen/camera/camera-components.h>
#include <ilen/headers/ilen-logs.h>

namespace ilen::ecs::systems {
    using namespace ilen::ecs::components;

    void CameraSystem::Init(const EngineContext* engineContext) {
        p_Coordinator = engineContext->p_coordinator.get();
        p_Renderer = engineContext->p_gameRenderer.get();

        //Asignarles a cada cámara un buffer para la matriz
        for (auto const& entity: entities) {
            auto* cameraComponent = p_Coordinator->getComponent<CameraComponent>(entity);

            render::IlenBufferDescriptor descriptor{};
            descriptor.bufferLabelName = "Camera Buffer for entity " + std::to_string(entity);
            descriptor.bufferSize = sizeof(mat4);
            descriptor.bufferType = ILEN_RENDER_GPU_BUFFER_UNIFORM;
            descriptor.count = 1; //Solo hay una cámara por buffer
            descriptor.stride = 0; //Misma cosa
            cameraComponent->cameraBuffer = p_Renderer->createBuffer(descriptor, nullptr);
        }
    }

    void CameraSystem::Update(uint32_t viewportWidth, uint32_t viewportHeight) {
        auto actualGraphicsAPI = ilen::render::IRenderer::getActualGraphicsAPI();
        if (viewportHeight == 0|| viewportWidth == 0) {
            return; //Si el tamaño de la ventana es 0, no se puede hacer ningún calculo para la cámara.
        }

        const float aspectRatio = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);

        for (auto const& entity: entities) {
            auto* cameraComponent = p_Coordinator->getComponent<CameraComponent>(entity);
            auto* transformComponent = p_Coordinator->getComponent<TransformComponent>(entity);

            if (!cameraComponent || !transformComponent) {
                continue; //Esta cámara no es válida
            }

            if (cameraComponent->isPrimary == true) {
                m_mainCamera = cameraComponent;
            }
           //ILEN_LOG_P("A", matrix::to_string(transformComponent->getWorldMatrix()).c_str());

            cameraComponent->viewMatrix = matrix::inverse(transformComponent->getWorldMatrix());
            //ILEN_LOG_P("A", matrix::to_string(cameraComponent->viewMatrix).c_str());

            if (cameraComponent->projectionType == ILEN_CAMERA_PROJECTION_TYPE::ILEN_CAMERA_PROJECTION_PERSPECTIVE) {
                cameraComponent->projectionMatrix = matrix::perspective(Degrees(cameraComponent->fovDegrees), aspectRatio, cameraComponent->nearClip, cameraComponent->farClip, actualGraphicsAPI);
            }else {
                float halfWidth = (cameraComponent->orthoSize * aspectRatio) * 0.5f;
                float halfHeight = cameraComponent->orthoSize * 0.5f;

                cameraComponent->projectionMatrix = matrix::orthographic(-halfWidth, halfWidth, -halfHeight, halfHeight, cameraComponent->nearClip, cameraComponent->farClip, actualGraphicsAPI);
            }

            auto viewProjectionMatrix = cameraComponent->projectionMatrix * cameraComponent->viewMatrix;
            cameraComponent->viewProjectionMatrix = viewProjectionMatrix;
            //ILEN_LOG_P("A", matrix::to_string(cameraComponent->viewProjectionMatrix).c_str());
            cameraComponent->cameraBuffer->writeDataToBuffer(&viewProjectionMatrix, 0 ,0);
        }
    }

    const components::CameraComponent* CameraSystem::getMainCamera() const {
        return m_mainCamera;
    }
}
