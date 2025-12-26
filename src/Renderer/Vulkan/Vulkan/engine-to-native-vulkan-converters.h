#pragma once
#if ILEN_LINUX || ILEN_WINDOWS || ILEN_ANDROID
#include <ilen/headers/enumerations.h>
#include <ilen/core/bitflags.h>
#include <vulkan/vulkan.hpp>

namespace ilen::render::utils::vulkan {
    //Conversores
    vk::Format transformEngineToNativePixelFormat(enumerations::ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE format, vk::Format presentFormat);
    vk::ImageUsageFlags transformEngineToNativeImageUsage(IlenTextureUsageFlags usage);
}

#endif
