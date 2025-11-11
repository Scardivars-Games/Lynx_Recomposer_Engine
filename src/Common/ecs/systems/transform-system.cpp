#include <ilen/transform/transform-system.h>

namespace ilen::ecs::systems {
    void TransformSystem::Init(ilen::EngineContext *engineContext) {
        p_Coordinator = engineContext->p_coordinator.get();
        p_jobSystem = engineContext->p_jobSystem.get();

        ilen::engine::api::TransformAPI::Init(p_Coordinator);
    }

    void TransformSystem::Update() {
        futures.clear();

        //Este sistema se encargara de las jerarquías de los objetos, convirtiendo la posición local en posición global. Luego calculará la matriz del modelo (modelToViewMatrix)
        for(auto const& entity : entities){

            std::future<void> future = p_jobSystem->SubmitJob([this, entity]() {
                auto transform = p_Coordinator->getComponent<TransformComponent>(entity);

                if(transform){
                    //Calculamos las nuevas matrices locales :)
                    auto translation = p_Coordinator->getComponent<Traslate_TransformOrder>(entity);
                    auto rotation = p_Coordinator->getComponent<Rotation_TransformOrder>(entity);
                    auto scale = p_Coordinator->getComponent<Scale_TransformOrder>(entity);

                    if(translation){
                        transform->position = transform->position + translation->traslation;
                        p_Coordinator->removeComponent<Traslate_TransformOrder>(entity);
                    }

                    if(rotation){
                        transform->rotation = transform->rotation + rotation->rotation;
                        p_Coordinator->removeComponent<Rotation_TransformOrder>(entity);

                    }

                    if(scale){
                        transform->scale.x *= scale->scale.x;
                        transform->scale.y *= scale->scale.y;
                        transform->scale.z *= scale->scale.z;

                        p_Coordinator->removeComponent<Scale_TransformOrder>(entity);
                    }

                    if(transform->parent == NULL_ENTITY){
                        //Si el objeto no tiene padre, empezamos a calcular las matrices. Luego, el padre, manda a calcular la de los hijos recursivamente.
                        calculateLocalToWorldMatrix(entity, matrix::mat4::identity());
                    }
                }
            });
            futures.push_back(std::move(future));

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
        modelMatrix = matrix::translate(modelMatrix, transform->position);
        //Rotation
        modelMatrix = matrix::rotate(modelMatrix, Radians(Degrees(transform->rotation.x), ILEN_ANGLE_TYPE_DEGREES), vec3::right()); //X
        modelMatrix = matrix::rotate(modelMatrix, Radians(Degrees(transform->rotation.y), ILEN_ANGLE_TYPE_DEGREES), vec3::up()); //Y
        modelMatrix = matrix::rotate(modelMatrix, Radians(Degrees(transform->rotation.z), ILEN_ANGLE_TYPE_DEGREES), vec3::forward()); //Z
        //Scale
        modelMatrix= matrix::scale(modelMatrix, transform->scale);

        transform->localToWorldMatrix = parentMatrix * modelMatrix;

        for(const auto& children : transform->childrens){
            std::future<void> future = p_jobSystem->SubmitJob([this, transform, children]() {
                calculateLocalToWorldMatrix(children, transform->localToWorldMatrix);
            });

            futures.push_back(std::move(future));
        }
    }
}