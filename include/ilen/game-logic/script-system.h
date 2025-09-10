#pragma once

#include <ilen/ecs/system.h>
#include <ilen/game-logic/ecs-bridge.h>

extern std::unique_ptr<ilen::ecs::ECSCoordinator> gCoordinator;

namespace ilen::ecs::systems {
    class ScriptingSystem : public System {
        public:
            //Llama a todos los init en el motor
            void init() {
                for(auto const& entity: entities){
                    auto& scriptComponent = gCoordinator->getComponent<ilen::ecs::components::ScriptingBridgeComponent>(entity);

                    //En este paso se instancia el script, así que lo hacemos aquí, y ejecutamos el init de él.
                    if(!scriptComponent.instance){
                        scriptComponent.instance = scriptComponent.instantiate();
                        scriptComponent.instance->entity = entity;
                        scriptComponent.instance->coordinator = gCoordinator.get(); //.get devuelve el puntero puro (*) en vez del unique_ptr
                        scriptComponent.instance->Init();
                    }
                }
            }

            void update() {
                for(auto const& entity: entities){
                    auto& scriptComponent = gCoordinator->getComponent<ilen::ecs::components::ScriptingBridgeComponent>(entity);
                    if(scriptComponent.instance){
                        scriptComponent.instance->Update();
                    }
                }
            }

            void fixedUpdate(){
                for(auto const& entity: entities){
                    auto& scriptComponent = gCoordinator->getComponent<ilen::ecs::components::ScriptingBridgeComponent>(entity);
                    if(scriptComponent.instance){
                        scriptComponent.instance->FixedUpdate();
                    }
                }
            }

            void lateUpdate(){
                for(const auto& entity: entities){
                    auto& scriptComponent = gCoordinator->getComponent<ilen::ecs::components::ScriptingBridgeComponent>(entity);
                    if(scriptComponent.instance){
                        scriptComponent.instance->LateUpdate();
                    }
                }
            }
    };
}