#if defined (__APPLE__)
#pragma once

#include <ilen/render/gpu_interfaces/buffers.h>

@protocol MTLBuffer;
@protocol MTLCommandQueue;
@protocol MTLDevice;

namespace ilen::render
{
    class MetalBuffer final: public IGPUBuffer {
        public:
            MetalBuffer(id<MTLDevice> device, const IlenBufferDescriptor& descriptor, const void* data, id<MTLCommandQueue> commandQueue);

            ~MetalBuffer() override = default;

            const IlenBufferDescriptor& getDescriptor() const override;
            void* getNativeHandle() const override;
            void writeDataToBuffer(const void* data, size_t size, size_t offset) override;
        private:
            __strong id<MTLBuffer> _buffer;
            IlenBufferDescriptor _descriptor;
            static inline const char* DEBUG_PREFIX = "METAL_BUFFERS";
    };
}

#endif
