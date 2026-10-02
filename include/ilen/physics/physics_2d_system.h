#pragma once

#include <array>
#include <ilen/physics/components/physics_components.h>
#include <ilen/transform/transform_component.h>
#include <ilen/ecs/system.h>
#include <ilen/ecs/ecs.h>
#include <ilen/ecs/system_registry.h>
#include <ilen/physics/collision_matrix.h>
#include <vector>
#include <ilen/physics/components/physics_components.h>
#include <ilen/physics/collider_proxy_builder_2d.h>
#include <ilen/physics/narrow_phase_2d.h>
#include <ilen/physics/broad_phase_2d.h>
#include <optional>

namespace ilen::ecs::systems {
    using namespace ilen::ecs::components;
    using namespace ilen::maths::aabb;

    class PhysicsSystem2D final: public ilen::ecs::ISystem {
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

        public:
            //Metodos obligatorios por ISystem
            void Init(const ilen::EngineContext* engineContext) override; //Obtiene punteros de los sistemas base del motor, y un puntero al coordinador ECS, para acceder a otros sistemas del ECS, o a componentes y entidades, etc.
            void Update() override; //Se ejecuta cada frame, en este caso, en un FixedUpdate.

            //funciones helpers del sistema
            void registerEntity(Entity e, ArrayOfComponentsUsed* componentsArray);
            void unregisterEntity(Entity e);

#ifdef ILEN_DEBUG
            void checkStaticGuard(ArrayOfComponentsUsed* componentsArray);
#endif
        private:
            std::vector<Entity> m_dynamicEntities;

            //Array paralelos para los proxies
            std::vector<ilen::physics::ColliderProxy2D> m_dynamicProxies;
            std::vector<ilen::physics::ColliderProxy2D> m_staticProxies;

            std::array<RegistrationState, MAX_ENTITIES> m_registrationStates{};

            // Órdenes transitorias consumidas en un único fixed update. Se
            // guardan fuera del ECS porque sólo son estado interno del solver.
            std::array<vec2, MAX_ENTITIES> m_requestedTranslations{};
            std::array<vec2, MAX_ENTITIES> m_teleportTargets{};
            std::array<bool, MAX_ENTITIES> m_hasTeleport{};


            //Esto se podria cambiar, pero necesito terminar ya, así que es un posible vector para optimizacion
            std::unordered_map<ilen::ecs::Entity, TransformOrder> m_accumulatedTransformOrder;

            //Debido al que sistema de físicas es de los más complejos, vamos a dividir las diferentes
            //partes del proceso en funciones especializadas, de esa forma es más facil entender el flujo
            //del sistema.
            void syncEntities(ArrayOfComponentsUsed* componentsArray);
            void clearCollisionData(ArrayOfComponentsUsed* componentsArray);
            void registerPrevPosition(ArrayOfComponentsUsed* ComponentArray);
            void applyForces(float dt, ArrayOfComponentsUsed* ComponentArray);
            void applyGravity(float dt, ArrayOfComponentsUsed* componentsArray);
            void integrate(float dt, ArrayOfComponentsUsed* componentsArray);
            void rebuildDynamicProxies(float dt, ArrayOfComponentsUsed* componentsArray);
            void detectAndResolve(float dt, ArrayOfComponentsUsed* componentsArray);

            // Prepara el movimiento restante y su volumen barrido para CCD.
            void prepareSweeps(float remainingTime, ArrayOfComponentsUsed* componentsArray);

            // Avanza todos los cuerpos dinámicos la misma fracción temporal.
            void advanceDynamicProxies(float timeFraction);

            //Construir Proxies
            std::optional<ilen::physics::ColliderProxy2D> buildProxy(Entity entity, ArrayOfComponentsUsed* componentsArray);

            //Corrige colisiones
            void resolveCollision(
                ilen::physics::ColliderProxy2D& proxyA, 
                ilen::physics::ColliderProxy2D& proxyB,
                const physics::ContactManifold2D& manifold,
                ArrayOfComponentsUsed* componentsArray
            );

            //Escribe el CollisionComponent2D
            void writeContacts(
                const physics::ContactManifold2D& manifold,
                ArrayOfComponentsUsed* componentsArray
            );

            static void translateProxy(
                ilen::physics::ColliderProxy2D& proxy,
                const vec2& translation
            );

            static AABB2D calculateSweptBounds(
                const ilen::physics::ColliderProxy2D& proxy,
                const vec2& translation
            );

            ilen::physics::BroadPhase2D m_broadPhase{15.0f};
            bool m_staticBroadPhaseDirty = true;
#if ILEN_DEBUG
            struct StaticGuard {
                Entity entity = NULL_ENTITY;

                vec3 originalPosition{0.0f};
                vec3 originalRotation{0.0f};
                vec3 originalScale{1.0f};

                int frameCounter = 0;
                int checkInterval = 60;
            };

            std::vector<StaticGuard> m_staticGuards;

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
REGISTER_NATIVE_SYSTEM(ilen::ecs::systems::PhysicsSystem2D, ilen::ecs::SystemPhase::FixedUpdate, 10);
REQUIRED_COMPONENTS_FOR_SYSTEM(ilen::ecs::systems::PhysicsSystem2D, ilen::ecs::components::physics::Rigidbody2D);
