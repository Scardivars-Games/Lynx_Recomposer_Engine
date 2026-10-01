#pragma once

#include <ilen/ecs/types.h>
#include <ilen/headers/ecs_components.h>
#include <ilen/core/thread_context.h>

namespace ilen::ecs::systems {
    class ScriptingSystem;
}

namespace ilen::engine::api {
    using namespace ilen::ecs;

    class PhysicsAPI {
        public:
            static void Init(const ilen::EngineContext* context);

            // Fuerza continua
            static void AddForce(Entity entity, vec3 force);
            
            // Fuerza instantánea
            static void AddImpulse(Entity entity, vec3 impulse);

            //Modificación de propiedades físicas de las entidades
            static void ChangeObjectGravity(Entity entity, bool newState);
            static void ChangeObjectGravity(Entity entity, float newMultiplier);
            static void ChangeObjectGravity(Entity entity, bool newState, float newMultiplier);

            static void ChangeObjectMass(Entity entity, float newMass);

            //Modificaciónes globales
            static void ChangeGlobalGravity(float newGravityValue);

            static void GenerateBoxColliderSize(Entity e, components::physics::BoxCollider2D* collider);
            static void GenerateBoxColliderSize(Entity e, components::physics::BoxCollider3D* collider);
        private:
            static inline ilen::ecs::ECSCoordinator* p_coordinator = nullptr;

            struct ThreadAccumulator {
                std::vector<components::physics::PhysicsOrder> orders;
                std::vector<Entity> touched;
            };

            static inline std::vector<ThreadAccumulator> s_threadAccumulators;

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

            // Flush de datos al ECS
            static void ExecuteAllAccumulatedForces();

        friend class ilen::ecs::systems::ScriptingSystem;
    };
}