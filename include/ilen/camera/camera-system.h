#pragma once

#include <ilen/ecs/ecs.h>
#include <ilen/ecs/system.h>
#include <ilen/ecs/types.h>
#include <ilen/core/lifetime.h>
#include <ilen/camera/camera-components.h>

namespace ilen::ecs::systems {
    class CameraSystem: public ilen::ecs::System {
        public:
            void Init(const EngineContext* engineContext);
            void Update(float viewportWidth, float viewportHeight);

            const components::CameraComponent& getMainCamera() const;
        private:
            ECSCoordinator* p_Coordinator = nullptr;
            components::CameraComponent* m_mainCamera = nullptr;
    };
}