#pragma once

#if !ILEN_ONLY_ECS

#include <ilen/ecs/ecs.h>

namespace ilen::game {
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

            ilen::ecs::Entity entity;
            ilen::ecs::ECSCoordinator* coordinator = nullptr;
    };
};

#endif //!ILEN_ONLY_ECS