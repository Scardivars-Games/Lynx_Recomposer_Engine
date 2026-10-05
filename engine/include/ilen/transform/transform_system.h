#pragma once

#include <ilen/ecs/system.h>
#include <ilen/ecs/ecs.h>
#include <ilen/headers/maths.h>
#include <ilen_public/API/transform_api.h>
#include <ilen/ecs/system_registry.h>
#include <ilen/core/engine_context.h>
#include <ilen/transform/transform_component.h>


using namespace ilen::maths;

namespace ilen::ecs::systems {
    using namespace ilen::ecs::components;

    class TransformSystem : public ilen::ecs::ISystem {
        public:
            void Init(const ilen::EngineContext* engineContext) override;
            void Update() override;
            void ExecuteTransformOrders();
        private:
            void calculateLocalToWorldMatrix(Entity entity, matrix::mat4 parentMatrix, ilen::job_system::JobGroup* group, ComponentArray<TransformComponent>* componentArray);
    };
}

REGISTER_NATIVE_SYSTEM(ilen::ecs::systems::TransformSystem, ilen::ecs::SystemPhase::PreRender, 1)
REQUIRED_COMPONENTS_FOR_SYSTEM(ilen::ecs::systems::TransformSystem, ilen::ecs::components::TransformComponent)