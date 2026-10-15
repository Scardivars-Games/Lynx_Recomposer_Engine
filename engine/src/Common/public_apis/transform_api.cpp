#include <lynx_public/API/transform_api.h>
#include <lynx_public/API/physics_2d_api.h>
#include <lynx/physics/components/specification/rigidbody_components.h>
#include <lynx/transform/transform_system.h>

using namespace lynx::engine::api;
using namespace lynx::ecs::components;

// transform.cpp
void TransformAPI::Init(const lynx::EngineContext* context) {
    p_coordinator = context->p_coordinator.get();
    size_t numThreads = context->p_jobSystem->AvailableThreads();
    s_threadAccumulators.resize(numThreads + 1);
    p_runtimeConfig = context->runtimeConfiguration.get();
    
    p_coordinator->registerComponent<components::TransformOrder>();
    p_transformSys = p_coordinator->getSystem<systems::TransformSystem>().get();

    for (auto& ta : s_threadAccumulators) {
        ta.orders.resize(p_runtimeConfig->ecs.initialEntityCapacity);           // pre-alocado
        ta.touched.reserve(p_runtimeConfig->ecs.initialEntityCapacity / numThreads + 1024);
    }
}

void TransformAPI::Translate(Entity e, vec3 delta) {
    if(e == NULL_ENTITY) return;

    auto* rigidbodyArray =
        p_coordinator->getComponentArray<components::physics::Rigidbody2D>();
    auto* rigidbody = rigidbodyArray ? rigidbodyArray->getData(e) : nullptr;

    if(rigidbody) {
        if(rigidbody->isStatic) {
#if ILEN_DEBUG
            ILEN_ERROR_P(
                "TRANSFORM_API",
                "La entidad estatica %u no puede moverse mediante Translate",
                e
            );
#endif
            return;
        }

        PhysicsAPI::Move(e, static_cast<vec2>(delta));

        // La física es 2D; Z continúa perteneciendo al Transform.
        if(delta.z != 0.0f) {
            accumulate(e, [&](TransformOrder& order) {
                order.translation.z += delta.z;
            });
        }
        return;
    }

    accumulate(e, [&](TransformOrder& order) {
        order.translation += delta;
    });
}

void TransformAPI::Rotate(Entity e, vec3 euler) {
    if(e == NULL_ENTITY) return;

    auto* rigidbodyArray =
        p_coordinator->getComponentArray<components::physics::Rigidbody2D>();
    auto* rigidbody = rigidbodyArray ? rigidbodyArray->getData(e) : nullptr;

    if(rigidbody) {
        if(rigidbody->isStatic) {
#if ILEN_DEBUG
            ILEN_ERROR_P(
                "TRANSFORM_API",
                "La entidad estatica %u no puede rotar mediante Rotate",
                e
            );
#endif
            return;
        }

        if(euler.z != 0.0f) {
            PhysicsAPI::Rotate(e, euler.z);
        }

        // La física 2D solamente gobierna el eje Z.
        if(euler.x != 0.0f || euler.y != 0.0f) {
            accumulate(e, [&](TransformOrder& order) {
                order.rotation.x += euler.x;
                order.rotation.y += euler.y;
            });
        }
        return;
    }

    accumulate(e, [&](TransformOrder& order) {
        order.rotation += euler;
    });
}

void TransformAPI::Rotate(Entity e, vec3 axis, float angle) {
    Rotate(e, axis * angle);
}

void TransformAPI::Scale(Entity e, vec3 factor) {
    if(e != NULL_ENTITY){
            accumulate(e, [&](TransformOrder& o){ o.scaleMultiplier *= factor; o.isNewScale = true;});
    }
}

void TransformAPI::Scale(Entity e, float uniform) {
    if(e != NULL_ENTITY){
            accumulate(e, [&](TransformOrder& o){ o.scaleMultiplier *= uniform; o.isNewScale = true;});
    }
}

void TransformAPI::SetPosition(Entity e, vec3 newPos){
    if(e == NULL_ENTITY) return;

    auto* rigidbodyArray =
        p_coordinator->getComponentArray<components::physics::Rigidbody2D>();
    auto* rigidbody = rigidbodyArray ? rigidbodyArray->getData(e) : nullptr;

    if(rigidbody) {
        if(rigidbody->isStatic) {
#if ILEN_DEBUG
            ILEN_ERROR_P(
                "TRANSFORM_API",
                "La entidad estatica %u no puede moverse mediante SetPosition",
                e
            );
#endif
            return;
        }

        PhysicsAPI::Teleport(e, static_cast<vec2>(newPos));

        auto* transform = p_coordinator->getComponent<TransformComponent>(e);
        if(transform && transform->position.z != newPos.z) {
            const vec3 zOnlyPosition{
                transform->position.x,
                transform->position.y,
                newPos.z
            };

            accumulate(e, [&](TransformOrder& order) {
                order.translation = zOnlyPosition;
                order.isNewPos = true;
            });
        }
        return;
    }

    accumulate(e, [&](TransformOrder& order) {
        order.translation = newPos;
        order.isNewPos = true;
    });
}

//Este método modifica el sistema de parenting del transform.
//entity es la entidad a la cual se le va a añadir una entidad hijo. En otras palabras, es la entidad padre.
//newChildren es la entidad nueva que cambia de padre y se hace hijo de entity
void TransformAPI::AddChildren(Entity entity, Entity newChildren){
    if(entity != NULL_ENTITY && newChildren != NULL_ENTITY){
        auto* transformArray = p_coordinator->getComponentArray<TransformComponent>();
        auto* transformParent =  transformArray->getData(entity);
        auto* transformChildren = transformArray->getData(newChildren);
        auto it = std::ranges::find(transformParent->childrens, newChildren);

        if(it == transformParent->childrens.end()){
            //Esta entidad no existe entre sus hijos
            transformParent->childrens.push_back(newChildren);
            transformChildren->parent = entity;
        }
    }
}

vec3 TransformAPI::GetEntityPosition(Entity entity) {
    if(entity != NULL_ENTITY){
        return p_coordinator->getComponent<TransformComponent>(entity)->position;
    }else {
        return vec3(0.0,0.0,0.0);
    }
}

void TransformAPI::ExecuteAllAccumulatedTransform() {
    if (!p_coordinator) return;

    auto* orderArray = p_coordinator->getComponentArray<components::TransformOrder>();
    if(!orderArray) return;

    std::vector<Entity> registeredEntities;
    std::vector<TransformOrder> accumulatedOrders;
    registeredEntities.reserve(p_runtimeConfig->ecs.initialEntityCapacity);
    accumulatedOrders.reserve(p_runtimeConfig->ecs.initialEntityCapacity);


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
                actualOrder.translation += order.translation;
                actualOrder.rotation += order.rotation;
                actualOrder.scaleMultiplier *= order.scaleMultiplier;
                if(order.isNewPos)
                    actualOrder.isNewPos = true;
                if(order.isNewRot)
                    actualOrder.isNewRot = true;
                if(order.isNewScale)
                    actualOrder.isNewScale = true;
            }else {
                //No existe.
                registeredEntities.push_back(e);
                accumulatedOrders.push_back(order);
            }

            order = TransformOrder{};
        }
        ta.touched.clear();
    }

    for(size_t i = 0; i < registeredEntities.size(); i++){
        orderArray->updateData(registeredEntities[i], std::move(accumulatedOrders[i]));
    }
}

void TransformAPI::ExecuteOrdersInTheTransformSystem(){
    p_transformSys->ExecuteTransformOrders();
}
