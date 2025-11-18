#pragma once
#if ILEN_LINUX || ILEN_WINDOWS || ILEN_ANDROID
#include <ilen/render/renderer.h>

namespace ilen::render {
    class VulkanRender final: public IRenderer {
        public:
            VulkanRender(void* native_surface);
            ~VulkanRender() override;
            void beginFrame() override;
            void endFrame() override;
            void endEncoding(IGPUParallelCommandEncoder* encoder) override;

            //Builder for GPU Interfaces
            std::shared_ptr<IGPUBuffer> createBuffer(const IlenBufferDescriptor& descriptor, const void* data) override;
            std::shared_ptr<IShader> createShader(const ilen::graphics::IlenShader& shaderDescriptor) override;
            std::shared_ptr<IGPURenderPipeline> createRenderPipeline(const ilen::render::IlenRenderPipelineDescriptor& pipelineDescriptor, ilen::resources::shaders::ShaderManager* shaderManager) override;
            std::unique_ptr<IGPUParallelCommandEncoder> createNewParallelCommandEncoder(const IlenParallelEncoderDescriptor& descriptor) override;
            std::unique_ptr<IGPUTexture> createNewPlatformTexture(const IlenGPUTextureDescriptor& descriptor) override;

            //getters of encoders and buffers.
            std::unique_ptr<IGPUParallelCommandEncoder> getParallelCommandEncoder() override;
            void returnParallelCommandEncoder(std::unique_ptr<IGPUParallelCommandEncoder> encoder) override;
            IGPUInstanceBuffer* getInstanceBuffer() override;
    };
}
#endif
