#ifdef __APPLE__

#include <Metal_Common/engine_to_native_converters.h>

using namespace ilen::enumerations;

MTLPixelFormat ilen::render::utils::metal::transformEngineToNativePixelFormat(ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE format, MTLPixelFormat viewFormat){
    switch (format) {
        
        // =========================================================================
        // ESTADO Y SISTEMA
        // =========================================================================
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_INVALID:
            return MTLPixelFormatInvalid;
            
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_SCREEN:
            return viewFormat;

        // =========================================================================
        // COLOR ESTÁNDAR (8-bit Unsigned Normalized)
        // =========================================================================
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_R8_UNORM:
            return MTLPixelFormatR8Unorm;
            
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_RG8_UNORM:
            return MTLPixelFormatRG8Unorm;
            
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_RGBA8_UNORM:
            return MTLPixelFormatRGBA8Unorm;
            
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_RGBA8_SRGB:
            return MTLPixelFormatRGBA8Unorm_sRGB;
            
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_BGRA8_UNORM:
            return MTLPixelFormatBGRA8Unorm;
            
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_BGRA8_SRGB:
            return MTLPixelFormatBGRA8Unorm_sRGB;

        // =========================================================================
        // HIGH DYNAMIC RANGE (Flotantes de 16-bit)
        // =========================================================================
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_R16_FLOAT:
            return MTLPixelFormatR16Float;
            
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_RG16_FLOAT:
            return MTLPixelFormatRG16Float;
            
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_RGBA16_FLOAT:
            return MTLPixelFormatRGBA16Float;

        // =========================================================================
        // PRECISIÓN ALTA (Flotantes de 32-bit)
        // =========================================================================
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_R32_FLOAT:
            return MTLPixelFormatR32Float;
            
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_RG32_FLOAT:
            return MTLPixelFormatRG32Float;
            
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_RGB32_FLOAT:
            // IMPORTANTE: Metal no tiene soporte nativo para RGB32Float (3 canales de 32 bits).
            // Por alineación de memoria en hardware, Apple te obliga a usar RGBA32Float.
            // Retornamos Invalid para que el motor obligue al Asset Pipeline a añadir un canal Alpha.
            return MTLPixelFormatInvalid; 
            
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_RGBA32_FLOAT:
            return MTLPixelFormatRGBA32Float;

        // =========================================================================
        // PROFUNDIDAD Y STENCIL (Depth & Stencil)
        // =========================================================================
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_DEPTH_16_UNORM:
            return MTLPixelFormatDepth16Unorm;
            
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_DEPTH_32_FLOAT:
            return MTLPixelFormatDepth32Float;
            
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_DEPTH_24_STENCIL_8:
#if ILEN_MACOS
            return MTLPixelFormatDepth24Unorm_Stencil8; // Solo existe en hardware Mac antiguo
#else
            return MTLPixelFormatDepth32Float_Stencil8; // Fallback automático para iOS/Apple Silicon
#endif
            
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_DEPTH_32_FLOAT_STENCIL_8:
            return MTLPixelFormatDepth32Float_Stencil8;

        // =========================================================================
        // COMPRESIÓN DE BLOQUES (Assets de Juego)
        // =========================================================================
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_BC7_UNORM:
#if ILEN_MACOS
            return MTLPixelFormatBC7_RGBAUnorm;
#else
            return MTLPixelFormatInvalid; // iOS nunca ha soportado BC7
#endif

        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_BC7_SRGB:
#if ILEN_MACOS
            return MTLPixelFormatBC7_RGBAUnorm_sRGB;
#else
            return MTLPixelFormatInvalid;
#endif

        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_ASTC_4X4_UNORM:
            // En la nomenclatura de Metal, "LDR" (Low Dynamic Range) es el equivalente a UNORM para ASTC.
            return MTLPixelFormatASTC_4x4_LDR;
            
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_ASTC_4X4_SRGB:
            return MTLPixelFormatASTC_4x4_sRGB;

        // =========================================================================
        // FALLBACK
        // =========================================================================
        default:
            return MTLPixelFormatInvalid;
    }
}

MTLTextureUsage ilen::render::utils::metal::transformEngineToNativeTextureUsage(IlenTextureUsageFlags usage){
    MTLTextureUsage returnUsage = 0;

    //BitFlags operations:
    if(usage & ILEN_GPU_TEXTURE_USAGE_RENDERTARGET)
        returnUsage |= MTLTextureUsageRenderTarget;
    if(usage & ILEN_GPU_TEXTURE_USAGE_SHADER_READ)
        returnUsage |= MTLTextureUsageShaderRead;
    if(usage & ILEN_GPU_TEXTURE_USAGE_SHADER_WRITE)
        returnUsage |= MTLTextureUsageShaderWrite;

    return returnUsage;
}

MTLStorageMode ilen::render::utils::metal::transformEngineToNativeStorageMode(ILEN_GPU_MEMORY_PROPERTY_TYPE storageMemory){
    switch (storageMemory) {
        case ILEN_GPU_MEMORY_PROPERTY_TYPE::ILEN_GPU_MEMORY_PROPERTY_PRIVATE:
            return MTLStorageModePrivate;
        case ILEN_GPU_MEMORY_PROPERTY_TYPE::ILEN_GPU_MEMORY_PROPERTY_SHARED:
            return MTLStorageModeShared;
    }
}

MTLStoreAction ilen::render::utils::metal::transformEngineToNativeStoreOption(ILEN_GPU_RENDER_PASS_STORE_OPTION_TYPE option){
        switch (option) {
            case ILEN_GPU_RENDER_PASS_STORE_OPTION_TYPE::ILEN_GPU_RENDER_PASS_STORE_OPTION_STORE:
                return MTLStoreActionStore;
            case ILEN_GPU_RENDER_PASS_STORE_OPTION_TYPE::ILEN_GPU_RENDER_PASS_STORE_OPTION_DISCARD:
            case ILEN_GPU_RENDER_PASS_STORE_OPTION_TYPE::ILEN_GPU_RENDER_PASS_STORE_OPTION_DONT_CARE:
                return MTLStoreActionDontCare;
        }
    }

MTLLoadAction ilen::render::utils::metal::transformEngineToNativeLoadOption(ILEN_GPU_RENDER_PASS_LOAD_OPTION_TYPE option){
    switch (option) {
        case ILEN_GPU_RENDER_PASS_LOAD_OPTION_TYPE::ILEN_GPU_RENDER_PASS_LOAD_OPTION_LOAD:
            return MTLLoadActionLoad;
        case ILEN_GPU_RENDER_PASS_LOAD_OPTION_TYPE::ILEN_GPU_RENDER_PASS_LOAD_OPTION_CLEAR:
            return MTLLoadActionClear;
        case ILEN_GPU_RENDER_PASS_LOAD_OPTION_TYPE::ILEN_GPU_RENDER_PASS_LOAD_OPTION_DONT_CARE:
            return MTLLoadActionDontCare;
    }
}

MTLCompareFunction ilen::render::utils::metal::transformEngineToNativeCompareFunction(enumerations::ILEN_GPU_COMPARE_FUNCTION function){
    switch (function) {
        case enumerations::ILEN_GPU_COMPARE_FUNCTION::ILEN_GPU_COMPARE_NEVER:
            return MTLCompareFunctionNever;
        case enumerations::ILEN_GPU_COMPARE_FUNCTION::ILEN_GPU_COMPARE_LESS:
            return MTLCompareFunctionLess;
        case enumerations::ILEN_GPU_COMPARE_FUNCTION::ILEN_GPU_COMPARE_LESS_EQUAL:
            return MTLCompareFunctionLessEqual;
        case enumerations::ILEN_GPU_COMPARE_FUNCTION::ILEN_GPU_COMPARE_EQUAL:
            return MTLCompareFunctionEqual;
        case enumerations::ILEN_GPU_COMPARE_FUNCTION::ILEN_GPU_COMPARE_GREATER:
            return MTLCompareFunctionGreater;
        case enumerations::ILEN_GPU_COMPARE_FUNCTION::ILEN_GPU_COMPARE_GREATER_EQUAL:
            return MTLCompareFunctionGreaterEqual;
        case enumerations::ILEN_GPU_COMPARE_FUNCTION::ILEN_GPU_COMPARE_ALWAYS:
            return MTLCompareFunctionAlways;
        default:
            return MTLCompareFunctionAlways;
    }
}

MTLSamplerMinMagFilter ilen::render::utils::metal::transformEngineToNativeSamplerMinMagFilter(enumerations::ILEN_GPU_SAMPLER_FILTER_TYPE filter){
    switch (filter) {
        case ILEN_GPU_SAMPLER_FILTER_TYPE::ILEN_GPU_SAMPLER_FILTER_LINEAR:
            return MTLSamplerMinMagFilterLinear;
        case ILEN_GPU_SAMPLER_FILTER_TYPE::ILEN_GPU_SAMPLER_FILTER_NEAREST:
            return MTLSamplerMinMagFilterNearest;
    }
}


MTLSamplerAddressMode ilen::render::utils::metal::transformEngineToNativeSamplerAddressMode(enumerations::ILEN_GPU_SAMPLER_ADDRESS_MODE addressMode){
    switch (addressMode) {
        case ILEN_GPU_SAMPLER_ADDRESS_MODE::ILEN_GPU_SAMPLER_ADDRESS_REPEAT:
            return MTLSamplerAddressModeRepeat;
        case ILEN_GPU_SAMPLER_ADDRESS_MODE::ILEN_GPU_SAMPLER_ADDRESS_MIRRORED_REPEAT:
            return MTLSamplerAddressModeMirrorRepeat;
        case ILEN_GPU_SAMPLER_ADDRESS_MODE::ILEN_GPU_SAMPLER_ADDRESS_CLAMP_TO_EDGE:
            return MTLSamplerAddressModeClampToEdge;
        case ILEN_GPU_SAMPLER_ADDRESS_MODE::ILEN_GPU_SAMPLER_ADDRESS_CLAMP_TO_BORDER_COLOR:
            if (@available(iOS 14.0, *)) {
                return MTLSamplerAddressModeClampToBorderColor;
            } else {
                // Fallback on earlier versions
                return MTLSamplerAddressModeClampToEdge;
            }
    }
}

#endif
