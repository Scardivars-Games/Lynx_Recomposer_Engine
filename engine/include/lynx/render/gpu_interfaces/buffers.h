#pragma once

#include <lynx/headers/enumerations.h>
#include <string>

namespace lynx::render {
    struct LynxBufferDescriptor {
        enumerations::LYNX_RENDER_GPU_BUFFER_TYPE bufferType;
        size_t bufferSize;
        size_t stride = 0;
        size_t count = 0;
        std::string bufferLabelName;
    };

    //Interfaz de la cual se crearan los buffers de la GPU según la API gráfica.
    class IGPUBuffer {
        public:
            virtual ~IGPUBuffer() = default;

            virtual const LynxBufferDescriptor& getDescriptor() const = 0;
            virtual void* getNativeHandle() const = 0;
            virtual void writeDataToBuffer(const void* data, size_t size, size_t offset) = 0;
    };
}