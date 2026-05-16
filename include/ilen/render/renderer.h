#pragma once

#include <string>
#include <ilen/headers/enumerations.h>
#include <ilen/render/gpu-interfaces/render-pipelines.h>
#include <ilen/render/gpu-interfaces/buffers.h>
#include <ilen/render/gpu-interfaces/shaders.h>
#include <ilen/render/gpu-interfaces/parallel-command-encoder.h>
#include <ilen/render/gpu-interfaces/instance-buffer.h>
#include <ilen/render/gpu-interfaces/sampler.h>
#include <ilen/graphics/shader.h>
#include <ilen/core/runtime_options.h>

namespace ilen::graphics
{
    struct IlenShader;
}

namespace ilen::resources::shaders {
    class ShaderManager;
}

namespace ilen::ecs::systems {
    class RenderDrawSystem;
}

namespace ilen::render {
    class IShader;
    class IGPURenderPipeline;
    struct IlenRenderPipelineDescriptor;
    class IGPUParallelCommandEncoder;

    //This is intended to be used only by the main thread. Once the parallel Encoder is obtained, it can be used to generate encoder for secondary threads.
    class IRenderer {
        friend class ilen::ecs::systems::RenderDrawSystem;

    public:
        virtual ~IRenderer() = default;
        virtual void beginFrame() = 0;
        virtual void endFrame() = 0;
        virtual void endEncoding(IGPUParallelCommandEncoder* encoder) = 0;
        virtual void regenerateEncoderDescriptor() = 0;
        virtual inline void defineSupportedGPUCompression() = 0;

        //Builder for GPU Interfaces
        virtual std::shared_ptr<IGPUBuffer> createBuffer(const IlenBufferDescriptor& descriptor, const void* data) = 0;
        virtual std::shared_ptr<IShader> createShader(const ilen::graphics::IlenShader& shaderDescriptor) = 0;
        virtual std::shared_ptr<IGPURenderPipeline> createRenderPipeline(const ilen::render::IlenRenderPipelineDescriptor& pipelineDescriptor, ilen::resources::shaders::ShaderManager* shaderManager) = 0;
        virtual std::unique_ptr<IGPUParallelCommandEncoder> createNewParallelCommandEncoder(IlenParallelEncoderDescriptor&& descriptor) = 0;
        virtual std::unique_ptr<IGPUTexture> createNewPlatformTexture(const IlenGPUTextureDescriptor& descriptor) = 0; //Crea una textura para uso interno del renderer (Presentation, attachments, etc). NO USAR PARA UNA TEXTURA DEL JUEGO
        virtual std::shared_ptr<IGPUTexture> createTexture(const IlenGPUTextureDescriptor& descriptor, const void* initialData = nullptr) = 0;
        virtual std::shared_ptr<IGPUSampler> createSampler(const IlenGPUSamplerDescriptor& descriptor) = 0;
        virtual std::unique_ptr<IGPUInstanceBuffer> createInstanceBuffer(size_t bufferSize, std::string bufferName) = 0;

        //getters of encoders and buffers.
        virtual std::unique_ptr<IGPUParallelCommandEncoder> getParallelCommandEncoder() = 0;
        virtual IGPUTexture* getDrawableTexturePtr() = 0;
        virtual std::vector<IGPUTexture*> getAllSceneTexturesPtr() = 0;
        virtual void returnParallelCommandEncoder(std::unique_ptr<IGPUParallelCommandEncoder> encoder) = 0;
        virtual IGPUInstanceBuffer* getInstanceBuffer() = 0;
        uint8_t getActualNumOfFramesInFlight() const { return MAX_FRAMES_IN_FLIGHT; }
        uint8_t getActualFrameInFlight() const { return _actualFrame; }

        static std::unique_ptr<IRenderer> CreateRenderer(const void* native_surface, ILEN_GPU_GRAPHICS_API_TYPE api);
        static ILEN_GPU_GRAPHICS_API_TYPE getActualGraphicsAPI() { return actualAPI; }
        static ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE getActualSupportedTextureCompressionFormat() { return compressionActualFormat; }
        protected:
            IlenGraphicsOptionsFlags lastUsedGraphicsOptions;
            IlenParallelEncoderDescriptor actualMainDescriptor;
            static void FinalizeEncoding(IGPUParallelCommandEncoder* commandEncoder) {
                if (commandEncoder) {
                    commandEncoder->EndEncoding();
                }
            }

            const uint8_t MAX_FRAMES_IN_FLIGHT = 3;
            uint8_t _actualFrame = 0;
            static inline const char* DEBUG_PREFIX = "RENDER";
            static inline ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE compressionActualFormat = ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_INVALID;
            bool isParallelEncoderInOtherObject = false; //Is the way to know at the end frame if the parallel encoder is nullptr cause there is no render target, or was given to the one renderer, but it don't return
            //that or another encoder the be used to present a drawable to screen.
        private:
            static inline ILEN_GPU_GRAPHICS_API_TYPE actualAPI = ILEN_GPU_GRAPHICS_API_TYPE::Auto;
    };
}
