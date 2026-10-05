#if ILEN_LINUX || ILEN_WINDOWS || ILEN_ANDROID
#include <Vulkan/engine_to_native_vulkan_converters.h>

using namespace ilen::enumerations;

vk::Format ilen::render::vulkan::utils::transformEngineToNativePixelFormat(ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE format,
                                                                         vk::Format presentFormat) {
    switch (format) {
        // ESTADO Y SISTEMA
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_SCREEN:
            return presentFormat;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_INVALID:
            return vk::Format::eUndefined;

        // COLOR ESTÁNDAR (8-bit Unsigned Normalized)
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_R8_UNORM:
            return vk::Format::eR8Unorm;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_RG8_UNORM:
            return vk::Format::eR8G8Unorm;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_RGBA8_UNORM:
            return vk::Format::eR8G8B8A8Unorm;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_RGBA8_SRGB:
            return vk::Format::eR8G8B8A8Srgb;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_BGRA8_UNORM:
            return vk::Format::eB8G8R8A8Unorm;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_BGRA8_SRGB:
            return vk::Format::eB8G8R8A8Srgb;

        // HIGH DYNAMIC RANGE (16-bit Float / "Half")
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_R16_FLOAT:
            return vk::Format::eR16Sfloat;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_RG16_FLOAT:
            return vk::Format::eR16G16Sfloat;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_RGBA16_FLOAT:
            return vk::Format::eR16G16B16A16Sfloat;

        // PRECISIÓN ALTA (32-bit Float)
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_R32_FLOAT:
            return vk::Format::eR32Sfloat;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_RG32_FLOAT:
            return vk::Format::eR32G32Sfloat;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_RGB32_FLOAT:
            return vk::Format::eR32G32B32Sfloat;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_RGBA32_FLOAT:
            return vk::Format::eR32G32B32A32Sfloat;

        // PROFUNDIDAD Y STENCIL
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_DEPTH_16_UNORM:
            return vk::Format::eD16Unorm;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_DEPTH_32_FLOAT:
            return vk::Format::eD32Sfloat;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_DEPTH_24_STENCIL_8:
            return vk::Format::eD24UnormS8Uint; // Nota: Asegúrate de que el hardware soporte D24 (común en desktop)
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_DEPTH_32_FLOAT_STENCIL_8:
            return vk::Format::eD32SfloatS8Uint;

        // COMPRESIÓN DE BLOQUES (BC7 y ASTC)
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_BC7_UNORM:
            return vk::Format::eBc7UnormBlock;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_BC7_SRGB:
            return vk::Format::eBc7SrgbBlock;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_ASTC_4X4_UNORM:
            return vk::Format::eAstc4x4UnormBlock;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_ASTC_4X4_SRGB:
            return vk::Format::eAstc4x4SrgbBlock;

        default:
            return vk::Format::eUndefined;
    }
}

vk::ImageUsageFlags ilen::render::vulkan::utils::transformEngineToNativeImageUsage(IlenTextureUsageFlags usage){
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

vk::BufferUsageFlags ilen::render::vulkan::utils::transformEngineToNativeBufferUsage(enumerations::ILEN_RENDER_GPU_BUFFER_TYPE bufferType){
    switch (bufferType)
    {
        case ILEN_RENDER_GPU_BUFFER_INDEX:
            return vk::BufferUsageFlagBits::eIndexBuffer;
        case ILEN_RENDER_GPU_BUFFER_VERTEX:
            return vk::BufferUsageFlagBits::eVertexBuffer;
        case ILEN_RENDER_GPU_BUFFER_UNIFORM:
            return vk::BufferUsageFlagBits::eUniformBuffer;
        default:
            return vk::BufferUsageFlagBits::eUniformBuffer;
    }
}


#endif
