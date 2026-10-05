#pragma once

#if !ILEN_ONLY_ECS
#include <ilen/ecs/system.h>
#include <ilen/ecs/system_registry.h>
#include <ilen/game_logic/ecs_bridge.h>
#include <ilen/physics/components/specification/collision_components.h>

namespace ilen::ecs::systems {
    class PhysicsEventSystem2D final : public ISystem {
        public:
            void Init(const ilen::EngineContext* engineContext) override;
            void Update() override;
    };
}

REGISTER_NATIVE_SYSTEM(ilen::ecs::systems::PhysicsEventSystem2D, ilen::ecs::SystemPhase::FixedUpdate, 20);
REQUIRED_COMPONENTS_FOR_SYSTEM(ilen::ecs::systems::PhysicsEventSystem2D, ilen::ecs::components::ScriptingBridgeComponent);

#endif