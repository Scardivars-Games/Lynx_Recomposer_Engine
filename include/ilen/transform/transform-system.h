#pragma once

#include <ilen/ecs/system.h>
#include <ilen/ecs/ecs.h>
#include <ilen/headers/ecs-components.h>
#include <ilen/headers/maths.h>

extern std::unique_ptr<ilen::ecs::ECSCoordinator> gCoordinator;

using namespace ilen::maths;

namespace ilen::ecs::systems {
    using namespace ilen::ecs::components;

    class TransformSystem : public ilen::ecs::System {
        public:
            void Update() {
                //Este sistema se encargara de las jerarquías de los objetos, convirtiendo la posición local en posición global. Luego calculará la matriz del modelo (modelToViewMatrix)
                for(auto const& entity : entities){
                    auto transform = gCoordinator->getComponent<TransformComponent>(entity);

                    //Si el objeto no tiene padre, empezamos a calcular las matrices. Luego, el padre, manda a calcular la de los hijos recursivamente.
                    if(transform){
                        if(transform->parent == NULL_ENTITY){
                            calculateLocalToWorldMatrix(entity, matrix::mat4::identity());
                        }
                    }
                }
            }

        private:
            void calculateLocalToWorldMatrix(Entity entity, matrix::mat4 parentMatrix){
                    auto transform = gCoordinator->getComponent<TransformComponent>(entity);

                    matrix::mat4 modelMatrix = matrix::mat4::identity();
                    //Orden es SRT (Scale-Rotation-Traslation)
                    //Se tiene que hacer alrevés porque las matrices operan de esa forma.
                    //Transform
                    matrix::translate(modelMatrix, transform->position);
                    //Rotation
                    matrix::rotate(modelMatrix, Radians(Degrees(transform->rotation.x), ILEN_ANGLE_TYPE_DEGREES), vec3::right()); //X
                    matrix::rotate(modelMatrix, Radians(Degrees(transform->rotation.y), ILEN_ANGLE_TYPE_DEGREES), vec3::up()); //Y
                    matrix::rotate(modelMatrix, Radians(Degrees(transform->rotation.z), ILEN_ANGLE_TYPE_DEGREES), vec3::forward()); //Z
                    //Scale
                    matrix::scale(modelMatrix, transform->scale);

                    transform->localToWorldMatrix = parentMatrix * modelMatrix;

                    for(const auto& children : transform->childrens){
                        calculateLocalToWorldMatrix(children, transform->localToWorldMatrix);
                    }
            }
    };
}