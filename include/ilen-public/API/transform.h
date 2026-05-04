#pragma once
#include <ilen/ecs/types.h>
#include <ilen/transform/transform-component.h>
#include <ilen/core/thread-context.h>
#include <vector>

namespace ilen::engine::api {
    using namespace ilen::ecs;

    class TransformAPI {
        public:
            static void Init(const ilen::EngineContext* context);

            static void Translate(Entity entity, vec3 delta);
            static void Rotate(Entity entity, vec3 eulerAngles);
            static void Rotate(Entity entity, vec3 axis, float angle);
            static void Scale(Entity entity, vec3 scaleFactor);
            static void Scale(Entity entity, float uniformScale);

            static void ExecuteAllAccumulatedTransform();

        private:
            static inline ilen::ecs::ECSCoordinator* p_coordinator = nullptr;

            // Estructura densa por hilo (la más rápida posible)
            struct ThreadAccumulator {
                std::vector<components::TransformOrder> orders;
                std::vector<Entity>         touched;   // solo las que se modificaron
            };

            static inline std::vector<ThreadAccumulator> s_threadAccumulators;

            // Helper interno para acumular sin duplicar código
            template<typename Func>
            static inline void accumulate(Entity entity, Func&& modifier) {
                auto& ta = s_threadAccumulators[ilen::core::threading::t_ThreadIndex];
                auto& order = ta.orders[entity];

                if (!order.hasOrders) {
                    ta.touched.push_back(entity);
                    order.hasOrders = true;
                }
                modifier(order);
            }
    };

} // namespace ilen::engine::api