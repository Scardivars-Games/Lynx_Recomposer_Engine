#pragma once

#include <lynx/ecs/ecs.h>
#include <lynx/ecs/system.h>
#include <lynx/ecs/types.h>
#include <lynx/camera/camera_components.h>
#include <lynx/ecs/system_registry.h>
#include <lynx/render/render_constants.h>
#include <unordered_map>
#include <vector>


namespace lynx::ecs::systems {
    class CameraSystem: public lynx::ecs::ISystem {
        public:
            void Init(const EngineContext* engineContext) override;
            void Update() override;
            // Call after renderer.beginFrame(), before any render pass uses the camera.
            void UploadFrameData();

            components::CameraComponent* getMainCamera() const;
        private:
            struct CameraFrameData {
                render::structs::CameraData data{};
                std::vector<std::shared_ptr<render::IGPUBuffer>> buffers;
            };
            std::unordered_map<Entity, CameraFrameData> m_frameData;
            render::IRenderer* p_Renderer = nullptr;
            components::CameraComponent* m_mainCamera = nullptr;
    };
}


REGISTER_NATIVE_SYSTEM(lynx::ecs::systems::CameraSystem, lynx::ecs::SystemPhase::PreRender, 2)
REQUIRED_COMPONENTS_FOR_SYSTEM(lynx::ecs::systems::CameraSystem, lynx::ecs::components::CameraComponent)
