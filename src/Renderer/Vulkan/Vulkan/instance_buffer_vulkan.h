#pragma once
#if ILEN_LINUX || ILEN_WINDOWS || ILEN_ANDROID
#include <ilen/render/gpu-interfaces/instance-buffer.h>
#include <ilen/headers/ilen-logs.h>
#include <vulkan/vulkan_raii.hpp>
#include <Vulkan/buffers-vulkan.h>
#include <atomic>


namespace ilen::render {
    class VulkanInstanceBuffer final: public IGPUInstanceBuffer {
        public:
            VulkanInstanceBuffer(vk::raii::Device& device, vk::raii::PhysicalDevice& physicalDevice, vk::CommandPool commandPool, vk::raii::Queue& graphicsQueue, size_t bufferSize, const std::string bufferName, const IlenBufferDescriptor& descriptor);
            ~VulkanInstanceBuffer() override;
            
            uint32_t UploadInstanceData(const void* data, size_t size) override;
            IGPUBuffer* GetBuffer() const override;
            void OnBeginFrame() override;

        private:
            size_t Align(size_t size);

            std::unique_ptr<VulkanBuffer> _buffer;
            std::atomic<size_t> _currentOffset;
            size_t _totalSize;

            static const size_t bufferAlign = 16;
            static inline const char* DEBUG_PREFIX = "VULKAN_INSTANCE_BUFFER";
    };
}

#endif