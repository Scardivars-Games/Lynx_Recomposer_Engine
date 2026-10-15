#pragma once
#include <lynx/ecs/system.h>

#if !LYNX_ONLY_ECS

#include <lynx/game_logic/ecs_bridge.h>
#include <lynx/core/lifetime.h>

namespace lynx {
    struct EngineContext;
}

namespace lynx::ecs::systems {
    class ScriptingSystem : public ISystem {
        public:
            //Llama a todos los init en el motor
            void Init(const EngineContext* engineContext) override;
            void Update() override;
            void FixedUpdate() const;
            void LateUpdate() const;
    };
}

REGISTER_NATIVE_SYSTEM(lynx::ecs::systems::ScriptingSystem, lynx::ecs::SystemPhase::VariableUpdate, 0);
REQUIRED_COMPONENTS_FOR_SYSTEM(lynx::ecs::systems::ScriptingSystem, lynx::ecs::components::ScriptingBridgeComponent);
#else
//Esto es por compatibilidad
namespace lynx::ecs::systems {
    class ScriptingSystem final: public ISystem {

    };
}
#endif //!LYNX_ONLY_ECS