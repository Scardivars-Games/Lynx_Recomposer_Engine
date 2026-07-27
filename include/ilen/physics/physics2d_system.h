#pragma once

#include <ilen/physics/components/physics_components.h>
#include <ilen/transform/transform_component.h>
#include <ilen/ecs/system.h>
#include <ilen/ecs/ecs.h>
#include <ilen/ecs/system_registry.h>
#include <ilen/physics/collision_matrix.h>
#include <vector>

namespace ilen::ecs::systems {
    using namespace ilen::ecs::components;
    using namespace ilen::maths::aabb;

    class PhysicsSystem2D final: public ilen::ecs::ISystem {
        struct ArrayOfComponentsUsed {
            ComponentArray<physics::Rigidbody2D>* rigidbodyComponentArray = nullptr;
            ComponentArray<physics::CollisionComponent2D>* collisionComponentArray = nullptr;
            ComponentArray<physics::BoxCollider2D>* boxColliderArray = nullptr;
            ComponentArray<TransformComponent>* transformComponentArray = nullptr;
            ComponentArray<TransformOrder>* transformOrderArray = nullptr;
            ComponentArray<physics::PhysicsOrder>* physicsOrderArray = nullptr;
            ComponentArray<EntityMetadataComponent>* entityMetadataArray = nullptr;
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

            //Array paralelos para los datos de las entidades estáticas
            std::vector<Entity> m_staticEntities;
            std::vector<maths::aabb::AABB2D> m_staticAABBs;

            //Esto se podria cambiar, pero necesito terminar ya, así que es un posible vector para optimizacion
            std::unordered_map<ilen::ecs::Entity, TransformOrder> m_accumulatedTransformOrder;

            //Debido al que sistema de físicas es de los más complejos, vamos a dividir las diferentes
            //partes del proceso en funciones especializadas, de esa forma es más facil entender el flujo
            //del sistema.
            void clearCollisionData(ArrayOfComponentsUsed* componentsArray);
            void applyGravity(float dt, ArrayOfComponentsUsed* componentsArray);
            void integrate(float dt, ArrayOfComponentsUsed* componentsArray);
            void detectAndResolve(float dt, ArrayOfComponentsUsed* componentsArray);
            void syncEntities(ArrayOfComponentsUsed* componentsArray);
            void applyForces(float dt, ArrayOfComponentsUsed* ComponentArray);
            void registerPrevPosition(ArrayOfComponentsUsed* ComponentArray);

            //Construye un AABB 2D en el espacio del mundo (Transform y el BoxCollider)
            maths::aabb::AABB2D buildAABB(Entity e, ArrayOfComponentsUsed* componentsArray) const;
            
            //Corrige colisiones
            void resolveCollision(Entity a, Entity b, const maths::aabb::IntersectResult2D& result, ArrayOfComponentsUsed* componentsArray);

            //Escribe el CollisionComponent2D
            void writeContact(Entity receiver, Entity other, const IntersectResult2D& result, bool wasTrigger, ArrayOfComponentsUsed* componentsArray);

#if ILEN_DEBUG
            struct StaticGuard {
                Entity entity;
                vec2 originalPosition;
                int frameCounter = 0;
                int checkInterval = 60;
            };

            std::vector<StaticGuard> m_staticGuards;
#endif
        };
}

//La física debe calcularse en el fixed update, y ser primero que el codigo del usuario
REGISTER_NATIVE_SYSTEM(ilen::ecs::systems::PhysicsSystem2D, ilen::ecs::SystemPhase::FixedUpdate, 10);
REQUIRED_COMPONENTS_FOR_SYSTEM(ilen::ecs::systems::PhysicsSystem2D, ilen::ecs::components::physics::Rigidbody2D);