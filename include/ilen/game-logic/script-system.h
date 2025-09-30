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
            void init(EngineContext* engineContext) {
                m_Coordinator = engineContext->p_coordinator.get();

                for(auto const& entity: entities){
                    auto scriptComponent = m_Coordinator->getComponent<ilen::ecs::components::ScriptingBridgeComponent>(entity);

                    //En este paso se instancia el script, así que lo hacemos aquí, y ejecutamos el init de él.
                    if(!scriptComponent->instance){
                        scriptComponent->instance = scriptComponent->instantiate();
                        scriptComponent->instance->entity = entity;
                        scriptComponent->instance->coordinator = m_Coordinator; //.get devuelve el puntero puro (*) en vez del unique_ptr
                        scriptComponent->instance->Init();
                    }
                }
            }

            void update() {
                for(auto const& entity: entities){
                    auto scriptComponent = m_Coordinator->getComponent<ilen::ecs::components::ScriptingBridgeComponent>(entity);
                    if(scriptComponent->instance){
                        scriptComponent->instance->Update();
                    }
                }
            }

            void fixedUpdate(){
                for(auto const& entity: entities){
                    auto scriptComponent = m_Coordinator->getComponent<ilen::ecs::components::ScriptingBridgeComponent>(entity);
                    if(scriptComponent->instance){
                        scriptComponent->instance->FixedUpdate();
                    }
                }
            }

            void lateUpdate(){
                for(const auto& entity: entities){
                    auto scriptComponent = m_Coordinator->getComponent<ilen::ecs::components::ScriptingBridgeComponent>(entity);
                    if(scriptComponent->instance){
                        scriptComponent->instance->LateUpdate();
                    }
                }
            }

        private:
            ilen::ecs::ECSCoordinator* m_Coordinator;
    };
}