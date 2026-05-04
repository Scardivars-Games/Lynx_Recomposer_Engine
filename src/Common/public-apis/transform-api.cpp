#include <ilen-public/API/transform.h>

using namespace ilen::engine::api;
using namespace ilen::ecs::components;

// transform.cpp
void TransformAPI::Init(const ilen::EngineContext* context) {
    p_coordinator = context->p_coordinator.get();
    size_t numThreads = context->p_jobSystem->AvailableThreads();
    s_threadAccumulators.resize(numThreads + 1);

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

    for (auto& ta : s_threadAccumulators) {
        for (Entity e : ta.touched) {
            auto& order = ta.orders[e];
            if (order.hasOrders) {
                p_coordinator->setComponent(e, std::move(order));
            }
        }

        // Reset solo de las tocadas (muy rápido)
        for (Entity e : ta.touched) {
            ta.orders[e] = TransformOrder{};   // reset a default
        }
        ta.touched.clear();
    }
}