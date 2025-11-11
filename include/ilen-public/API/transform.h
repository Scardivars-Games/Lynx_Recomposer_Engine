//Api pública para facilitar las operaciones de transform del motor. Crea los componentes necesarios para que el motor luego puedo ejecutar las traslaciones
#pragma once

#include <ilen/ecs/ecs.h>
#include <ilen/transform/transform-system.h>
#include <ilen/headers/ilen-logs.h>
#include <ilen/transform/transform-component.h>
#include <utility>



namespace ilen::engine::api {
    class TransformAPI {
        public:
            //Traslation calls
            static void Translate(ilen::ecs::Entity entity, Vector3 traslation);
            
            //Rotation calls
            static void Rotate(ilen::ecs::Entity entity, Vector3 rotation);
            static void Rotate(ilen::ecs::Entity entity, Vector3 axis, float rotation);

            //Scale calls
            static void Scale(ilen::ecs::Entity entity, float scale);
        private:    
            static void Init(ilen::ecs::ECSCoordinator* coordinator);
            static void CheckInit();
            static inline ilen::ecs::ECSCoordinator* p_coordinator = nullptr;

            friend class ilen::ecs::systems::TransformSystem;
    };
}