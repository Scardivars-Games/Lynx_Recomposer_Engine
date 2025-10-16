#pragma once

#include <ilen/headers/enumerations.h>
#include <string>

namespace ilen::render {
    struct IlenBufferDescriptor {
        enumerations::ILEN_RENDER_GPU_BUFFER_TYPE bufferType;
        size_t bufferSize;
        size_t stride = 0;
        size_t count = 0;
        std::string bufferLabelName;
    };

    //Interfaz de la cual se crearan los buffers de la GPU según la API gráfica.
    class IGPUBuffer {
        public:
            virtual ~IGPUBuffer() = default;

            virtual const IlenBufferDescriptor& getDescriptor() const = 0;
            virtual void* getNativeHandle() const = 0;
            
    };
}