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

    class CameraComponent {
        friend class ilen::ecs::systems::CameraSystem;

        public:
            bool isPrimary = true;
            ILEN_CAMERA_PROJECTION_TYPE projectionType = ILEN_CAMERA_PROJECTION_TYPE::ILEN_CAMERA_PROJECTION_PERSPECTIVE;

            //Valores de la camara
            float fovDegrees  = 70.0f;
            float orthoSize = 10.0f;
            float nearClip = 0.1f;
            float farClip  = 1000.0f;

            //Para que siga a un objeto, o lo mire.
            Entity toFollowEntity = NULL_ENTITY;
            vec3 cameraOffsetWithFollowEntity = vec3(0.0f);
            Entity toLookAtEntity = NULL_ENTITY;

            Entity cameraEntity = NULL_ENTITY; //Acceso directo de la entidad que contiene la camara para un fácil acceso.

      /*      //Getters.
            const mat4& getViewMatrix() const { return viewMatrix; }
            const mat4& getProjectionMatrix() const { return projectionMatrix; }
            const mat4& getViewProjectionMatrix() const { return viewProjectionMatrix; }
            render::IGPUBuffer* getViewProjectionCameraBuffer() const {return  cameraBuffer.get(); }
        private:*/
            mat4 viewMatrix = mat4::identity();
            mat4 projectionMatrix = mat4::identity();
            mat4 viewProjectionMatrix = mat4::identity();
            std::shared_ptr<render::IGPUBuffer> cameraBuffer = {};
            //Faltaría implementar un Frustum para hacer clipping.
    };
}
