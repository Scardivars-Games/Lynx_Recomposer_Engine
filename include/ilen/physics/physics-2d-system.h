#pragma once

#include <ilen/physics/components/physics-components.h>
#include <ilen/ecs/system.h>
#include <ilen/ecs/ecs.h>
#include <ilen/ecs/system_registry.h>
#include <vector>

namespace ilen::ecs::systems {
    using namespace ilen::ecs::components;
    using namespace ilen::maths::aabb;

    class PhysicsSystem2D final: public ilen::ecs::ISystem {
        public:
            //Metodos obligatorios por ISystem
            void Init(const ilen::EngineContext* engineContext) override; //Obtiene punteros de los sistemas base del motor, y un puntero al coordinador ECS, para acceder a otros sistemas del ECS, o a componentes y entidades, etc.
            void Update() override; //Se ejecuta cada frame, en este caso, en un FixedUpdate.

            //funciones helpers del sistema
            void registerEntity(Entity e);
            void unregisterEntity(Entity e);

#ifdef ILEN_DEBUG
            void checkStaticGuard();
#endif
        private:
            std::vector<Entity> m_dynamicEntities;
            std::vector<Entity> m_staticEntities;

            //Debido al que sistema de físicas es de los más complejos, vamos a dividir las diferentes
            //partes del proceso en funciones especializadas, de esa forma es más facil entender el flujo
            //del sistema.
            void clearCollisionData();
            void applyGravity(float dt);
            void integrate(float dt);
            void detectAndResolve();

            //Construye un AABB 2D en el espacio del mundo (Transform y el BoxCollider)
            maths::aabb::AABB2D buildAABB(Entity e) const;
            
            //Corrige colisiones
            void resolveCollision(Entity a, Entity b, const maths::aabb::IntersectResult2D& result);

            //Escribe el CollisionComponent2D
            void writeContact(Entity receiver, Entity other, const IntersectResult2D& result, bool wasTrigger);

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