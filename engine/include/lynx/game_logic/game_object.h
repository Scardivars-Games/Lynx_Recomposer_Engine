#pragma once
#if !LYNX_ONLY_ECS
#include <lynx/physics/components/specification/collision_components.h>
#include <lynx/ecs/ecs.h>

namespace lynx::ecs::components::physics {
    struct ContactManifold2D;
}

namespace lynx::game {
    using namespace lynx::ecs::components::physics;

    class GameObject {
        public:
            virtual ~GameObject() = default;

            //Se ejecuta al crearse el objeto en la lógica del programa.
            virtual void Init() {}; 
            //Se ejecuta una vez por cada ciclo del reloj.
            virtual void Update() {}; 
            //Se ejecuta una vez cada cierto tiempo fijo, no siendo afectado por los frames.
            virtual void FixedUpdate() {};
            //Se ejecuta tras la lógica de todos los posibles update del juego.
            virtual void LateUpdate() {};

            //Este permite a los GameObjects consumir eventos de físicas y triggers
            virtual void OnCollisionEnter(const ContactManifold2D& contact) {};
            virtual void OnCollisionStay(const ContactManifold2D& contact) {};
            virtual void OnCollisionExit(const ContactManifold2D& contact) {};
            virtual void OnTriggerEnter(const ContactManifold2D& contact) {};
            virtual void OnTriggerStay(const ContactManifold2D& contact) {};
            virtual void OnTriggerExit(const ContactManifold2D& contact) {};

            lynx::ecs::Entity entity;
            lynx::ecs::ECSCoordinator* coordinator = nullptr;
    };
};

#endif //!ILEN_ONLY_ECS