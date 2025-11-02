#include <ilen/render/render-system.h>

namespace ilen::ecs::systems {
    void RenderSystem::Init(EngineContext *engineContext) {
        p_jobSystemInstance = engineContext->p_jobSystem.get();
        p_meshManager = engineContext->p_meshManager.get();
        p_Coordinator = engineContext->p_coordinator.get();
        p_renderPipelineManager = engineContext->p_renderPipelineManager.get();
        p_materialManager = engineContext->p_materialManager.get();
    }

    void RenderSystem::Update() {
        p_meshManager->UpdateProcess();
        // Este método se debe llamar en cada fotograma.
        for(auto const& entity : entities){
            if(auto render = p_Coordinator->getComponent<RenderComponent>(entity); render != nullptr){
                //En este loops estamos trabajando con una entidad con rendering. Toda entidad que renderice tiene que tener sus datos en VRAM, sino, hay que asignarlos
                //Carga y obtención de Meshes
                HaveVisibilityDefined(*render, entity);
                CheckMeshStatus(*render, entity);
                CheckRenderPipelineStatus(*render, entity);
            }
        }
    }

    void RenderSystem::HaveVisibilityDefined(const RenderComponent &render, const Entity &entity) {
        auto visibility = p_Coordinator->getComponent<VisibilityComponent>(entity);

        if (visibility == nullptr) {
            p_Coordinator->addComponent(entity, VisibilityComponent{});
        }
    }

    void RenderSystem::CheckMeshStatus(const RenderComponent &render, const Entity &entity) const {
        auto staticMeshData = p_Coordinator->getComponent<StaticRenderingData>(entity);

        if(staticMeshData == nullptr){
            //Solicitar la carga de estos datos
            auto solitudeResult = p_meshManager->getMeshHandlers(render.meshID);

            if(solitudeResult.lock() != nullptr){
                StaticRenderingData data{};
                data.staticBuffers = solitudeResult;
                p_Coordinator->addComponent(entity, std::move(data));
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

    void RenderSystem::CheckRenderPipelineStatus(const RenderComponent &render, const Entity &entity) const {
        auto staticMaterialData = p_Coordinator->getComponent<StaticMaterialRenderingData>(entity);

        if (staticMaterialData == nullptr) {
            auto material = p_materialManager->getMaterial(render.materialID);

            auto descriptor = IlenRenderPipelineDescriptor(material);

            auto psoSolicitude = p_renderPipelineManager->getRenderPipeline(descriptor);
            if (psoSolicitude.lock() != nullptr) {
                StaticMaterialRenderingData data{};
                data.pso = psoSolicitude;
                p_Coordinator->addComponent(entity, std::move(data));
            }
        }
    }
}