#pragma once

#include <ilen/render/gpu_interfaces/buffers.h>

namespace ilen::render {
    class IGPUInstanceBuffer {
    public:
        virtual ~IGPUInstanceBuffer() = default;

        virtual uint32_t UploadInstanceData(const void* data, size_t size) = 0;
        virtual IGPUBuffer* GetBuffer() const = 0;
        virtual void OnBeginFrame() = 0;
    };
}