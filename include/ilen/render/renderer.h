#pragma once

#include <string>
#include <ilen/headers/enumerations.h>
#include <ilen/render/gpu-interfaces/render-pipelines.h>
#include <ilen/render/gpu-interfaces/buffers.h>
#include <ilen/render/gpu-interfaces/shaders.h>
#include <ilen/graphics/shader.h>

namespace ilen::graphics
{
    struct IlenShader;
}

namespace ilen::render {
    class IShader;

    enum class GraphicsAPI {
        Auto,
        Metal,
        Vulkan,
        OpenGL,
        DirectX12,
    };

    const static VertexBufferLayout ILEN_GLOBAL_VERTEX_LAYOUT {
        .stride = 40, // sizeof(float(4 bytes)) * 10 = 40 bytes
        .attributes = {
            VertexBufferFormats::Float4, //Position
            VertexBufferFormats::Float4, //Color
            VertexBufferFormats::Float2 //UV Coords
        }
    };


    class IRenderer {
        public:
            virtual ~IRenderer() = default;
            virtual void beginFrame() = 0;
            virtual void endFrame() = 0;

            //Builder for GPU Interfaces
            virtual std::shared_ptr<IGPUBuffer> createBuffer(const IlenBufferDescriptor& descriptor, const void* data) = 0;
            virtual std::shared_ptr<IShader> createShader(const ilen::graphics::IlenShader& shaderDescriptor) = 0;

            static std::unique_ptr<IRenderer> CreateRenderer(void* native_surface, GraphicsAPI api);
            //static Renderer* create(void* native_surface);
        protected:
            static inline const char* DEBUG_PREFIX = "RENDER";
    };
}
