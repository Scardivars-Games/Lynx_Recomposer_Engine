#if defined (__APPLE__)
#pragma once

#include <ilen/render/gpu-interfaces/buffers.h>

@protocol MTLBuffer;

namespace ilen::render
{
    class MetalBuffer : public IGPUBuffer {
        public:
            MetalBuffer(id<MTLBuffer> buffer, const IlenBufferDescriptor& descriptor);

            ~MetalBuffer() override = default;

            const IlenBufferDescriptor& getDescriptor() const override;
            void* getNativeHandle() const override;
        private:
            __strong id<MTLBuffer> _buffer;
            IlenBufferDescriptor _descriptor;
    };
}

#endif
