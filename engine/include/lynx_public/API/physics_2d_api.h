#pragma once

#include <lynx/ecs/types.h>
#include <lynx/headers/ecs_components.h>
#include <lynx/core/thread_context.h>
#include <lynx/core/runtime_configuration.h>

namespace lynx::ecs::systems {
    class ScriptingSystem;
}

namespace lynx::engine::api {
    using namespace lynx::ecs;

    class PhysicsAPI {
        public:
            static void Init(const lynx::EngineContext* context);

            // Fuerza continua
            static void AddForce(Entity entity, vec3 force);
            
            // Fuerza instantánea
            static void AddImpulse(Entity entity, vec3 impulse);

            // Movimiento gobernado por físicas. A diferencia de modificar el
            // Transform, esta velocidad participa en broadphase, swept SAT y
            // resolución por masa.
            static void SetLinearVelocity(Entity entity, vec2 velocity);
            static void SetLinearVelocityX(Entity entity, float velocity);
            static void SetLinearVelocityY(Entity entity, float velocity);

            // Desplazamiento relativo o destino absoluto procesado por CCD.
            static void Move(Entity entity, vec2 translation);
            static void MovePosition(Entity entity, vec2 targetPosition);

            // Rotación 2D gobernada por físicas, expresada en grados y
            // procesada mediante CCD angular.
            static void Rotate(Entity entity, float degrees);
            static void RotateTo(Entity entity, float targetDegrees);

            // Cambio instantáneo: omite el recorrido pero resuelve contactos
            // que existan en la posición de destino.
            static void Teleport(Entity entity, vec2 targetPosition);
            static void TeleportRotation(Entity entity, float targetDegrees);

            //Modificación de propiedades físicas de las entidades
            static void ChangeObjectGravity(Entity entity, bool newState);
            static void ChangeObjectGravity(Entity entity, float newMultiplier);
            static void ChangeObjectGravity(Entity entity, bool newState, float newMultiplier);

            static void ChangeObjectMass(Entity entity, float newMass, bool additive = false);

            //Modificaciónes globales
            static void ChangeGlobalGravity(float newGravityValue);

            static void GenerateBoxColliderSize(Entity e, components::physics::BoxCollider2D* collider);
            static void GenerateBoxColliderSize(Entity e, components::physics::BoxCollider3D* collider);
        private:
            static inline lynx::ecs::ECSCoordinator* p_coordinator = nullptr;
            static inline lynx::core::RuntimeConfiguration* p_runtimeOptions = nullptr;

            struct ThreadAccumulator {
                std::vector<components::physics::PhysicsOrder> orders;
                std::vector<Entity> touched;
            };

            static inline std::vector<ThreadAccumulator> s_threadAccumulators;

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

            // Flush de datos al ECS
            static void ExecuteAllAccumulatedForces();

        friend class lynx::ecs::systems::ScriptingSystem;
    };
}
