#pragma once

#include <string>
#include <ilen/headers/enumerations.h>
#include <ilen/render/gpu-interfaces/render-pipelines.h>
#include <ilen/render/gpu-interfaces/buffers.h>
#include <ilen/render/gpu-interfaces/shaders.h>
//#include <ilen/resources/shaders-manager.h>
#include <ilen/graphics/shader.h>

namespace ilen::graphics
{
    struct IlenShader;
}

namespace ilen::resources::shaders {
    class ShaderManager;
}

namespace ilen::render {
    class IShader;
    class IGPURenderPipeline;
    struct IlenRenderPipelineDescriptor;

    enum class GraphicsAPI {
        Auto,
        Metal,
        Vulkan,
        OpenGL,
        DirectX12,
    };

    class IRenderer {
        public:
            virtual ~IRenderer() = default;
            virtual void beginFrame() = 0;
            virtual void endFrame() = 0;

            //Builder for GPU Interfaces
            virtual std::shared_ptr<IGPUBuffer> createBuffer(const IlenBufferDescriptor& descriptor, const void* data) = 0;
            virtual std::shared_ptr<IShader> createShader(const ilen::graphics::IlenShader& shaderDescriptor) = 0;
            virtual std::shared_ptr<IGPURenderPipeline> createRenderPipeline(const ilen::render::IlenRenderPipelineDescriptor& pipelineDescriptor, ilen::resources::shaders::ShaderManager* shaderManager) = 0;

            static std::unique_ptr<IRenderer> CreateRenderer(void* native_surface, GraphicsAPI api);
        protected:
            static inline const char* DEBUG_PREFIX = "RENDER";
    };
}
