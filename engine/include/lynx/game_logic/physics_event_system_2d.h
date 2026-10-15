#pragma once

#if !LYNX_ONLY_ECS
#include <lynx/ecs/system.h>
#include <lynx/ecs/system_registry.h>
#include <lynx/game_logic/ecs_bridge.h>
#include <lynx/physics/components/specification/collision_components.h>

namespace lynx::ecs::systems {
    class PhysicsEventSystem2D final : public ISystem {
        public:
            void Init(const lynx::EngineContext* engineContext) override;
            void Update() override;
    };
}

REGISTER_NATIVE_SYSTEM(lynx::ecs::systems::PhysicsEventSystem2D, lynx::ecs::SystemPhase::FixedUpdate, 20);
REQUIRED_COMPONENTS_FOR_SYSTEM(lynx::ecs::systems::PhysicsEventSystem2D, lynx::ecs::components::ScriptingBridgeComponent);

#endif