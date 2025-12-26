#pragma once
#if ILEN_LINUX || ILEN_WINDOWS || ILEN_ANDROID
#include <ilen/render/gpu-interfaces/texture.h>
#include <memory>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace ilen::render {
    class VulkanTexture final: public IGPUTexture {
    public:
        //VulkanTexture(const IlenGPUTextureDescriptor& descriptor);
        VulkanTexture(const vk::Image& image, vk::Format imageFormat, vk::raii::Device& logicDevice); //Para crear la abstraccion directamente.
        ~VulkanTexture() = default;
        void* getNativeTexture() override;
    private:
        vk::Image _nativeImage = nullptr;
        vk::raii::ImageView _nativeImageView = nullptr;

        static inline const char* DEBUG_PREFIX = "VULKAN_TEXTURE_HANDLER";
    };
}

#endif