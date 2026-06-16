#pragma once
#if ILEN_LINUX || ILEN_WINDOWS || ILEN_ANDROID
#include <ilen/render/gpu_interfaces/buffers.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <Vulkan/engine_to_native_vulkan_converters.h>
#include <ilen/headers/ilen_logs.h>

namespace ilen::render {
    class VulkanRender;

    class VulkanBuffer final: public IGPUBuffer {
        public:
            VulkanBuffer(uint32_t vulkanVersion, vk::raii::Device& device, vk::raii::PhysicalDevice& physicalDevice,
                    vk::CommandPool commandPool, vk::raii::Queue& graphicsQueue,
                    const IlenBufferDescriptor& descriptor, const void* initialData);
            ~VulkanBuffer() override;

            const IlenBufferDescriptor& getDescriptor() const override;
            void* getNativeHandle() const override;
            void writeDataToBuffer(const void* data, size_t size, size_t offset) override;
        private:
            vk::Device _device;
            vk::PhysicalDevice _physicalDevice;
            vk::CommandPool _commandPool;
            vk::Queue _queue;

            vk::Buffer _buffer;
            vk::DeviceMemory _memory;
            IlenBufferDescriptor _descriptor;

            uint32_t _vulkanVersion;

            static inline const char* DEBUG_PREFIX = "VULKAN_BUFFER";

            uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags propertyFlags);
            void createBufferInternal(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& memory);
            void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size, vk::DeviceSize srcOffset, vk::DeviceSize dstOffset);
    };
}

#endif