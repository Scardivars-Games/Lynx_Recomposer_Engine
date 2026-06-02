#pragma once

#if !ILEN_ONLY_ECS

#include <ilen/ecs/system.h>
#include <ilen/game_logic/ecs_bridge.h>
#include <ilen/core/lifetime.h>

namespace ilen {
    struct EngineContext;
}

namespace ilen::ecs::systems {
    class ScriptingSystem : public ISystem {
        public:
            //Llama a todos los init en el motor
            void Init(const EngineContext* engineContext) override;
            void Update() override;
            void FixedUpdate() const;
            void LateUpdate() const;
    };
}

REGISTER_NATIVE_SYSTEM(ilen::ecs::systems::ScriptingSystem, ilen::ecs::SystemPhase::VariableUpdate, 0);
REQUIRED_COMPONENTS_FOR_SYSTEM(ilen::ecs::systems::ScriptingSystem, ilen::ecs::components::ScriptingBridgeComponent);

#endif //!ILEN_ONLY_ECS