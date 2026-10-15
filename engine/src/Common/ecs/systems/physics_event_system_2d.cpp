
#if !ILEN_ONLY_ECS

#include <lynx/game_logic/physics_event_system_2d.h>
#include <lynx/game_logic/game_object.h>
#include <lynx/game_logic/ecs_bridge.h>
#include <lynx/physics/components/specification/collision_components.h>

namespace lynx::ecs::systems {
    using lynx::ecs::components::ScriptingBridgeComponent;
    using namespace lynx::ecs::components::physics;

    namespace {
        void DispatchContact (lynx::game::GameObject& gameObject, const ContactManifold2D& contact) {
            if(contact.isTrigger) {
                switch (contact.phase) {
                    case CollisionPhase2D::Enter:
                        gameObject.OnTriggerEnter(contact);
                        break;
                    case CollisionPhase2D::Stay:
                        gameObject.OnTriggerStay(contact);
                        break;
                    case CollisionPhase2D::Exit:
                        gameObject.OnTriggerExit(contact);
                        break;
                }

                return;
            }

            switch (contact.phase) {
                case CollisionPhase2D::Enter:
                    gameObject.OnCollisionEnter(contact);
                    break;
                case CollisionPhase2D::Stay:
                    gameObject.OnCollisionStay(contact);
                    break;
                case CollisionPhase2D::Exit:
                    gameObject.OnCollisionExit(contact);
                    break;
            }
        }
    }

    void PhysicsEventSystem2D::Init(const lynx::EngineContext *engineContext) {
        p_coordinator = engineContext->p_coordinator.get();
        p_jobSystem = engineContext->p_jobSystem.get();
        p_commandDispatcher = engineContext->p_commandDispatcher.get();
    }

    void PhysicsEventSystem2D::Update() {
        auto* scriptArray = p_coordinator->getComponentArray<ScriptingBridgeComponent>();
        auto* collisionArray = p_coordinator->getComponentArray<CollisionComponent2D>();

        if(!scriptArray || !collisionArray) return;

        for(Entity entity: entities) {
            auto* bridge = scriptArray->getData(entity);
            auto* collision = collisionArray->getData(entity);

            if(!bridge || !bridge->instance || !collision) continue;

            //Esto es para evitar que el gameobject sea destruido mientras se ejecutan los callbacks de físicas
            const auto gameObject = bridge->instance;

            for(const ContactManifold2D& contact : collision->contacts) {
                DispatchContact(*gameObject, contact);
            }
        }
    }
}

#endif