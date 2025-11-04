#if defined (__APPLE__)
#pragma once

#include <ilen/render/renderer.h>
#include <memory>
#include <ilen/render/gpu-interfaces/parallel-command-encoder.h>

@protocol MTLDevice;
@protocol MTLCommandQueue;
@protocol MTLLibrary;
@protocol MTLCommandBuffer;
@protocol MTLRenderCommandEncoder;
@class MTKView;

namespace ilen::render
{
    class MetalRender : public IRenderer {
        public:
            id<MTLDevice> _device;
            id<MTLCommandQueue> _commandQueue;
            id<MTLLibrary> _ilenMetalLibrary;
            MTKView* metalView;

            MetalRender(void* native_surface);

            ~MetalRender() override;

            void beginFrame() override;

            void endFrame() override;

            std::shared_ptr<IGPUBuffer> createBuffer(const IlenBufferDescriptor& descriptor, const void* data = nullptr) override;
            std::shared_ptr<IShader> createShader(const ilen::graphics::IlenShader& shaderDescriptor) override;
            
            std::shared_ptr<IGPURenderPipeline> createRenderPipeline(const ilen::render::IlenRenderPipelineDescriptor& pipelineDescriptor, ilen::resources::shaders::ShaderManager* shaderManager) override;

            IGPUParallelCommandEncoder* getParallelCommandEncoder() override;
        private:
            id<MTLCommandBuffer> _commandBuffer;
            void* _frameAutoReleasePool;

            std::unique_ptr<IGPUParallelCommandEncoder> _parallelCommandEncoder;

            void generateNewMetalLibrary(); //Método fallback que busca todos los archivos fuente de .metal y los compila como un único .metallib
    };
}

#endif
