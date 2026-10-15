#include <lynx_public/API/physics_2d_api.h>
#include <lynx/physics/components/specification/rigidbody_components.h>

using namespace lynx::engine::api;
using namespace lynx::ecs;
using namespace lynx::ecs::components;
using namespace lynx::ecs::components::physics;

void PhysicsAPI::Init(const lynx::EngineContext* context){
    p_coordinator = context->p_coordinator.get();
    p_runtimeOptions = context->runtimeConfiguration.get();

    size_t numThreads = context->p_jobSystem->AvailableThreads();
    s_threadAccumulators.resize(numThreads + 1);
    
    p_coordinator->registerComponent<components::physics::PhysicsOrder>();

    for (auto& ta : s_threadAccumulators) {
        ta.orders.resize(p_runtimeOptions->ecs.initialEntityCapacity);           // pre-alocado
        ta.touched.reserve(p_runtimeOptions->ecs.initialEntityCapacity / numThreads + 1024);
    }
}

void PhysicsAPI::GenerateBoxColliderSize(Entity e, BoxCollider2D* collider){
    //Si el tamaño no es 0, el box collider ya existe y se respeta.
    if(collider->halfExtents.x != 0.0f && collider->halfExtents.y != 0.0f) return;

    auto* transform = p_coordinator->getComponent<TransformComponent>(e);

    if(!transform){
        //Si no hay transform, se le da una tamaño minimo de seguridad.
        collider->halfExtents = vec2(0.1f);
        collider->offset = vec2(0.0f);
        return;
    }

    //Obtenemos el tamaño del quad de acuerdo a su escala.
    /*float width = std::abs(transform->scale.x);
    float height = std::abs(transform->scale.y);

    collider->halfExtents.x = width * 0.5;
    collider->halfExtents.y = height * 0.5;*/

    collider->halfExtents = vec2(0.5f, 0.5f); //Ya normalizamos, la aplicación de la escala del transform es delegada al ECS.

    collider->offset = vec2(0.0f);
}

void PhysicsAPI::AddForce(Entity e, vec3 force) {
    accumulate(e, [&](components::physics::PhysicsOrder& o) {o.accumulatedForce += force; });
}

void PhysicsAPI::AddImpulse(Entity e, vec3 impulse) {
    accumulate(e, [&](components::physics::PhysicsOrder& o){ o.accumulatedImpulse += impulse; });
}

void PhysicsAPI::SetLinearVelocity(Entity e, vec2 velocity) {
    if(e == NULL_ENTITY) return;

    accumulate(e, [&](components::physics::PhysicsOrder& order) {
        order.velocityOverride = velocity;
        order.overrideVelocityX = true;
        order.overrideVelocityY = true;
    });
}

void PhysicsAPI::SetLinearVelocityX(Entity e, float velocity) {
    if(e == NULL_ENTITY) return;

    accumulate(e, [&](components::physics::PhysicsOrder& order) {
        order.velocityOverride.x = velocity;
        order.overrideVelocityX = true;
    });
}

void PhysicsAPI::SetLinearVelocityY(Entity e, float velocity) {
    if(e == NULL_ENTITY) return;

    accumulate(e, [&](components::physics::PhysicsOrder& order) {
        order.velocityOverride.y = velocity;
        order.overrideVelocityY = true;
    });
}

void PhysicsAPI::Move(Entity e, vec2 translation) {
    if(e == NULL_ENTITY) return;

    accumulate(e, [&](components::physics::PhysicsOrder& order) {
        if(order.hasTeleport || order.hasMovePosition) {
            order.targetPosition += translation;
        } else {
            order.accumulatedTranslation += translation;
        }
    });
}

void PhysicsAPI::MovePosition(Entity e, vec2 targetPosition) {
    if(e == NULL_ENTITY) return;

    accumulate(e, [&](components::physics::PhysicsOrder& order) {
        order.targetPosition = targetPosition;
        order.accumulatedTranslation = vec2(0.0f);
        order.hasMovePosition = true;
        order.hasTeleport = false;
    });
}

void PhysicsAPI::Rotate(Entity e, float degrees) {
    if(e == NULL_ENTITY) return;

    accumulate(e, [&](components::physics::PhysicsOrder& order) {
        if(order.hasTeleportRotation || order.hasRotateTo) {
            order.targetRotation += degrees;
        } else {
            order.accumulatedRotation += degrees;
        }
    });
}

void PhysicsAPI::RotateTo(Entity e, float targetDegrees) {
    if(e == NULL_ENTITY) return;

    accumulate(e, [&](components::physics::PhysicsOrder& order) {
        order.targetRotation = targetDegrees;
        order.accumulatedRotation = 0.0f;
        order.hasRotateTo = true;
        order.hasTeleportRotation = false;
    });
}

void PhysicsAPI::Teleport(Entity e, vec2 targetPosition) {
    if(e == NULL_ENTITY) return;

    accumulate(e, [&](components::physics::PhysicsOrder& order) {
        order.targetPosition = targetPosition;
        order.accumulatedTranslation = vec2(0.0f);
        order.hasMovePosition = false;
        order.hasTeleport = true;
    });
}

void PhysicsAPI::TeleportRotation(Entity e, float targetDegrees) {
    if(e == NULL_ENTITY) return;

    accumulate(e, [&](components::physics::PhysicsOrder& order) {
        order.targetRotation = targetDegrees;
        order.accumulatedRotation = 0.0f;
        order.hasRotateTo = false;
        order.hasTeleportRotation = true;
    });
}

void PhysicsAPI::ChangeObjectGravity(Entity entity, bool newState, float newMultiplier) {
    ChangeObjectGravity(entity, newState);
    ChangeObjectGravity(entity, newMultiplier);
}

void PhysicsAPI::ChangeObjectGravity(Entity entity, bool newState) {
    auto* objectRigidbody = p_coordinator->getComponent<Rigidbody2D>(entity);
    objectRigidbody->usesGravity = newState;
}

void PhysicsAPI::ChangeObjectGravity(Entity entity, float newMultiplier) {
    auto* objectRigidbody = p_coordinator->getComponent<Rigidbody2D>(entity);
    objectRigidbody->gravityScale = newMultiplier;
}

void PhysicsAPI::ChangeGlobalGravity(float newGravityValue) {
    p_runtimeOptions->physics.gravity = newGravityValue;
}

void PhysicsAPI::ChangeObjectMass(Entity entity, float newMass, bool additive) {
    auto* objectRigidbody = p_coordinator->getComponent<Rigidbody2D>(entity);
    if(additive){
        objectRigidbody->mass += newMass;
        if(objectRigidbody->mass <= 0.001f)
            objectRigidbody->mass = 0.1f;
    }else {
        objectRigidbody->mass = newMass;
        if(objectRigidbody->mass <= 0.001f)
            objectRigidbody->mass = 0.1f;
    }
}

void PhysicsAPI::ExecuteAllAccumulatedForces() {
    if (!p_coordinator) return;

    auto* orderArray = p_coordinator->getComponentArray<components::physics::PhysicsOrder>();
    if(!orderArray) return;

    const auto mergeOrder = [](
        PhysicsOrder& destination,
        const PhysicsOrder& source
    ) {
        destination.accumulatedForce += source.accumulatedForce;
        destination.accumulatedImpulse += source.accumulatedImpulse;

        // Una asignación de velocidad es un override, no una suma. La orden
        // más reciente de cada eje tiene prioridad.
        if(source.overrideVelocityX) {
            destination.velocityOverride.x = source.velocityOverride.x;
            destination.overrideVelocityX = true;
        }

        if(source.overrideVelocityY) {
            destination.velocityOverride.y = source.velocityOverride.y;
            destination.overrideVelocityY = true;
        }

        if(source.hasTeleport) {
            destination.targetPosition = source.targetPosition;
            destination.accumulatedTranslation = vec2(0.0f);
            destination.hasMovePosition = false;
            destination.hasTeleport = true;
        } else if(source.hasMovePosition) {
            destination.targetPosition = source.targetPosition;
            destination.accumulatedTranslation = vec2(0.0f);
            destination.hasMovePosition = true;
            destination.hasTeleport = false;
        } else {
            destination.accumulatedTranslation +=
                source.accumulatedTranslation;
        }

        if(source.hasTeleportRotation) {
            destination.targetRotation = source.targetRotation;
            destination.accumulatedRotation = 0.0f;
            destination.hasRotateTo = false;
            destination.hasTeleportRotation = true;
        } else if(source.hasRotateTo) {
            destination.targetRotation = source.targetRotation;
            destination.accumulatedRotation = 0.0f;
            destination.hasRotateTo = true;
            destination.hasTeleportRotation = false;
        } else {
            destination.accumulatedRotation += source.accumulatedRotation;
        }

        destination.hasOrders = true;
    };

    std::vector<Entity> registeredEntities;
    std::vector<PhysicsOrder> accumulatedOrders;
    registeredEntities.reserve(p_runtimeOptions->ecs.initialEntityCapacity);
    accumulatedOrders.reserve(p_runtimeOptions->ecs.initialEntityCapacity);


    for (auto& ta : s_threadAccumulators) {
        for (Entity e : ta.touched) {
            /*auto& order = ta.orders[e];
            if (!order.hasOrders) continue;

            orderArray->updateData(e, std::move(order));
            order = TransformOrder{};*/

            auto& order = ta.orders[e];
            if(!order.hasOrders) continue;

            //Ver si esa entidad ya esta registrada
            auto it = std::ranges::find(registeredEntities, e);
            if(it != registeredEntities.end()){
                //Existe, vamos a obtener el indice.
                size_t index = std::distance(registeredEntities.begin(), it);
                auto& actualOrder = accumulatedOrders[index];
                mergeOrder(actualOrder, order);
            }else {
                //No existe.
                registeredEntities.push_back(e);
                accumulatedOrders.push_back(order);
            }

            order = PhysicsOrder{};
        }
        ta.touched.clear();
    }

    for(size_t i = 0; i < registeredEntities.size(); i++){
        const Entity entity = registeredEntities[i];
        auto* pendingOrder = orderArray->getData(entity);

        // A una frecuencia de render mayor que PhysicsUpdateTime pueden pasar
        // varios Update antes del siguiente FixedUpdate. La orden publicada
        // anteriormente todavía no fue consumida y debe combinarse con la
        // nueva; reemplazarla hacía que a 120 FPS se perdiera una de cada dos
        // traslaciones y rotaciones calculadas con deltaTime.
        if(pendingOrder && pendingOrder->hasOrders) {
            mergeOrder(*pendingOrder, accumulatedOrders[i]);
        } else {
            orderArray->updateData(
                entity,
                std::move(accumulatedOrders[i])
            );
        }
    }
}
