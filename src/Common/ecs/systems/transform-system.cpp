#include <ilen/transform/transform-system.h>


namespace ilen::ecs::systems {
    void TransformSystem::Init(const ilen::EngineContext *engineContext) {
        p_Coordinator = engineContext->p_coordinator.get();
        p_jobSystem = engineContext->p_jobSystem.get();

        ilen::engine::api::TransformAPI::Init(p_Coordinator);
    }

    void TransformSystem::Update() {
        //Hacemos copia del vector de entidades, para hacerlo cache-friendly
        std::vector<Entity> entityList(entities.begin(), entities.end());
        size_t total = entityList.size();
        size_t numThreads = p_jobSystem->AvailableThreads();
        size_t chunckSize = (total + numThreads - 1) / numThreads;
        size_t numChuncks = (total + chunckSize - 1) / chunckSize;

        //Primera fase, actualizar la posicion/rotacion/escala en paralelo
        //Cada chunk tendra su propia lista de vectores para trabajar
        std::vector<std::vector<Entity>> removeTranslation(numChuncks);
        std::vector<std::vector<Entity>> removeRotation(numChuncks);
        std::vector<std::vector<Entity>> removeScale(numChuncks);

        ilen::job_system::JobGroup phase1;

        for(size_t i = 0; i < numChuncks; i++){
            size_t start = i * chunckSize;
            size_t end = std::min(start + chunckSize, total);

            p_jobSystem->SubmitJob([this, &entityList, start, end, i, &removeRotation, &removeScale, &removeTranslation]() {
                for(size_t j = start; j < end; j++){
                    Entity entity = entityList[j];
                    auto* transform = p_Coordinator->getComponent<TransformComponent>(entity);
                    if(!transform) continue;

                    auto* translation = p_Coordinator->getComponent<Traslate_TransformOrder>(entity);
                    auto* rotation = p_Coordinator->getComponent<Rotation_TransformOrder>(entity);
                    auto* scale = p_Coordinator->getComponent<Scale_TransformOrder>(entity);

                    if(translation){
                        transform->position = transform->position + translation->traslation;
                        removeTranslation[i].push_back(entity);
                    }
                    if(rotation){
                        transform->rotation = transform->rotation + rotation->rotation;
                        removeRotation[i].push_back(entity);
                    }
                    if(scale) {
                        transform->scale.x *= scale->scale.x;
                        transform->scale.y *= scale->scale.y;
                        transform->scale.z *= scale->scale.z;
                        removeScale[i].push_back(entity);
                    }
                }
            }, &phase1);
        }

        phase1.Wait();

        for(size_t i = 0; i < numChuncks; i++){
            for(auto e: removeTranslation[i]) p_Coordinator->removeComponent<Traslate_TransformOrder>(e);
            for(auto e: removeRotation[i]) p_Coordinator->removeComponent<Rotation_TransformOrder>(e);
            for(auto e: removeScale[i]) p_Coordinator->removeComponent<Scale_TransformOrder>(e);
        }

        //Fase 2. Calculo de las matrices en paralelo, desde las raices.
        std::vector<Entity> rootEntities;
        rootEntities.reserve(total);

        for(const auto& entity: entityList){
            auto* transform = p_Coordinator->getComponent<TransformComponent>(entity);
            if(transform && transform->parent == NULL_ENTITY)
                rootEntities.push_back(entity);
        }

        size_t rootTotal = rootEntities.size();
        size_t rootChunkSize = std::max(size_t(1), (rootTotal + numThreads - 1)/ numThreads);

        ilen::job_system::JobGroup phase2;
        for(size_t i = 0; i < rootTotal; i += rootChunkSize){
            size_t start = i;
            size_t end = std::min(start + rootChunkSize, rootTotal);

            p_jobSystem->SubmitJob([this, &rootEntities, start, end, &phase2]() {
                for(size_t j = start; j < end; j++){
                    calculateLocalToWorldMatrix(rootEntities[j], matrix::mat4::identity(), &phase2);
                }
            }, &phase2);
        }

        phase2.Wait();
    }

    void TransformSystem::calculateLocalToWorldMatrix(Entity entity, matrix::mat4 parentMatrix, ilen::job_system::JobGroup* group) {
        auto transform = p_Coordinator->getComponent<TransformComponent>(entity);
        if(!transform) return;

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

        for(const auto& child: transform->childrens){
            p_jobSystem->SubmitJob([this, child, worldMatrix = transform->localToWorldMatrix, group](){
                calculateLocalToWorldMatrix(child, worldMatrix, group);
            }, group);
        }
    }
}