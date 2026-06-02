#include <ilen_public/API/physics2d_api.h>

using namespace ilen::engine::api;
using namespace ilen::ecs;
using namespace components;

void PhysicsAPI::Init(const ilen::EngineContext* context){
    p_coordinator = context->p_coordinator.get();

    size_t numThreads = context->p_jobSystem->AvailableThreads();
    s_threadAccumulators.resize(numThreads + 1);
    
    p_coordinator->registerComponent<components::physics::PhysicsOrder>();

    for (auto& ta : s_threadAccumulators) {
        ta.orders.resize(ilen::ecs::MAX_ENTITIES);           // pre-alocado
        ta.touched.reserve(ilen::ecs::MAX_ENTITIES / numThreads + 1024);
    }
}

void PhysicsAPI::GenerateBoxColliderSize(Entity e, physics::BoxCollider2D* collider){
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

void PhysicsAPI::ExecuteAllAccumulatedForces() {
    if (!p_coordinator) return;

    auto* orderArray = p_coordinator->getComponentArray<components::physics::PhysicsOrder>();
    if(!orderArray) return;

    std::vector<Entity> registeredEntities;
    std::vector<physics::PhysicsOrder> accumulatedOrders;
    registeredEntities.reserve(ilen::ecs::MAX_ENTITIES);
    accumulatedOrders.reserve(ilen::ecs::MAX_ENTITIES);


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
                actualOrder.accumulatedForce += order.accumulatedForce;
                actualOrder.accumulatedImpulse += order.accumulatedImpulse;
            }else {
                //No existe.
                registeredEntities.push_back(e);
                accumulatedOrders.push_back(order);
            }

            order = physics::PhysicsOrder{};
        }
        ta.touched.clear();
    }

    for(size_t i = 0; i < registeredEntities.size(); i++){
        orderArray->updateData(registeredEntities[i], std::move(accumulatedOrders[i]));
    }
}