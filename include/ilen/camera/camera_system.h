#pragma once

#include <ilen/ecs/ecs.h>
#include <ilen/ecs/system.h>
#include <ilen/ecs/types.h>
#include <ilen/camera/camera_components.h>
#include <ilen/ecs/system_registry.h>
#include <ilen/render/render_constants.h>
#include <unordered_map>
#include <vector>


namespace ilen::ecs::systems {
    class CameraSystem: public ilen::ecs::ISystem {
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


REGISTER_NATIVE_SYSTEM(ilen::ecs::systems::CameraSystem, ilen::ecs::SystemPhase::PreRender, 2)
REQUIRED_COMPONENTS_FOR_SYSTEM(ilen::ecs::systems::CameraSystem, ilen::ecs::components::CameraComponent)
