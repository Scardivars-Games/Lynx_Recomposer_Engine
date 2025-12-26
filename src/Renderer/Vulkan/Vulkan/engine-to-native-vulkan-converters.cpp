#if ILEN_LINUX || ILEN_WINDOWS || ILEN_ANDROID
#include <Vulkan/engine-to-native-vulkan-converters.h>

using namespace ilen::render::utils::vulkan;
using namespace ilen::enumerations;

vk::Format transformEngineToNativePixelFormat(ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE format,
                                                                vk::Format presentFormat) {
    switch (format) {
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_SCREEN:
            return presentFormat;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_DEPTH:
            return vk::Format::eD32Sfloat;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_DEPTH_STENCIL:
            return vk::Format::eD32SfloatS8Uint;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_FLOAT_HALF:
            return vk::Format::eR16Sfloat;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_FLOAT2_HALF:
            return vk::Format::eR16G16Sfloat;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_FLOAT3_HALF:
            return vk::Format::eR16G16B16Sfloat;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_FLOAT:
            return vk::Format::eR32Sfloat;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_FLOAT2:
            return vk::Format::eR32G32Sfloat;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_FLOAT3:
            return vk::Format::eR32G32B32Sfloat;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_INVALID:
            return vk::Format::eUndefined;
        default:
            return vk::Format::eUndefined;
    }
}

vk::ImageUsageFlags transformEngineToNativeImageUsage(IlenTextureUsageFlags usage){
    vk::ImageUsageFlags returnUsage;

    //BitFlags operations:
    if(usage & ILEN_GPU_TEXTURE_USAGE_RENDERTARGET)
        returnUsage |= vk::ImageUsageFlagBits::eColorAttachment;
    if(usage & ILEN_GPU_TEXTURE_USAGE_SHADER_READ)
        returnUsage |= vk::ImageUsageFlagBits::eSampled;
    if(usage & ILEN_GPU_TEXTURE_USAGE_SHADER_WRITE)
        returnUsage |= vk::ImageUsageFlagBits::eStorage;
    if (usage & ILEN_GPU_TEXTURE_USAGE_DEPTHSTENCIL)
        returnUsage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;

    return returnUsage;
}

#endif
