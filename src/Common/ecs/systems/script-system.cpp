#if !ILEN_ONLY_ECS

#include <ilen/game-logic/script-system.h>
#include <ilen/core/engine-context.h>


namespace ilen::ecs::systems {
    void ScriptingSystem::Init(const EngineContext *engineContext) {
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


    void ScriptingSystem::Update() {
        {
            ilen::time::ScopedTimer timer("Scripting Update method");
            for(auto const& entity: entities){
                auto scriptComponent = m_Coordinator->getComponent<ilen::ecs::components::ScriptingBridgeComponent>(entity);
                if(scriptComponent->instance){
                    scriptComponent->instance->Update();
                }
            }
        }
    }

    void ScriptingSystem::FixedUpdate() const {
        for(auto const& entity: entities){
            auto scriptComponent = m_Coordinator->getComponent<ilen::ecs::components::ScriptingBridgeComponent>(entity);
            if(scriptComponent->instance){
                scriptComponent->instance->FixedUpdate();
            }
        }
    }

    void ScriptingSystem::LateUpdate() const {
        for(const auto& entity: entities){
            auto scriptComponent = m_Coordinator->getComponent<ilen::ecs::components::ScriptingBridgeComponent>(entity);
            if(scriptComponent->instance){
                scriptComponent->instance->LateUpdate();
            }
        }
    }
}

#endif //!ILEN_ONLY_ECS