#if !ILEN_ONLY_ECS

#include <ilen/game_logic/script_system.h>
#include <ilen/core/engine_context.h>
#include <ilen_public/API/transform_api.h>
#include <ilen_public/API/physics2d_api.h>
#include <ilen_public/API/entity_api.h>
#include <ilen_public/API/camera_api.h>
#include <ilen_public/API/rendering_api.h>


namespace ilen::ecs::systems {
    void ScriptingSystem::Init(const EngineContext *engineContext) {
        p_coordinator = engineContext->p_coordinator.get();
        p_jobSystem = engineContext->p_jobSystem.get();
        p_commandDispatcher = engineContext->p_commandDispatcher.get();

        //Inicializar apis
        ilen::engine::api::EntityAPI::Init(engineContext);
        ilen::engine::api::CameraAPI::Init(engineContext);
        ilen::engine::api::RenderingAPI::Init(engineContext);


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
            ilen::engine::api::CameraAPI::GetMainCameraPointer();

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
                ilen::engine::api::PhysicsAPI::ExecuteAllAccumulatedForces();               
                p_commandDispatcher->ExecuteAll();
            }
            ilen::engine::api::TransformAPI::ExecuteOrdersInTheTransformSystem();
        }
    }

    void ScriptingSystem::FixedUpdate() const {
            ilen::time::ScopedTimer timer("Scripting Update method -- Fixed Update");
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
        ilen::time::ScopedTimer timer("Scripting Update method -- Late Update");
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
        ilen::engine::api::EntityAPI::ResetMetadataPointer();    
        ilen::engine::api::CameraAPI::ResetMainCameraPointer();
        p_commandDispatcher->ExecuteAll();
    }
}

#endif //!ILEN_ONLY_ECS