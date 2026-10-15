#if defined (__APPLE__)
#pragma once

#include <lynx/render/gpu_interfaces/buffers.h>

@protocol MTLBuffer;
@protocol MTLCommandQueue;
@protocol MTLDevice;

namespace lynx::render
{
    class MetalBuffer final: public IGPUBuffer {
        public:
            MetalBuffer(id<MTLDevice> device, const LynxBufferDescriptor& descriptor, const void* data, id<MTLCommandQueue> commandQueue);

            ~MetalBuffer() override;

            const LynxBufferDescriptor& getDescriptor() const override;
            void* getNativeHandle() const override;
            void writeDataToBuffer(const void* data, size_t size, size_t offset) override;
        private:
            __strong id<MTLBuffer> _buffer;
            LynxBufferDescriptor _descriptor;
            static inline const char* DEBUG_PREFIX = "METAL_BUFFERS";
    };
}

#endif
