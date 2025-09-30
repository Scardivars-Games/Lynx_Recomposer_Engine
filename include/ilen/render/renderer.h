#pragma once

#include <string>
#include <ilen/headers/enumerations.h>

namespace ilen::render {

    enum class GraphicsAPI {
        Metal,
        Vulkan,
        OpenGL,
        DirectX12,
    };

    struct IlenBufferDescriptor {
        enumerations::ILEN_RENDER_GPU_BUFFER_TYPE bufferType;
        size_t bufferSize;
        size_t stride = 0;
        size_t count = 0;
        bool cpuVisible = false;
    };

    //Interfaz de la cual se crearan los buffers de la GPU según la API gráfica.
    class IGPUBuffer {
        public:
            virtual ~IGPUBuffer() = default;

            virtual const IlenBufferDescriptor& getDescriptor() const = 0;
            virtual void* getNativeHandle() const = 0;
            
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