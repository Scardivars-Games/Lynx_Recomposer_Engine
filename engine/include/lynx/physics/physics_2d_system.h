#pragma once

#include <cstddef>
#include <lynx/physics/components/physics_components.h>
#include <lynx/transform/transform_component.h>
#include <lynx/ecs/system.h>
#include <lynx/ecs/ecs.h>
#include <lynx/ecs/system_registry.h>
#include <lynx/physics/collision_matrix.h>
#include <vector>
#include <lynx/physics/components/physics_components.h>
#include <lynx/physics/collider_proxy_builder_2d.h>
#include <lynx/physics/narrow_phase_2d.h>
#include <lynx/physics/broad_phase_2d.h>
#include <optional>
#include <unordered_set>
#include "lynx/ecs/types.h"

namespace lynx::core {
    struct PhysicsOptions;
}

namespace lynx::ecs::systems {
    using namespace lynx::ecs::components;
    using namespace lynx::maths::aabb;

    class PhysicsSystem2D final: public lynx::ecs::ISystem {
        private:
            struct ArrayOfComponentsUsed {
                ComponentArray<physics::Rigidbody2D>* rigidbodyComponentArray = nullptr;
                ComponentArray<physics::CollisionComponent2D>* collisionComponentArray = nullptr;
                ComponentArray<physics::BoxCollider2D>* boxColliderArray = nullptr;
                ComponentArray<physics::ConvexPolygonCollider2D>* convexPolygonCollider = nullptr;
                ComponentArray<TransformComponent>* transformComponentArray = nullptr;
                ComponentArray<TransformOrder>* transformOrderArray = nullptr;
                ComponentArray<physics::PhysicsOrder>* physicsOrderArray = nullptr;
                ComponentArray<EntityMetadataComponent>* entityMetadataArray = nullptr;
            };

            enum class RegistrationState: uint8_t {
                None,
                Dynamic,
                Static
            };

            const lynx::core::PhysicsOptions* p_physicsOptions = nullptr;
            Entity maxEntities = 0;

        public:
            //Metodos obligatorios por ISystem
            void Init(const lynx::EngineContext* engineContext) override; //Obtiene punteros de los sistemas base del motor, y un puntero al coordinador ECS, para acceder a otros sistemas del ECS, o a componentes y entidades, etc.
            void Update() override; //Se ejecuta cada frame, en este caso, en un FixedUpdate.

            //funciones helpers del sistema
            void registerEntity(Entity e, ArrayOfComponentsUsed* componentsArray);
            void unregisterEntity(Entity e);

#if LYNX_DEBUG
            struct DebugImpact2D {
                vec2 point{0.0f};
                vec2 direction{0.0f};
                float force = 0.0f;
                float z = 0.0f;
                Entity entityA = NULL_ENTITY;
                Entity entityB = NULL_ENTITY;
            };

            const std::vector<DebugImpact2D>& GetPersistentDebugImpacts() const {
                return m_persistentDebugImpacts;
            }

            enum class DebugActiveVectorKind2D : uint8_t {
                LinearVelocity,
                AngularVelocity,
                Force
            };

            /**
             * Vector que estuvo activo durante el fixed update más reciente.
             *
             * A diferencia de DebugImpact2D, estos datos no son históricos:
             * se reconstruyen en cada paso para que una flecha desaparezca en
             * cuanto el cuerpo deje de tener esa velocidad o fuerza.
             */
            struct DebugActiveVector2D {
                vec2 point{0.0f};
                vec2 value{0.0f};
                float z = 0.0f;
                Entity entity = NULL_ENTITY;
                DebugActiveVectorKind2D kind =
                    DebugActiveVectorKind2D::LinearVelocity;
            };

            const std::vector<DebugActiveVector2D>&
            GetDebugActiveVectors() const {
                return m_debugActiveVectors;
            }
#endif

#ifdef LYNX_DEBUG
            void checkStaticGuard(ArrayOfComponentsUsed* componentsArray);
#endif
        private:
            std::vector<Entity> m_dynamicEntities;

            //Array paralelos para los proxies
            std::vector<lynx::physics::ColliderProxy2D> m_dynamicProxies;
            std::vector<lynx::physics::ColliderProxy2D> m_staticProxies;

            std::vector<RegistrationState> m_registrationStates{};

            // Órdenes transitorias consumidas en un único fixed update. Se
            // guardan fuera del ECS porque sólo son estado interno del solver.
            std::vector<vec2> m_requestedTranslations{};
            std::vector<vec2> m_teleportTargets{};
            std::vector<bool> m_hasTeleport{};
            std::vector<float> m_requestedRotations{};
            std::vector<float> m_teleportRotationTargets{};
            std::vector<bool> m_hasTeleportRotation{};

            // Máscara transitoria de los ejes cuya velocidad está siendo
            // alimentada durante el fixed update actual. Bit 0 representa X
            // y bit 1 representa Y. La resistencia sólo actúa sobre los ejes
            // que no recibieron una orden de velocidad, fuerza, impulso o
            // aceleración continua como la gravedad.
            std::vector<uint8_t> m_activeVelocityAxes{};

            // Collider estático que mantiene bloqueada la rotación solicitada
            // de cada entidad. NULL_ENTITY significa que puede volver a girar.
            std::vector<Entity> m_rotationBlockingStatic{};


            //Esto se podria cambiar, pero necesito terminar ya, así que es un posible vector para optimizacion
            std::unordered_map<lynx::ecs::Entity, TransformOrder> m_accumulatedTransformOrder;

            //Debido al que sistema de físicas es de los más complejos, vamos a dividir las diferentes
            //partes del proceso en funciones especializadas, de esa forma es más facil entender el flujo
            //del sistema.
            void syncEntities(ArrayOfComponentsUsed* componentsArray);
            void prepareCollisionData(ArrayOfComponentsUsed* componentsArray);
            void registerPrevPosition(ArrayOfComponentsUsed* ComponentArray);
            void applyForces(float dt, ArrayOfComponentsUsed* ComponentArray);
            void applyGravity(float dt, ArrayOfComponentsUsed* componentsArray);
            void applyMovementResistance(
                float dt,
                ArrayOfComponentsUsed* componentsArray
            );
            void integrate(float dt, ArrayOfComponentsUsed* componentsArray);
            void rebuildDynamicProxies(float dt, ArrayOfComponentsUsed* componentsArray);
            void detectAndResolve(float dt, ArrayOfComponentsUsed* componentsArray);
            void refreshRotationBlocks();

            // Prepara el movimiento restante y su volumen barrido para CCD.
            void prepareSweeps(float remainingTime, ArrayOfComponentsUsed* componentsArray);

            // Avanza todos los cuerpos dinámicos la misma fracción temporal.
            void advanceDynamicProxies(float timeFraction);

            //Construir Proxies
            std::optional<lynx::physics::ColliderProxy2D> buildProxy(Entity entity, ArrayOfComponentsUsed* componentsArray);

            // Devuelve la pose que tendrá la entidad después de consumir la
            // TransformOrder pendiente. Es imprescindible cuando un frame de
            // render contiene varios fixed updates: el segundo paso debe
            // comenzar donde terminó el primero, aunque TransformSystem aún
            // no haya escrito el resultado en TransformComponent.
            static TransformComponent effectiveTransform(
                const TransformComponent& transform,
                const TransformOrder* pendingOrder
            );

            //Corrige colisiones
            void resolveCollision(
                lynx::physics::ColliderProxy2D& proxyA, 
                lynx::physics::ColliderProxy2D& proxyB,
                const physics::ContactManifold2D& manifold,
                ArrayOfComponentsUsed* componentsArray
            );

            //Escribe el CollisionComponent2D
            void writeContacts(
                const physics::ContactManifold2D& manifold,
                ArrayOfComponentsUsed* componentsArray
            );

            static void translateProxy(
                lynx::physics::ColliderProxy2D& proxy,
                const vec2& translation
            );

            static void rotateProxy(
                lynx::physics::ColliderProxy2D& proxy,
                float radians
            );

            static AABB2D calculateSweptBounds(
                const lynx::physics::ColliderProxy2D& proxy,
                const vec2& translation
            );

            lynx::physics::BroadPhase2D m_broadPhase{15.0f};
            bool m_staticBroadPhaseDirty = true;
#if LYNX_DEBUG
            struct StaticGuard {
                Entity entity = NULL_ENTITY;

                vec3 originalPosition{0.0f};
                vec3 originalRotation{0.0f};
                vec3 originalScale{1.0f};

                int frameCounter = 0;
                int checkInterval = 60;
            };

            std::vector<StaticGuard> m_staticGuards;

            std::vector<DebugImpact2D> m_persistentDebugImpacts;
            std::vector<DebugActiveVector2D> m_debugActiveVectors;
            std::vector<vec2> m_debugAppliedForces{};
            std::unordered_set<uint64_t> m_debugPreviousCollisionPairs;
            std::unordered_set<uint64_t> m_debugCurrentCollisionPairs;
            float m_debugFixedDeltaTime = 0.0f;

            void recordDebugImpact(
                const lynx::physics::ColliderProxy2D& proxyA,
                const lynx::physics::ColliderProxy2D& proxyB,
                const physics::ContactManifold2D& manifold,
                ArrayOfComponentsUsed* componentsArray
            );

            void rebuildDebugActiveVectors(
                ArrayOfComponentsUsed* componentsArray
            );

            struct CollisionDebugStats {
                size_t dynamicVsDynamicPairs = 0;
                size_t dynamicVsStaticPairs = 0;

                size_t rejectedByLayer = 0;

                size_t aabbTests = 0;
                size_t rejectedByAABB = 0;

                size_t satTests = 0;
                size_t collisionDetected = 0;

                size_t bruteForcePairs = 0;
                size_t broadPhaseCandidates = 0;
                size_t collisionPasses = 0;

                size_t totalPairs() const {
                    return dynamicVsDynamicPairs + dynamicVsStaticPairs;
                }

                void reset() {
                    *this = CollisionDebugStats{};
                }
            };

            CollisionDebugStats m_collisionDebugStats{};
            
            uint32_t m_collisionDebugFrameCounter = 0;
            uint32_t m_collisionDebugReportInterval = 60;

            void reportCollisionDebugStats();
#endif
        };
}

//La física debe calcularse en el fixed update, y ser primero que el codigo del usuario
REGISTER_NATIVE_SYSTEM(lynx::ecs::systems::PhysicsSystem2D, lynx::ecs::SystemPhase::FixedUpdate, 10);
REQUIRED_COMPONENTS_FOR_SYSTEM(lynx::ecs::systems::PhysicsSystem2D, lynx::ecs::components::physics::Rigidbody2D);
