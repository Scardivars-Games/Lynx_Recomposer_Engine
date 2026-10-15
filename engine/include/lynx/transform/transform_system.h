#pragma once

#include <lynx/ecs/system.h>
#include <lynx/ecs/ecs.h>
#include <lynx/headers/maths.h>
#include <lynx_public/API/transform_api.h>
#include <lynx/ecs/system_registry.h>
#include <lynx/core/engine_context.h>
#include <lynx/transform/transform_component.h>


using namespace lynx::maths;

namespace lynx::ecs::systems {
    using namespace lynx::ecs::components;

    class TransformSystem : public lynx::ecs::ISystem {
        public:
            void Init(const lynx::EngineContext* engineContext) override;
            void Update() override;
            void ExecuteTransformOrders();
        private:
            void calculateLocalToWorldMatrix(Entity entity, matrix::mat4 parentMatrix, lynx::job_system::JobGroup* group, ComponentArray<TransformComponent>* componentArray);
    };
}

REGISTER_NATIVE_SYSTEM(lynx::ecs::systems::TransformSystem, lynx::ecs::SystemPhase::PreRender, 1)
REQUIRED_COMPONENTS_FOR_SYSTEM(lynx::ecs::systems::TransformSystem, lynx::ecs::components::TransformComponent)