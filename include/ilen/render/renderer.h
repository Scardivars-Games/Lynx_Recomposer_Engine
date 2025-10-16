#pragma once

#include <string>
#include <ilen/headers/enumerations.h>
#include <ilen/render/gpu-interfaces/render-pipelines.h>
#include <ilen/render/gpu-interfaces/buffers.h>

namespace ilen::render {

    enum class GraphicsAPI {
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


    class Renderer {
        public:
            virtual ~Renderer() = default;
            virtual void beginFrame() = 0;
            virtual void endFrame() = 0;

            virtual std::shared_ptr<IGPUBuffer> createBuffer(const IlenBufferDescriptor& descriptor, const void* data = nullptr) = 0;

            static Renderer* create(void* native_surface);
        protected:
            static inline const char* DEBUG_PREFIX = "RENDER";
    };
}