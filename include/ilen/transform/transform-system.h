#pragma once

#include <ilen/ecs/system.h>
#include <ilen/ecs/ecs.h>
#include <ilen/headers/ecs-components.h>

extern std::unique_ptr<ilen::ecs::ECSCoordinator> gCoordinator;

namespace ilen::ecs::systems {
    class TransformSystem : public ilen::ecs::System {
        public:
            void Update() {
                //Este sistema se encargara de las jerarquías de los objetos, convirtiendo la posición local en posición global. Luego calculará la matriz del modelo (modelToViewMatrix)
                for(auto const& entity : entities){
                    auto& transform = gCoordinator->getComponent<ilen::ecs::components::TransformComponent>(entity);


                }
            }
    };
}