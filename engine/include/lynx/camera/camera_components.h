#pragma once

#include <lynx/headers/enumerations.h>
#include <lynx/camera/camera_system.h>
#include <lynx/headers/maths.h>
#include <lynx/render/gpu_interfaces/buffers.h>
#include <lynx/graphics/color.h>

namespace lynx::ecs::systems {
    class CameraSystem;
}

namespace lynx::ecs::components {
    using namespace lynx::enumerations;
    using namespace lynx::maths::matrix;
    using namespace lynx::maths::vector;

    LYNX_COMPONENT
    struct CameraComponent {
        friend class lynx::ecs::systems::CameraSystem;

        public:
            LYNX_PROPERTY bool isPrimary = true;
            LYNX_PROPERTY LYNX_CAMERA_PROJECTION_TYPE projectionType = LYNX_CAMERA_PROJECTION_TYPE::LYNX_CAMERA_PROJECTION_PERSPECTIVE;

            //Valores de la camara
            LYNX_PROPERTY float fovDegrees  = 70.0f;
            LYNX_PROPERTY float orthoSize = 10.0f;
            LYNX_PROPERTY float nearClip = 0.1f;
            LYNX_PROPERTY float farClip  = 1000.0f;
            LYNX_PROPERTY graphics::Color clearColor = {0.0, 0.6588, 0.4196, 1.0}; //Jade por defecto... Ashiel eyes color.

            //Para que siga a un objeto, o lo mire.
            LYNX_PROPERTY Entity toFollowEntity = NULL_ENTITY;
            LYNX_PROPERTY vec3 cameraOffsetWithFollowEntity = vec3(0.0f);
            LYNX_PROPERTY  Entity toLookAtEntity = NULL_ENTITY;

            LYNX_PROPERTY Entity cameraEntity = NULL_ENTITY; //Acceso directo de la entidad que contiene la camara para un fácil acceso.

      /*      //Getters.
            const mat4& getViewMatrix() const { return viewMatrix; }
            const mat4& getProjectionMatrix() const { return projectionMatrix; }
            const mat4& getViewProjectionMatrix() const { return viewProjectionMatrix; }
            render::IGPUBuffer* getViewProjectionCameraBuffer() const {return  cameraBuffer.get(); }
        private:*/
            LYNX_PROPERTY mat4 viewMatrix = mat4::identity();
            LYNX_PROPERTY mat4 projectionMatrix = mat4::identity();
            LYNX_PROPERTY mat4 viewProjectionMatrix = mat4::identity();
            std::shared_ptr<render::IGPUBuffer> cameraBuffer = {};
            //Faltaría implementar un Frustum para hacer clipping.
    };
}
