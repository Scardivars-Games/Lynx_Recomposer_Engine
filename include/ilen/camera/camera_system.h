#pragma once

#include <ilen/ecs/ecs.h>
#include <ilen/ecs/system.h>
#include <ilen/ecs/types.h>
#include <ilen/camera/camera_components.h>
#include <ilen/ecs/system_registry.h>


namespace ilen::ecs::systems {
    class CameraSystem: public ilen::ecs::ISystem {
        public:
            void Init(const EngineContext* engineContext) override;
            void Update() override;

            components::CameraComponent* getMainCamera() const;
        private:
            render::IRenderer* p_Renderer = nullptr;
            components::CameraComponent* m_mainCamera = nullptr;
    };
}


REGISTER_NATIVE_SYSTEM(ilen::ecs::systems::CameraSystem, ilen::ecs::SystemPhase::PreRender, 2)
REQUIRED_COMPONENTS_FOR_SYSTEM(ilen::ecs::systems::CameraSystem, ilen::ecs::components::CameraComponent)