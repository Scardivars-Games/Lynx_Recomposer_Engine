#include <ilen/resources/render-pipeline-manager.h>

namespace ilen::resources::render_pipeline {
    PipelineManager::PipelineManager(EngineContext* context) {
        p_renderer = context->p_gameRenderer.get();
        p_matManager = context->p_materialManager.get();
        p_shaderManager = context->p_shaderManager.get();
        p_jobSystem = context->p_jobSystem.get();
    }

    std::weak_ptr<IGPURenderPipeline> PipelineManager::getRenderPipeline(const IlenRenderPipelineDescriptor& descriptor) {
        std::lock_guard lock(m_mutex);

        //Primero, buscar si existe la Pipeline ya creada en el mapa, sino se solicita crear una
        auto it = m_pipelines.find(descriptor);
        if (it != m_pipelines.end()) {
            return it->second;
        }

        auto isLoad = m_loadingPipelines.find(descriptor);
        if (isLoad != m_loadingPipelines.end()) {
            return {}; //Se esta cargando
        }

        std::future<std::shared_ptr<IGPURenderPipeline>> future = p_jobSystem->SubmitJob([this, descriptor] -> std::shared_ptr<IGPURenderPipeline> {
            //Hay que generar una nueva PSO.
            auto newPSO = p_renderer->createRenderPipeline(descriptor, p_shaderManager);
            //  m_pipelines[descriptor] = newPSO;
            return newPSO;
        });

        m_loadingPipelines[descriptor] = std::move(future);
        return {};
    }

    void PipelineManager::UpdatePipelineWorks() {
        std::lock_guard lock(m_mutex);

        for (auto it = m_loadingPipelines.begin(); it != m_loadingPipelines.end(); /* no-op */) {
            auto& [descriptor, future] = *it;

            if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                std::shared_ptr<IGPURenderPipeline> newPSO = future.get();

                m_nextPsoID++;
                newPSO->setID(m_nextPsoID);

                m_pipelines[descriptor] = std::move(newPSO);
                it = m_loadingPipelines.erase(it);
            } else {
                ++it;
            }
        }
    }
}