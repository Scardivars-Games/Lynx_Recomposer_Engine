#if !ILEN_ONLY_ECS

#include <ilen/game-logic/script-system.h>
#include <ilen/core/engine-context.h>
#include <ilen-public/API/transform.h>
#include <ilen-public/API/physics-2d.h>


namespace ilen::ecs::systems {
    void ScriptingSystem::Init(const EngineContext *engineContext) {
        p_coordinator = engineContext->p_coordinator.get();
        p_jobSystem = engineContext->p_jobSystem.get();
        p_commandDispatcher = engineContext->p_commandDispatcher.get();

        for(auto const& entity: entities){
            auto scriptComponent = p_coordinator->getComponent<ilen::ecs::components::ScriptingBridgeComponent>(entity);

            //En este paso se instancia el script, así que lo hacemos aquí, y ejecutamos el init de él.
            if(!scriptComponent->instance){
                scriptComponent->instance = scriptComponent->instantiate();
                scriptComponent->instance->entity = entity;
                scriptComponent->instance->coordinator = p_coordinator; //.get devuelve el puntero puro (*) en vez del unique_ptr
                scriptComponent->instance->Init();
            }
        }
    }


    void ScriptingSystem::Update() {
        {
            ilen::time::ScopedTimer timer("Scripting Update method");

            std::vector<Entity> entityList(entities.begin(), entities.end());
            size_t total = entityList.size();
            if(total == 0) return;

            size_t numThreads = p_jobSystem->AvailableThreads();
            size_t chunkSize = (total + numThreads - 1) / numThreads;
            size_t numChunks = (total + chunkSize - 1) / chunkSize;

            auto* scriptComponentArray = p_coordinator->getComponentArray<ilen::ecs::components::ScriptingBridgeComponent>();

            {
                ilen::time::ScopedTimer timer("Scripting Update method -- Paralelized execution");
                ilen::job_system::JobGroup group;

                for(int i = 0; i < numChunks; i++){
                    size_t start = i * chunkSize;
                    size_t end = std::min(start + chunkSize, total);

                    p_jobSystem->SubmitJob([this, &entityList, start, end, scriptComponentArray]() {
                        for(size_t j = start; j < end; j++){
                            auto entity = entityList[j];
                            auto* scriptComponent = scriptComponentArray->getData(entity);
                            if(scriptComponent && scriptComponent->instance){
                                scriptComponent->instance->Update();
                            }
                        }
                    }, &group);
                }

                group.Wait();
            }
            
            {
                ilen::time::ScopedTimer timer("Scripting Update method -- API pools executian, and ECS command pool execution");
                ilen::engine::api::TransformAPI::ExecuteAllAccumulatedTransform();                
                p_commandDispatcher->ExecuteAll();
            }
        }
    }

    void ScriptingSystem::FixedUpdate() const {
            std::vector<Entity> entityList(entities.begin(), entities.end());
            size_t total = entityList.size();
            if(total == 0) return;

            size_t numThreads = p_jobSystem->AvailableThreads();
            size_t chunkSize = (total + numThreads - 1) / numThreads;
            size_t numChunks = (total + chunkSize - 1) / chunkSize;

            auto* scriptComponentArray = p_coordinator->getComponentArray<ilen::ecs::components::ScriptingBridgeComponent>();

            
            ilen::job_system::JobGroup group;

            for(int i = 0; i < numChunks; i++){
                size_t start = i * chunkSize;
                size_t end = std::min(start + chunkSize, total);

                p_jobSystem->SubmitJob([this, &entityList, start, end, scriptComponentArray]() {
                    for(size_t j = start; j < end; j++){
                        auto entity = entityList[j];
                        auto* scriptComponent = scriptComponentArray->getData(entity);
                        if(scriptComponent && scriptComponent->instance){
                            scriptComponent->instance->FixedUpdate();
                        }
                    }
                }, &group);
            }

            group.Wait();

            //Metodos para realizar serialmente tareas no thread-safe
            ilen::engine::api::TransformAPI::ExecuteAllAccumulatedTransform();                
            p_commandDispatcher->ExecuteAll();
    }

    void ScriptingSystem::LateUpdate() const {
        std::vector<Entity> entityList(entities.begin(), entities.end());
        size_t total = entityList.size();
        if(total == 0) return;

        size_t numThreads = p_jobSystem->AvailableThreads();
        size_t chunkSize = (total + numThreads - 1) / numThreads;
        size_t numChunks = (total + chunkSize - 1) / chunkSize;

        auto* scriptComponentArray = p_coordinator->getComponentArray<ilen::ecs::components::ScriptingBridgeComponent>();


            ilen::job_system::JobGroup group;

            for(int i = 0; i < numChunks; i++){
                size_t start = i * chunkSize;
                size_t end = std::min(start + chunkSize, total);

                p_jobSystem->SubmitJob([this, &entityList, start, end, scriptComponentArray]() {
                    for(size_t j = start; j < end; j++){
                        auto entity = entityList[j];
                        auto* scriptComponent = scriptComponentArray->getData(entity);
                        if(scriptComponent && scriptComponent->instance){
                            scriptComponent->instance->LateUpdate();
                        }
                    }
                }, &group);
            }

        group.Wait();

        //Metodos para realizar serialmente tareas no thread-safe
        ilen::engine::api::TransformAPI::ExecuteAllAccumulatedTransform();   
        ilen::engine::api::PhysicsAPI::ExecuteAllAccumulatedForces();            
        p_commandDispatcher->ExecuteAll();
    }
}

#endif //!ILEN_ONLY_ECS