#pragma once

#include <string>
#include <lynx/headers/enumerations.h>
#include <lynx/render/gpu_interfaces/render_pipelines.h>
#include <lynx/render/gpu_interfaces/buffers.h>
#include <lynx/render/gpu_interfaces/shaders.h>
#include <lynx/render/gpu_interfaces/parallel_command_encoder.h>
#include <lynx/render/gpu_interfaces/instance_buffer.h>
#include <lynx/render/gpu_interfaces/sampler.h>
#include <lynx/graphics/shader.h>
#include <lynx/core/runtime_configuration.h>

namespace lynx::graphics
{
    struct LynxShader;
}

namespace lynx::resources::shaders {
    class ShaderManager;
}

namespace lynx::ecs::systems {
    class RenderDrawSystem;
}

namespace lynx::render {
    class IShader;
    class IGPURenderPipeline;
    struct LynxRenderPipelineDescriptor;
    class IGPUParallelCommandEncoder;

    //This is intended to be used only by the main thread. Once the parallel Encoder is obtained, it can be used to generate encoder for secondary threads.
    class IRenderer {
        friend class lynx::ecs::systems::RenderDrawSystem;

    public:
        virtual ~IRenderer() = default;
        virtual void beginFrame() = 0;
        virtual void endFrame() = 0;
        virtual void endEncoding(IGPUParallelCommandEncoder* encoder) = 0;
        virtual void regenerateEncoderDescriptor() = 0;
        virtual inline void defineSupportedGPUCompression() = 0;

        //Builder for GPU Interfaces
        virtual std::shared_ptr<IGPUBuffer> createBuffer(const LynxBufferDescriptor& descriptor, const void* data) = 0;
        virtual std::shared_ptr<IShader> createShader(const lynx::graphics::LynxShader& shaderDescriptor) = 0;
        virtual std::shared_ptr<IGPURenderPipeline> createRenderPipeline(const lynx::render::LynxRenderPipelineDescriptor& pipelineDescriptor, lynx::resources::shaders::ShaderManager* shaderManager) = 0;
        virtual std::unique_ptr<IGPUParallelCommandEncoder> createNewParallelCommandEncoder(LynxParallelEncoderDescriptor&& descriptor) = 0;
        virtual std::unique_ptr<IGPUTexture> createNewPlatformTexture(const LynxGPUTextureDescriptor& descriptor) = 0; //Crea una textura para uso interno del renderer (Presentation, attachments, etc). NO USAR PARA UNA TEXTURA DEL JUEGO
        virtual std::shared_ptr<IGPUTexture> createTexture(const LynxGPUTextureDescriptor& descriptor, const void* initialData = nullptr) = 0;
        virtual std::shared_ptr<IGPUSampler> createSampler(const LynxGPUSamplerDescriptor& descriptor) = 0;
        virtual std::unique_ptr<IGPUInstanceBuffer> createInstanceBuffer(size_t bufferSize, std::string bufferName) = 0;

        //getters of encoders and buffers.
        virtual std::unique_ptr<IGPUParallelCommandEncoder> getParallelCommandEncoder() = 0;
        virtual IGPUTexture* getDrawableTexturePtr() = 0;
        virtual std::vector<IGPUTexture*> getAllSceneTexturesPtr() = 0;
        virtual void returnParallelCommandEncoder(std::unique_ptr<IGPUParallelCommandEncoder> encoder) = 0;
        virtual IGPUInstanceBuffer* getInstanceBuffer() = 0;
        uint8_t getActualNumOfFramesInFlight() const { return MAX_FRAMES_IN_FLIGHT; }
        uint8_t getActualFrameInFlight() const { return _actualFrame; }

        static std::unique_ptr<IRenderer> CreateRenderer(const void* native_surface, LYNX_GPU_GRAPHICS_API_TYPE api);
        static LYNX_GPU_GRAPHICS_API_TYPE getActualGraphicsAPI() { return actualAPI; }
        static LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE getActualSupportedTextureCompressionFormat() { return compressionActualFormat; }
        protected:
            LynxGraphicsOptionsFlags lastUsedGraphicsOptions;
            LynxParallelEncoderDescriptor actualMainDescriptor;
            static void FinalizeEncoding(IGPUParallelCommandEncoder* commandEncoder) {
                if (commandEncoder) {
                    commandEncoder->EndEncoding();
                }
            }

            const uint8_t MAX_FRAMES_IN_FLIGHT = 3;
            uint8_t _actualFrame = 0;
            static inline const char* DEBUG_PREFIX = "RENDER";
            static inline LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE compressionActualFormat = LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_INVALID;
            bool isParallelEncoderInOtherObject = false; //Is the way to know at the end frame if the parallel encoder is nullptr cause there is no render target, or was given to the one renderer, but it don't return
            //that or another encoder the be used to present a drawable to screen.
        private:
            static inline LYNX_GPU_GRAPHICS_API_TYPE actualAPI = LYNX_GPU_GRAPHICS_API_TYPE::Auto;
    };
}
