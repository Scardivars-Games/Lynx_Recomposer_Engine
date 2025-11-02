#include <ilen/transform/transform-system.h>

namespace ilen::ecs::systems {
    void TransformSystem::Init(ilen::EngineContext *engineContext) {
        p_Coordinator = engineContext->p_coordinator.get();
        p_jobSystem = engineContext->p_jobSystem.get();
    }

    void TransformSystem::Update() {
        std::vector<std::future<void>> futures;
        //Este sistema se encargara de las jerarquías de los objetos, convirtiendo la posición local en posición global. Luego calculará la matriz del modelo (modelToViewMatrix)
        for(auto const& entity : entities){
            auto transform = p_Coordinator->getComponent<TransformComponent>(entity);

            //Si el objeto no tiene padre, empezamos a calcular las matrices. Luego, el padre, manda a calcular la de los hijos recursivamente.
            if(transform){
                if(transform->parent == NULL_ENTITY){
                    std::future<void> future = p_jobSystem->SubmitJob([this, entity]() {
                        calculateLocalToWorldMatrix(entity, matrix::mat4::identity());
                    });

                    futures.push_back(std::move(future));
                }
            }
        }

        //Esperamos que se hayan completado los cálculos
        for(auto& future : futures) {
            future.wait();
        }
    }

    void TransformSystem::calculateLocalToWorldMatrix(Entity entity, matrix::mat4 parentMatrix) {
        auto transform = p_Coordinator->getComponent<TransformComponent>(entity);

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
}