#pragma once

#include <ilen/headers/ecs.h>
#include <ilen/camera/camera-system.h>
#include <ilen/camera/camera-components.h>
#include <ilen/game-logic/script-system.h>
#include <ilen/maths/vector.h>
#include <ilen/maths/matrix.h>

namespace ilen::engine::api {
    using namespace ilen::maths::vector;

    class CameraAPI {
        public:
            static vec2 ConvertScreenToNDC(const vec2& screenPos);
            static vec3 ConvertNDCToWorld(const ilen::ecs::components::CameraComponent* camera, const vec2& ndcPos);
            static vec3 ConvertScreenToWorld(const ilen::ecs::components::CameraComponent* camera, const vec2& screenPos);

            const static inline ilen::ecs::components::CameraComponent* mainCamera = nullptr; //Para poder ser usado directamente por los scripts
        private:
            static void Init(const ilen::EngineContext* context);
            static void ResetMainCameraPointer();
            static void GetMainCameraPointer();

            //Step by step functions, for doing conversions
            static inline vec2 ScreenToNDC(const vec2& screenPos);
            static inline vec3 NDCToWorld(const ilen::ecs::components::CameraComponent* camera, const vec2& ndcPos);

            static inline ilen::ecs::ECSCoordinator* p_coordinator = nullptr;
            static inline ilen::ecs::systems::CameraSystem* p_cameraSystem = nullptr;

            friend class ilen::ecs::systems::ScriptingSystem;
    };
}