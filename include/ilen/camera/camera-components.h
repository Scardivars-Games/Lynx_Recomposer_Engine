#pragma once

#include <ilen/headers/enumerations.h>
#include <ilen/camera/camera-system.h>
#include <ilen/headers/maths.h>
#include <ilen/render/gpu-interfaces/buffers.h>

namespace ilen::ecs::systems {
    class CameraSystem;
}

namespace ilen::ecs::components {
    using namespace ilen::enumerations;
    using namespace ilen::maths::matrix;
    using namespace ilen::maths::vector;

    ILEN_COMPONENT
    struct CameraComponent {
        friend class ilen::ecs::systems::CameraSystem;

        public:
            ILEN_PROPERTY bool isPrimary = true;
            ILEN_PROPERTY ILEN_CAMERA_PROJECTION_TYPE projectionType = ILEN_CAMERA_PROJECTION_TYPE::ILEN_CAMERA_PROJECTION_PERSPECTIVE;

            //Valores de la camara
            ILEN_PROPERTY float fovDegrees  = 70.0f;
            ILEN_PROPERTY float orthoSize = 10.0f;
            ILEN_PROPERTY float nearClip = 0.1f;
            ILEN_PROPERTY float farClip  = 1000.0f;

            //Para que siga a un objeto, o lo mire.
            ILEN_PROPERTY Entity toFollowEntity = NULL_ENTITY;
            ILEN_PROPERTY vec3 cameraOffsetWithFollowEntity = vec3(0.0f);
            ILEN_PROPERTY  Entity toLookAtEntity = NULL_ENTITY;

            ILEN_PROPERTY Entity cameraEntity = NULL_ENTITY; //Acceso directo de la entidad que contiene la camara para un fácil acceso.

      /*      //Getters.
            const mat4& getViewMatrix() const { return viewMatrix; }
            const mat4& getProjectionMatrix() const { return projectionMatrix; }
            const mat4& getViewProjectionMatrix() const { return viewProjectionMatrix; }
            render::IGPUBuffer* getViewProjectionCameraBuffer() const {return  cameraBuffer.get(); }
        private:*/
            ILEN_PROPERTY mat4 viewMatrix = mat4::identity();
            ILEN_PROPERTY mat4 projectionMatrix = mat4::identity();
            ILEN_PROPERTY mat4 viewProjectionMatrix = mat4::identity();
            std::shared_ptr<render::IGPUBuffer> cameraBuffer = {};
            //Faltaría implementar un Frustum para hacer clipping.
    };
}
