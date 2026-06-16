#if ILEN_LINUX || ILEN_WINDOWS || ILEN_ANDROID
#include <Vulkan/version_adapters_vulkan.h>

using namespace ilen::render::vulkan;

vk::AttachmentDescription VulkanAttachmentData::getV1Descriptor(){
    vk::AttachmentDescription description;
    description.format = this->format;
    description.samples = this->samples;
    description.loadOp = this->loadOp;
    description.storeOp = this->storeOp;
    description.initialLayout = this->initialLayout;
    description.finalLayout = this->finalLayout;
    description.stencilLoadOp = this->stencilLoadOp;
    description.stencilStoreOp = this->stencilStoreOp;

    return description;
}

vk::AttachmentDescription2 VulkanAttachmentData::getV2Descriptor(){
    vk::AttachmentDescription2 description;
    description.format = this->format;
    description.samples = this->samples;
    description.loadOp = this->loadOp;
    description.storeOp = this->storeOp;
    description.initialLayout = this->initialLayout;
    description.finalLayout = this->finalLayout;
    description.stencilLoadOp = this->stencilLoadOp;
    description.stencilStoreOp = this->stencilStoreOp;

    return description;
}

vk::AttachmentReference VulkanAttachmentData::getV1Reference(){
    vk::AttachmentReference reference;
    reference.attachment = this->attachSlot;
    reference.layout = vk::ImageLayout::eColorAttachmentOptimal;

    return reference;
}

vk::AttachmentReference2 VulkanAttachmentData::getV2Reference(){
    vk::AttachmentReference2 reference;
    reference.attachment = this->attachSlot;
    reference.layout = vk::ImageLayout::eColorAttachmentOptimal;

    return reference;
}

#endif