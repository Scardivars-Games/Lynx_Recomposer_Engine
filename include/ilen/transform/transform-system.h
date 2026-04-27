#pragma once

#include <ilen/ecs/system.h>
#include <ilen/ecs/ecs.h>
#include <ilen/headers/maths.h>
#include <ilen-public/API/transform.h>
#include <ilen/ecs/system_registry.h>
#include <ilen/core/engine-context.h>
#include <ilen/transform/transform-component.h>


using namespace ilen::maths;

namespace ilen::ecs::systems {
    using namespace ilen::ecs::components;

    class TransformSystem : public ilen::ecs::ISystem {
        public:
            void Init(const ilen::EngineContext* engineContext) override;

            void Update() override;

        private:
            ilen::ecs::ECSCoordinator* p_Coordinator = nullptr;
            job_system::JobSystem* p_jobSystem = nullptr;

            void calculateLocalToWorldMatrix(Entity entity, matrix::mat4 parentMatrix, ilen::job_system::JobGroup* group);
    };
}

REGISTER_NATIVE_SYSTEM(ilen::ecs::systems::TransformSystem, ilen::ecs::SystemPhase::PreRender, 1)
REQUIRED_COMPONENTS_FOR_SYSTEM(ilen::ecs::systems::TransformSystem, ilen::ecs::components::TransformComponent)