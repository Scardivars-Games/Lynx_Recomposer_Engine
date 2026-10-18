#if !ILEN_ONLY_ECS

#include <lynx/game_logic/script_system.h>
#include <lynx/game_logic/script_registry.h>
#include <lynx/core/engine_context.h>
#include <lynx_public/API/transform_api.h>
#include <lynx_public/API/physics_2d_api.h>
#include <lynx_public/API/entity_api.h>
#include <lynx_public/API/camera_api.h>
#include <lynx_public/API/rendering_api.h>

REGISTER_NATIVE_SYSTEM(lynx::ecs::systems::ScriptingSystem, lynx::ecs::SystemPhase::VariableUpdate, 0);
REQUIRED_COMPONENTS_FOR_SYSTEM(lynx::ecs::systems::ScriptingSystem, lynx::ecs::components::ScriptingBridgeComponent);

namespace lynx::ecs::systems {
    void ScriptingSystem::Init(const EngineContext *engineContext) {
        p_coordinator = engineContext->p_coordinator.get();
        p_jobSystem = engineContext->p_jobSystem.get();
        p_commandDispatcher = engineContext->p_commandDispatcher.get();

        //Inicializar apis
        lynx::engine::api::EntityAPI::Init(engineContext);
        lynx::engine::api::CameraAPI::Init(engineContext);
        lynx::engine::api::RenderingAPI::Init(engineContext);


        for(auto const& entity: entities){
            auto scriptComponent = p_coordinator->getComponent<lynx::ecs::components::ScriptingBridgeComponent>(entity);

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
        //Primero hacemos binding si es necesario en el main thread
        auto* scriptComponentArray = p_coordinator->getComponentArray<lynx::ecs::components::ScriptingBridgeComponent>();

        for(const auto& entity: entities) {
            auto* scriptComp = scriptComponentArray->getData(entity);

            if(!scriptComp) continue;

            if(!scriptComp->instantiate && !scriptComp->scriptName.empty()) {
                lynx::game::ScriptRegistry::TryBind(scriptComp->scriptName, *scriptComp);
            }

            if(!scriptComp->instance && scriptComp->instantiate) {
                scriptComp->instance = scriptComp->instantiate();

                scriptComp->instance->entity = entity;
                scriptComp->instance->coordinator = p_coordinator;
                scriptComp->instance->Init();
            }
        }

        //Ejecutamos los transforms que hayan acumulados otros sistemas ya que no usan la API de transformaciones.
        lynx::engine::api::TransformAPI::ExecuteOrdersInTheTransformSystem();

        {
            lynx::time::ScopedTimer timer("Scripting Update method");

            std::vector<Entity> entityList(entities.begin(), entities.end());
            size_t total = entityList.size();
            if(total == 0) return;

            size_t numThreads = p_jobSystem->AvailableThreads();
            size_t chunkSize = (total + numThreads - 1) / numThreads;
            size_t numChunks = (total + chunkSize - 1) / chunkSize;

            lynx::engine::api::CameraAPI::GetMainCameraPointer();

            {
                lynx::time::ScopedTimer timer("Scripting Update method -- Paralelized execution");
                lynx::job_system::JobGroup group;

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
                lynx::time::ScopedTimer timer("Scripting Update method -- API pools executian, and ECS command pool execution");
                lynx::engine::api::TransformAPI::ExecuteAllAccumulatedTransform();         
                lynx::engine::api::PhysicsAPI::ExecuteAllAccumulatedForces();               
                p_commandDispatcher->ExecuteAll();
            }

            //Ejecutamos los transforms generados durante los scripts.
            lynx::engine::api::TransformAPI::ExecuteOrdersInTheTransformSystem();
        }
    }

    void ScriptingSystem::FixedUpdate() const {
            lynx::time::ScopedTimer timer("Scripting Update method -- Fixed Update");
            std::vector<Entity> entityList(entities.begin(), entities.end());
            size_t total = entityList.size();
            if(total == 0) return;

            size_t numThreads = p_jobSystem->AvailableThreads();
            size_t chunkSize = (total + numThreads - 1) / numThreads;
            size_t numChunks = (total + chunkSize - 1) / chunkSize;

            auto* scriptComponentArray = p_coordinator->getComponentArray<lynx::ecs::components::ScriptingBridgeComponent>();

            
            lynx::job_system::JobGroup group;

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
            lynx::engine::api::TransformAPI::ExecuteAllAccumulatedTransform();
            // TransformAPI puede redirigir el movimiento XY de un Rigidbody a
            // PhysicsAPI. Debe publicarse antes de ejecutar PhysicsSystem2D en
            // este mismo fixed update.
            lynx::engine::api::PhysicsAPI::ExecuteAllAccumulatedForces();
            p_commandDispatcher->ExecuteAll();
    }

    void ScriptingSystem::LateUpdate() const {
        lynx::time::ScopedTimer timer("Scripting Update method -- Late Update");
        std::vector<Entity> entityList(entities.begin(), entities.end());
        size_t total = entityList.size();
        if(total == 0) return;

        size_t numThreads = p_jobSystem->AvailableThreads();
        size_t chunkSize = (total + numThreads - 1) / numThreads;
        size_t numChunks = (total + chunkSize - 1) / chunkSize;

        auto* scriptComponentArray = p_coordinator->getComponentArray<lynx::ecs::components::ScriptingBridgeComponent>();


            lynx::job_system::JobGroup group;

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
        lynx::engine::api::TransformAPI::ExecuteAllAccumulatedTransform();   
        lynx::engine::api::PhysicsAPI::ExecuteAllAccumulatedForces();        
        lynx::engine::api::EntityAPI::ResetMetadataPointer();    
        lynx::engine::api::CameraAPI::ResetMainCameraPointer();
        p_commandDispatcher->ExecuteAll();
    }
}

#endif //!ILEN_ONLY_ECS
