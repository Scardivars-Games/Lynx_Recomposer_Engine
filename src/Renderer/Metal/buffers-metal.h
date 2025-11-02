#if defined (__APPLE__)
#pragma once

#include <ilen/render/gpu-interfaces/buffers.h>

@protocol MTLBuffer;
@protocol MTLCommandQueue;
@protocol MTLDevice;

namespace ilen::render
{
    class MetalBuffer : public IGPUBuffer {
        public:
            MetalBuffer(id<MTLDevice> device, const IlenBufferDescriptor& descriptor, const void* data, id<MTLCommandQueue> commandQueue);

            ~MetalBuffer() override = default;

            const IlenBufferDescriptor& getDescriptor() const override;
            void* getNativeHandle() const override;
        private:
            __strong id<MTLBuffer> _buffer;
            IlenBufferDescriptor _descriptor;
    };
}

#endif
