#pragma once

#include <ilen/ecs/system.h>
#include <ilen/game-logic/ecs-bridge.h>
#include <ilen/core/lifetime.h>

namespace ilen {
    struct EngineContext;
}

namespace ilen::ecs::systems {
    class ScriptingSystem : public System {
        public:
            //Llama a todos los init en el motor
            void Init(EngineContext* engineContext);
            void Update() const;
            void FixedUpdate() const;
            void LateUpdate() const;

        private:
            ilen::ecs::ECSCoordinator* m_Coordinator = nullptr;
    };
}