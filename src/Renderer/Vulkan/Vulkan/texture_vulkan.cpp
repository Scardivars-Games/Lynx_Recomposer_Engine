#if ILEN_LINUX || ILEN_WINDOWS || ILEN_ANDROID
#include <Vulkan/texture_vulkan.h>
using namespace ilen::render;

VulkanTexture::VulkanTexture(const vk::Image& image, vk::Format imageFormat, vk::raii::Device& logicDevice) {
    //Debemos crear una VkImageView para esta textura. Como esta función es usada directamente por el renderer, y no por alguna acción del usuario,
    //el VkImageView tendra varios valores por defecto.

    vk::ImageViewCreateInfo imageViewCreateInfo{};
    imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
    imageViewCreateInfo.format = imageFormat;
    //Dato para implementacion futura al generar texturas directamente: Puede aplicarse Swizzle directamente en el ImageView (createInfo.components.r = VK_COMPONENT_SWIZZLE_?)
    imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.image = image;

    _nativeImageView = vk::raii::ImageView(logicDevice, imageViewCreateInfo);
    _nativeImage = image;
}

void* VulkanTexture::getNativeTexture() {
    return *_nativeImageView;
}
#endif
