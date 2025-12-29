#pragma once
#if ILEN_LINUX || ILEN_WINDOWS || ILEN_ANDROID

#include <vulkan/vulkan.hpp>

namespace ilen::render::vulkan {
    struct VulkanAttachmentData {
        vk::Format format;
        vk::SampleCountFlagBits samples;
        vk::AttachmentLoadOp loadOp;
        vk::AttachmentStoreOp storeOp;
        vk::AttachmentLoadOp stencilLoadOp;
        vk::AttachmentStoreOp stencilStoreOp;    
        vk::ImageLayout finalLayout;
        vk::ImageLayout initialLayout;
        uint32_t attachSlot;

        vk::AttachmentDescription getV1Descriptor();
        vk::AttachmentDescription2 getV2Descriptor();
        vk::AttachmentReference2 getV2Reference();
        vk::AttachmentReference getV1Reference();
    };
}

#endif