#pragma once

#include <lynx/headers/ecs.h>
#include <lynx/camera/camera_system.h>
#include <lynx/camera/camera_components.h>
#include <lynx/game_logic/script_system.h>
#include <lynx/maths/vector.h>
#include <lynx/maths/matrix.h>
#include <vector>
#include <functional>

namespace lynx::engine::api {
    using namespace lynx::maths::vector;

    class CameraAPI {
        public:
            static vec2 ConvertScreenToNDC(const vec2& screenPos);
            static vec3 ConvertNDCToWorld(const lynx::ecs::components::CameraComponent* camera, const vec2& ndcPos);
            static vec3 ConvertScreenToWorld(const lynx::ecs::components::CameraComponent* camera, const vec2& screenPos);

            static void MakeMainCameraFollowThisEntity(lynx::ecs::Entity entityToFollow);

            static inline lynx::ecs::components::CameraComponent* mainCamera = nullptr; //Para poder ser usado directamente por los scripts
        private:
            static void Init(const lynx::EngineContext* context);
            static void ResetMainCameraPointer();
            static void GetMainCameraPointer();

            //Step by step functions, for doing conversions
            static inline vec2 ScreenToNDC(const vec2& screenPos);
            static inline vec3 NDCToWorld(const lynx::ecs::components::CameraComponent* camera, const vec2& ndcPos);

            static inline lynx::ecs::ECSCoordinator* p_coordinator = nullptr;
            static inline lynx::ecs::systems::CameraSystem* p_cameraSystem = nullptr;

            //Para deferir tareas cuando la main camera no es posible acceder a ella
            inline static std::vector<std::function<void()>> defferedTask;
            inline static void ProcessDeferredTasks();

            friend class lynx::ecs::systems::ScriptingSystem;
    };
}