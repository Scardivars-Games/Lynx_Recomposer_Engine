#include <ilen/camera/camera-system.h>
#include <ilen/transform/transform-system.h>
#include <ilen/camera/camera-components.h>

namespace ilen::ecs::systems {
    using namespace ilen::ecs::components;

    void CameraSystem::Init(const EngineContext* engineContext) {
        p_Coordinator = engineContext->p_coordinator.get();
    }

    void CameraSystem::Update(float viewportWidth, float viewportHeight) {
        if (viewportHeight == 0.0f || viewportWidth == 0.0f) {
            return; //Si el tamaño de la ventana es 0, no se puede hacer ningún calculo para la cámara.
        }

        const float aspectRatio = viewportWidth / viewportHeight;

        for (auto const& entity: entities) {
            auto* cameraComponent = p_Coordinator->getComponent<CameraComponent>(entity);
            auto* transformComponent = p_Coordinator->getComponent<TransformComponent>(entity);

            if (!cameraComponent || !transformComponent) {
                continue; //Esta cámara no es válida
            }

            if (cameraComponent->isPrimary == true) {
                m_mainCamera = cameraComponent;
            }

            cameraComponent->viewMatrix = matrix::inverse(transformComponent->getWorldMatrix());
            if (cameraComponent->projectionType == ILEN_CAMERA_PROJECTION_TYPE::ILEN_CAMERA_PROJECTION_PERSPECTIVE) {
                cameraComponent->projectionMatrix = matrix::perspective(Degrees(cameraComponent->fovDegrees), aspectRatio, cameraComponent->nearClip, cameraComponent->farClip);
            }else {
                float halfWidth = (cameraComponent->orthoSize * aspectRatio) * 0.5f;
                float halfHeight = cameraComponent->orthoSize * 0.5f;

                cameraComponent->projectionMatrix = matrix::orthographic(-halfWidth, halfWidth, -halfHeight, halfHeight, cameraComponent->nearClip, cameraComponent->farClip);
            }

            cameraComponent->viewProjectionMatrix = cameraComponent->projectionMatrix * cameraComponent->viewMatrix;
        }
    }

    const components::CameraComponent& CameraSystem::getMainCamera() const {
        return *m_mainCamera;
    }
}
