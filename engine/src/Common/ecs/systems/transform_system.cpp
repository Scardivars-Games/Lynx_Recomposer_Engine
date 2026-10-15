#include <lynx/transform/transform_system.h>
#include <lynx/core/time.h>

namespace lynx::ecs::systems {
    void TransformSystem::Init(const lynx::EngineContext *engineContext) {
        p_coordinator = engineContext->p_coordinator.get();
        p_jobSystem = engineContext->p_jobSystem.get();
        p_commandDispatcher = engineContext->p_commandDispatcher.get();

        lynx::engine::api::TransformAPI::Init(engineContext);
    }

    void TransformSystem::Update() {
        lynx::time::ScopedTimer timer("Transform system");

        std::vector<Entity> entityList(entities.begin(), entities.end());
        size_t total = entityList.size();
        if(total == 0) return;

        size_t numThreads = p_jobSystem->AvailableThreads();
        size_t chunkSize = (total + numThreads -1 ) / numThreads;
        size_t numChunks = (total + chunkSize - 1) / chunkSize;

        //obtener arrays
        auto* transformArray = p_coordinator->getComponentArray<TransformComponent>();
        auto* orderArray = p_coordinator->getComponentArray<TransformOrder>();
        if(!transformArray) return;      
        
        ExecuteTransformOrders();

        {
            lynx::time::ScopedTimer timer("Transform system -- Fase 2");

            std::vector<Entity> rootEntities;
            rootEntities.reserve(total);

            for(const auto& entity :  entityList){
                if(auto* transform = transformArray->getData(entity)){
                    if(transform->parent == NULL_ENTITY){
                        rootEntities.push_back(entity);
                    }
                }
            }

            size_t rootTotal = rootEntities.size();
            if(rootTotal == 0) return;

            size_t rootChunkSize = std::max(size_t(1), (rootTotal + numThreads - 1) / numThreads);
            size_t rootNumChunks = std::max(size_t(1), (rootTotal + rootChunkSize - 1) / rootChunkSize);
            lynx::job_system::JobGroup phase2;

            for(size_t i = 0; i < rootNumChunks; i++){
                size_t start = i * rootChunkSize;
                size_t end = std::min(start + rootChunkSize, rootTotal);

                p_jobSystem->SubmitJob([this, &rootEntities, start, end, &phase2, transformArray]() {
                    for(size_t j = start; j < end; j++) {
                        calculateLocalToWorldMatrix(rootEntities[j], matrix::mat4::identity(), &phase2, transformArray);
                    }
                }, &phase2);
            }

            phase2.Wait();
        }

        //OLD
        /*{
            lynx::time::ScopedTimer timer("Transform system");

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

            {
                lynx::time::ScopedTimer timer("Transform system Phase 1");
                lynx::job_system::JobGroup phase1;

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
            }


            {
                lynx::time::ScopedTimer timer("Transform system Phase 2");
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

                lynx::job_system::JobGroup phase2;
                for(size_t i = 0; i < numChuncks; i++){
                    size_t start = i * rootChunkSize;
                    size_t end = std::min(start + rootChunkSize, rootTotal);

                    p_jobSystem->SubmitJob([this, &rootEntities, start, end, &phase2]() {
                        for(size_t j = start; j < end; j++){
                            calculateLocalToWorldMatrix(rootEntities[j], matrix::mat4::identity(), &phase2);
                        }
                    }, &phase2);
                }

                phase2.Wait();
            }
        }*/
    }

    void TransformSystem::ExecuteTransformOrders(){
        {
            lynx::time::ScopedTimer timer("Transform system -- Fase 1");
            lynx::job_system::JobGroup phase1;

            std::vector<Entity> entityList(entities.begin(), entities.end());
            size_t total = entityList.size();        if(total == 0) return;

            size_t numThreads = p_jobSystem->AvailableThreads();
            size_t chunkSize = (total + numThreads -1 ) / numThreads;
            size_t numChunks = (total + chunkSize - 1) / chunkSize;

            //obtener arrays
            auto* transformArray = p_coordinator->getComponentArray<TransformComponent>();
            auto* orderArray = p_coordinator->getComponentArray<TransformOrder>();
            if(!transformArray) return;      

            for(size_t i = 0; i < numChunks; i++){
                size_t start = i * chunkSize;
                size_t end = std::min(start + chunkSize, total);

                p_jobSystem->SubmitJob([this, &entityList, start, end, transformArray, orderArray]() {
                    for(size_t j = start; j < end; j++){
                        Entity entity = entityList[j];

                        auto* transform = transformArray->getData(entity);
                        if(!transform) continue;
                        
                        if(!orderArray) continue;
                        
                        auto* order = orderArray->getData(entity);
                        if(order && order->hasOrders) {
                            if(order->isNewPos){
                                transform->position = order->translation;
                            }else{
                                transform->position += order->translation;
                            }

                            if(order->isNewRot){
                                transform->rotation = order->rotation;
                            }else{
                                transform->rotation += order->rotation;
                            }

                            if(order->isNewScale){
                                transform->scale = order->scaleMultiplier;
                            }else{
                                transform->scale *= order->scaleMultiplier;
                            }

                           /* if(order->translation.y > 0 || order->translation.y < 0) {
                                ILEN_LOG_P("Prueba de gravedad", "La entidad con nombre %s fue modificada su posicion con estos parametros: %s", p_coordinator->getComponent<EntityMetadataComponent>(entity)->getName().c_str(), vector::to_string(order->translation).c_str());
                            }*/

                            //Reset
                            order->translation = vec3(0.0f);
                            order->rotation = vec3(0.0f);
                            order->scaleMultiplier = vec3(1.0f);
                            order->hasOrders = false;
                        }
                    }
                }, &phase1);
            }

            phase1.Wait();
        }        
    }

    void TransformSystem::calculateLocalToWorldMatrix(Entity entity, matrix::mat4 parentMatrix, lynx::job_system::JobGroup* group, ComponentArray<TransformComponent>* componentArray){
        auto* transform = componentArray->getData(entity);
        if(!transform) return;

        // 1. Conversión hiper-rápida de grados a radianes para los 3 ejes a la vez.
        // Al multiplicar el vector por un escalar, usamos el operador desenrollado que creamos.
        constexpr float DEG_TO_RAD = 3.14159265359f / 180.0f;
        vec3 rotRadians = transform->rotation * DEG_TO_RAD;

        // 2. Construcción directa de la matriz local (Cero sobrecoste, sin matrices de identidad)
        matrix::mat4 modelMatrix = matrix::createTRS(transform->position, rotRadians, transform->scale);

        // 3. Cálculo de la matriz en el mundo
        // Usa tu nuevo operator*(mat4, mat4) 100% desenrollado sin bucles
        transform->localToWorldMatrix = parentMatrix * modelMatrix;

        // 4. Propagación a los hijos
        for(const auto& child: transform->childrens){
            p_jobSystem->SubmitJob([this, child, worldMatrix = transform->localToWorldMatrix, group, componentArray](){
                calculateLocalToWorldMatrix(child, worldMatrix, group, componentArray);
            }, group);
        }
    }
}