#pragma once
#include <lynx/ecs/types.h>
#include <lynx/transform/transform_component.h>
#include <lynx/core/thread_context.h>
#include <vector>
#include <lynx/core/runtime_configuration.h>

//Declaración adelantada
namespace lynx::ecs::systems {
    class ScriptingSystem;
}

namespace lynx::engine::api {
    using namespace lynx::ecs;
    using namespace lynx::maths::vector;

    class TransformAPI {
        public:
            static void Init(const lynx::EngineContext* context);

            static void Translate(Entity entity, vec3 delta);
            static void SetPosition(Entity entity, vec3 newPosition);
            static void Rotate(Entity entity, vec3 eulerAngles);
            static void Rotate(Entity entity, vec3 axis, float angle);
            static void Scale(Entity entity, vec3 scaleFactor);
            static void Scale(Entity entity, float uniformScale);

            static void AddChildren(Entity entity, Entity newChildren);
            static vec3 GetEntityPosition(Entity entity);

        private:
            static inline lynx::ecs::ECSCoordinator* p_coordinator = nullptr;
            static inline lynx::ecs::systems::TransformSystem* p_transformSys = nullptr;
            static inline lynx::core::RuntimeConfiguration* p_runtimeConfig = nullptr;

            // Estructura densa por hilo (la más rápida posible)
            struct ThreadAccumulator {
                std::vector<components::TransformOrder> orders;
                std::vector<Entity>         touched;   // solo las que se modificaron
            };

            static inline std::vector<ThreadAccumulator> s_threadAccumulators;

            // Helper interno para acumular sin duplicar código
            template<typename Func>
            static inline void accumulate(Entity entity, Func&& modifier) {
                auto& ta = s_threadAccumulators[lynx::core::threading::t_ThreadIndex];
                auto& order = ta.orders[entity];

                if (!order.hasOrders) {
                    ta.touched.push_back(entity);
                    order.hasOrders = true;
                }
                modifier(order);
            }

            static void ExecuteAllAccumulatedTransform();
            static void ExecuteOrdersInTheTransformSystem();

        friend class lynx::ecs::systems::ScriptingSystem;
    };

} // namespace lynx::engine::api