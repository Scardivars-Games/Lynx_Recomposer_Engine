#pragma once

#include <ilen/core/lifetime.h>
#include <ilen/render/gpu-interfaces/render-pipelines.h>
//#include <ilen/resources/materials-manager.h>
#include <memory>
#include <future>

namespace ilen::resources::materials {
    class MaterialManager;
}

namespace ilen::resources::render_pipeline {
    using namespace ilen::render;

    class PipelineManager {
        public:
            explicit PipelineManager(EngineContext* context);
            std::weak_ptr<IGPURenderPipeline> getRenderPipeline(const IlenRenderPipelineDescriptor& descriptor);
            void UpdatePipelineWorks();
        private:
            //void generatePipeline();
            std::map<IlenRenderPipelineDescriptor, std::shared_ptr<IGPURenderPipeline>> m_pipelines;
            std::map<IlenRenderPipelineDescriptor, std::future<std::shared_ptr<IGPURenderPipeline>>> m_loadingPipelines;
            std::mutex m_mutex;
            uint16_t m_nextPsoID = 1;

            IRenderer* p_renderer;
            shaders::ShaderManager* p_shaderManager;
            materials::MaterialManager* p_matManager;
            job_system::JobSystem* p_jobSystem;
    };
}