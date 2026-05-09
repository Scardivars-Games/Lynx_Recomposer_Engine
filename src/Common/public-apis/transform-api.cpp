#include <ilen-public/API/transform.h>

using namespace ilen::engine::api;
using namespace ilen::ecs::components;

// transform.cpp
void TransformAPI::Init(const ilen::EngineContext* context) {
    p_coordinator = context->p_coordinator.get();
    size_t numThreads = context->p_jobSystem->AvailableThreads();
    s_threadAccumulators.resize(numThreads + 1);
    
    p_coordinator->registerComponent<components::TransformOrder>();

    for (auto& ta : s_threadAccumulators) {
        ta.orders.resize(ilen::ecs::MAX_ENTITIES);           // pre-alocado
        ta.touched.reserve(ilen::ecs::MAX_ENTITIES / numThreads + 1024);
    }
}

void TransformAPI::Translate(Entity e, vec3 delta) {
    accumulate(e, [&](TransformOrder& o){ o.translation += delta; });
}

void TransformAPI::Rotate(Entity e, vec3 euler) {
    accumulate(e, [&](TransformOrder& o){ o.rotation += euler; });
}

void TransformAPI::Rotate(Entity e, vec3 axis, float angle) {
    accumulate(e, [&](TransformOrder& o){ o.rotation += axis * angle; });
}

void TransformAPI::Scale(Entity e, vec3 factor) {
    accumulate(e, [&](TransformOrder& o){ o.scaleMultiplier *= factor; });
}

void TransformAPI::Scale(Entity e, float uniform) {
    accumulate(e, [&](TransformOrder& o){ o.scaleMultiplier *= uniform; });
}

void TransformAPI::ExecuteAllAccumulatedTransform() {
    if (!p_coordinator) return;

    auto* orderArray = p_coordinator->getComponentArray<components::TransformOrder>();
    if(!orderArray) return;

    std::vector<Entity> registeredEntities;
    std::vector<TransformOrder> accumulatedOrders;
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
                actualOrder.translation += order.translation;
                actualOrder.rotation += order.rotation;
                actualOrder.scaleMultiplier *= order.scaleMultiplier;
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