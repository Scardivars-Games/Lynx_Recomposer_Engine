#pragma once

#include <lynx/core/lifetime.h>
#include <lynx/render/gpu_interfaces/render_pipelines.h>
//#include <lynx/resources/materials-manager.h>
#include <memory>
#include <future>

namespace lynx::resources::materials {
    class MaterialManager;
}

namespace lynx::resources::render_pipeline {
    using namespace lynx::render;

    class PipelineManager {
        public:
            explicit PipelineManager(EngineContext* context);
            std::weak_ptr<IGPURenderPipeline> getRenderPipeline(const LynxRenderPipelineDescriptor& descriptor);
            void UpdatePipelineWorks();
            float LoadingStatus();
        private:
            //void generatePipeline();
            std::map<LynxRenderPipelineDescriptor, std::shared_ptr<IGPURenderPipeline>> m_pipelines;
            std::map<LynxRenderPipelineDescriptor, std::future<std::shared_ptr<IGPURenderPipeline>>> m_loadingPipelines;
            std::mutex m_mutex;
            uint16_t m_nextPsoID = 1;

            uint32_t lastTimeAmountOfElementLoading = 0;

            IRenderer* p_renderer;
            shaders::ShaderManager* p_shaderManager;
            materials::MaterialManager* p_matManager;
            job_system::JobSystem* p_jobSystem;
    };
}