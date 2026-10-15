#pragma once

#if defined(__APPLE__)

#include <lynx/render/gpu_interfaces/instance_buffer.h>
#include <memory>
#include <atomic>
#include <Metal/buffers_metal.h>

namespace lynx::render {
    class MetalInstanceBuffer final : public IGPUInstanceBuffer {
    public:
        MetalInstanceBuffer(id<MTLDevice> device, size_t bufferSize, const std::string& bufferName);
        ~MetalInstanceBuffer() override;

        uint32_t UploadInstanceData(const void *data, size_t size) override;
        IGPUBuffer* GetBuffer() const override;
        void OnBeginFrame() override;

    private:
        size_t Align(size_t size);
        std::unique_ptr<MetalBuffer> _buffer;
        std::atomic<size_t> _currentOffset;
        size_t _totalSize;
        
        id<MTLBuffer> _nativeBuffer;

        static const size_t bufferAlign = 256;
        static inline const char* DEBUG_PREFIX = "METAL_INSTANCE_BUFFER";
    };
}

#endif
