#include <ilen/render/render-system.h>
#include <ilen/core/engine-context.h>


namespace ilen::ecs::systems {
    void RenderSystem::Init(const EngineContext *engineContext) {
        p_jobSystem = engineContext->p_jobSystem.get();
        p_meshManager = engineContext->p_meshManager.get();
        p_coordinator = engineContext->p_coordinator.get();
        p_renderPipelineManager = engineContext->p_renderPipelineManager.get();
        p_materialManager = engineContext->p_materialManager.get();
        p_commandDispatcher = engineContext->p_commandDispatcher.get();
        p_textureManager = engineContext->p_textureManager.get();

        //Registramos los componentes que requiere esta cosa
        p_coordinator->registerComponent<VisibilityComponent>();
        p_coordinator->registerComponent<StaticRenderingData>();
        p_coordinator->registerComponent<StaticMaterialRenderingData>();
        p_coordinator->registerComponent<StaticTextureRenderingData>();
    }

    void RenderSystem::Update() {
        {
            ilen::time::ScopedTimer timer("Prep for Rendering System");
            //Actualizando el estado de los elementos cargados en paralelo
            p_meshManager->UpdateProcess();
            p_renderPipelineManager->UpdatePipelineWorks();
            p_textureManager->UpdateProcess();

            std::vector<Entity> entityList(entities.begin(), entities.end());
            size_t total = entityList.size();
            if(total == 0) return; //Early return

            size_t threadsAvailables = p_jobSystem->AvailableThreads();
            size_t chunkSize = (total + threadsAvailables - 1) / threadsAvailables;
            size_t numChunks = (total + chunkSize - 1) / chunkSize;

            auto* renderComponentArray = p_coordinator->getComponentArray<RenderComponent>();
            auto* visibilityComponentArray = p_coordinator->getComponentArray<VisibilityComponent>();
            auto* staticRenderDataArray = p_coordinator->getComponentArray<StaticRenderingData>();
            auto* staticMatRenderDataArray = p_coordinator->getComponentArray<StaticMaterialRenderingData>();
            auto* staticTexRenderDataArray = p_coordinator->getComponentArray<StaticTextureRenderingData>();
            auto* metadataDataArray = p_coordinator->getComponentArray<EntityMetadataComponent>();

            ilen::job_system::JobGroup group;

            for(size_t i = 0; i < numChunks; i++){
                size_t start = i * chunkSize;
                size_t end = std::min(start + chunkSize, total);

                p_jobSystem->SubmitJob([this, start, end, entityList, renderComponentArray, visibilityComponentArray, staticRenderDataArray, staticMatRenderDataArray, metadataDataArray, staticTexRenderDataArray]() {
                    for(size_t j = start; j < end; j++){
                        Entity entity = entityList[j];
                        auto* metadata = metadataDataArray->getData(entity);
                        auto* render = renderComponentArray->getData(entity);
                        if(!render) continue;
                        
                        if(!metadata->isActive()){
                            SetVisibilityValue(*render, entity, visibilityComponentArray, false);
                            continue;
                        }



                        SetVisibilityValue(*render, entity, visibilityComponentArray, true);
                        CheckMeshStatus(*render, entity, staticRenderDataArray);
                        CheckRenderPipelineStatus(*render, entity, staticMatRenderDataArray);
                        CheckTextureStatus(*render, entity, staticTexRenderDataArray);
                    }
                }, &group);
            }

            group.Wait();
            p_commandDispatcher->ExecuteAll();


           /* // Este method se debe llamar en cada fotograma.
            for(auto const& entity : entities){
                if(auto render = p_Coordinator->getComponent<RenderComponent>(entity); render != nullptr){
                    //En este loops estamos trabajando con una entidad con rendering. Toda entidad que renderice tiene que tener sus datos en VRAM, sino, hay que asignarlos
                    //Carga y obtención de Meshes
                    HaveVisibilityDefined(*render, entity);
                    CheckMeshStatus(*render, entity);
                    CheckRenderPipelineStatus(*render, entity);
                }
            }*/
        }
    }

    void RenderSystem::SetVisibilityValue(const RenderComponent &render, const Entity &entity, ComponentArray<VisibilityComponent>* componentArray, bool newValue) {
        //auto visibility = p_Coordinator->getComponent<VisibilityComponent>(entity);
        VisibilityComponent* visibility = nullptr;
        
        if(componentArray){
            visibility = componentArray->getData(entity);
        }

        if (visibility == nullptr) {
            componentArray->updateData(entity, VisibilityComponent{newValue});
            //p_coordinator->setComponent(entity, VisibilityComponent{});
        }else{
            visibility->isVisible = newValue;
        }
    }

    void RenderSystem::CheckMeshStatus(const RenderComponent &render, const Entity &entity, ComponentArray<StaticRenderingData>* componentArray) const {
        //auto staticMeshData = p_Coordinator->getComponent<StaticRenderingData>(entity);
        StaticRenderingData* staticMeshData = nullptr;
        
        if(componentArray){
            staticMeshData = componentArray->getData(entity);
        }

        if(staticMeshData == nullptr){
            //Solicitar la carga de estos datos
            auto solitudeResult = p_meshManager->getMeshHandlers(render.meshID);
            auto runtimeID = p_meshManager->getMeshRuntimeID(render.meshID);

            if(solitudeResult.lock() != nullptr && runtimeID != 0){
                //ILEN_LOG_P(DEBUG_PREFIX, "Added Mesh with runtime mesh ID %i and meshID %i", runtimeID, render.meshID);
                StaticRenderingData data{solitudeResult, runtimeID};
                componentArray->updateData(entity, std::move(data));
                //p_coordinator->setComponent(entity, std::move(data));
            }
            //continue; //Salta a la siguiente entidad, esta no puede hacer nada.
            //No hace continue porque sino anularia para ese frame empezar la creación del PSO
        }
        //Lo muevo para que se compruebe al momento de renderizar (no tiene sentido un doble chequeo, mucho menos porque aqui no se manda a regenerar)
        /*else{
            auto pointer = staticMeshData->staticBuffers.lock();
            if(pointer == nullptr){
                //Ya no existe, borramos el componente
                p_Coordinator->removeComponent<StaticRenderingData>(entity);
            }
            //Puntero es válido a datos de la GPU.
        }*/
    }

    void RenderSystem::CheckRenderPipelineStatus(const RenderComponent &render, const Entity &entity, ComponentArray<StaticMaterialRenderingData>* componentArray) const {
       // auto staticMaterialData = p_Coordinator->getComponent<StaticMaterialRenderingData>(entity);
        StaticMaterialRenderingData* staticMaterialData = nullptr;
        
        if(componentArray){
            staticMaterialData = componentArray->getData(entity);
        }

        if (staticMaterialData == nullptr) {
            auto material = p_materialManager->getMaterial(render.materialID);

            auto descriptor = IlenRenderPipelineDescriptor(material);

            auto psoSolicitude = p_renderPipelineManager->getRenderPipeline(descriptor);
            auto runtimeID = p_materialManager->getRuntimeMaterial(render.materialID);

            if (psoSolicitude.lock() != nullptr && runtimeID != 0) {
                //ILEN_LOG_P(DEBUG_PREFIX, "Added PSO with runtime material ID %i and MatID %i", runtimeID, render.materialID);
                StaticMaterialRenderingData data{psoSolicitude, runtimeID};
                componentArray->updateData(entity, std::move(data));
                //p_coordinator->setComponent(entity, std::move(data));
            }
        }
    }

    void RenderSystem::CheckTextureStatus(const RenderComponent& render, const Entity& entity, ComponentArray<StaticTextureRenderingData>* componentArray) const {
        StaticTextureRenderingData* staticTexData = nullptr;
        
        if(componentArray){
            staticTexData = componentArray->getData(entity);
        }

        if(staticTexData == nullptr){
            //Solicitar la carga de estos datos
            auto solitudeResult = p_textureManager->getTextureHandler(render.mainTexID);
            auto runtimeID = p_textureManager->getTextureRuntimeID(render.mainTexID);

            if(solitudeResult.lock() != nullptr && runtimeID != 0){
                //ILEN_LOG_P(DEBUG_PREFIX, "Added Mesh with runtime mesh ID %i and meshID %i", runtimeID, render.meshID);
                StaticTextureRenderingData data{solitudeResult, runtimeID};
                componentArray->updateData(entity, std::move(data));
                //p_coordinator->setComponent(entity, std::move(data));
            }
            //continue; //Salta a la siguiente entidad, esta no puede hacer nada.
            //No hace continue porque sino anularia para ese frame empezar la creación del PSO
        }
    }
}