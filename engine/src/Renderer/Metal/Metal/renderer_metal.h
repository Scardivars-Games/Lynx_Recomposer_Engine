#include "lynx/core/runtime_configuration.h"
#include "lynx/graphics/color.h"
#if defined (__APPLE__)
#pragma once

#include <lynx/render/renderer.h>
#include <memory>
#include <lynx/render/gpu_interfaces/parallel_command_encoder.h>
#include <Metal_Common/common_structures_metal.h>
#include <dispatch/dispatch.h>
#include <lynx/core/runtime_configuration.h>

#if defined LYNX_MACOS
#import <Cocoa/Cocoa.h>
#elif defined LYNX_IOS || LYNX_TVOS
#import <UIKit/UIKit.h>
#endif

@protocol MTLDevice;
@protocol MTLCommandQueue;
@protocol MTLLibrary;
@protocol MTLCommandBuffer;
@protocol MTLRenderCommandEncoder;
@class CAMetalLayer;
@class NSView;
@protocol CAMetalDrawable;
@protocol MTLTexture;

namespace lynx::render {
    class MetalRender final: public IRenderer {
        public:
            id<MTLDevice> _device;
            id<MTLCommandQueue> _commandQueue;
            id<MTLLibrary> _lynxMetalLibrary;
#ifdef LYNX_MACOS
            NSView* _metalView;
#elif defined LYNX_IOS || LYNX_TVOS
            UIView* _metalView;
#endif
            CAMetalLayer* _metalLayer;

            MetalRender(const void* native_surface);

            ~MetalRender() override;

            void beginFrame() override;

            void endFrame() override;
            void endEncoding(IGPUParallelCommandEncoder* encoder) override;
            void regenerateEncoderDescriptor() override;
            void setNewRendererClearColor(graphics::Color newClearColor) override;
            inline void defineSupportedGPUCompression() override;
        
            bool acquireDrawable();

            std::shared_ptr<IGPUBuffer> createBuffer(const LynxBufferDescriptor& descriptor, const void* data = nullptr) override;
            std::unique_ptr<IGPUInstanceBuffer> createInstanceBuffer(size_t bufferSize, std::string bufferName) override;
            std::shared_ptr<IShader> createShader(const lynx::graphics::LynxShader& shaderDescriptor) override;
            std::shared_ptr<IGPURenderPipeline> createRenderPipeline(const lynx::render::LynxRenderPipelineDescriptor& pipelineDescriptor, lynx::resources::shaders::ShaderManager* shaderManager) override;
            std::unique_ptr<IGPUParallelCommandEncoder> getParallelCommandEncoder() override;
            std::unique_ptr<IGPUParallelCommandEncoder> createNewParallelCommandEncoder(LynxParallelEncoderDescriptor&& descriptor) override;
            std::unique_ptr<IGPUTexture> createNewPlatformTexture(const LynxGPUTextureDescriptor& descriptor) override;
            std::shared_ptr<IGPUTexture> createTexture(const LynxGPUTextureDescriptor& descriptor, const void* initialData = nullptr) override;
            std::shared_ptr<IGPUSampler> createSampler(const LynxGPUSamplerDescriptor& descriptor) override;

            IGPUTexture* getDrawableTexturePtr() override;
            std::vector<IGPUTexture*> getAllSceneTexturesPtr() override;

            void returnParallelCommandEncoder(std::unique_ptr<IGPUParallelCommandEncoder> encoder) override;
            IGPUInstanceBuffer* getInstanceBuffer() override;
        private:
            std::vector<id<MTLCommandBuffer>> _commandBuffer;
            void* _frameAutoReleasePool;

            std::vector<std::unique_ptr<IGPUParallelCommandEncoder>> _parallelCommandEncoder;
            std::vector<std::unique_ptr<IGPUInstanceBuffer>> _instanceBuffer;

            std::vector<dispatch_semaphore_t> _frameSemaphores;
        
            std::vector<std::unique_ptr<IGPUTexture>> _sceneTexture;
            std::vector<std::unique_ptr<IGPUTexture>> _depthTexture;
            std::vector<std::unique_ptr<IGPUTexture>> _drawableTexture;
            std::vector<id<CAMetalDrawable>> _drawables;

            void generateNewMetalLibrary(); //Método fallback que busca todos los archivos fuente de .metal y los compila como un único .metallib
            void generateFrameTextures();
        
            //MTLClearColor _renderingClearColor;
            lynx::render::utils::metal::MetalViewPredefinedValues _predefinedValues;

            bool _drawableAcquired;

            core::RuntimeConfiguration* p_runtimeConfiguration;
    };
}

#endif
