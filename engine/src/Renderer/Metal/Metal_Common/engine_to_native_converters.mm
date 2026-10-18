#ifdef __APPLE__

#include <Metal_Common/engine_to_native_converters.h>
#include <TargetConditionals.h>

using namespace lynx::enumerations;

MTLPixelFormat lynx::render::utils::metal::transformEngineToNativePixelFormat(LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE format, MTLPixelFormat viewFormat){
    switch (format) {
        
        // =========================================================================
        // ESTADO Y SISTEMA
        // =========================================================================
        case LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_INVALID:
            return MTLPixelFormatInvalid;
            
        case LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_SCREEN:
            return viewFormat;

        // =========================================================================
        // COLOR ESTÁNDAR (8-bit Unsigned Normalized)
        // =========================================================================
        case LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_R8_UNORM:
            return MTLPixelFormatR8Unorm;
            
        case LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_RG8_UNORM:
            return MTLPixelFormatRG8Unorm;
            
        case LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_RGBA8_UNORM:
            return MTLPixelFormatRGBA8Unorm;
            
        case LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_RGBA8_SRGB:
            return MTLPixelFormatRGBA8Unorm_sRGB;
            
        case LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_BGRA8_UNORM:
            return MTLPixelFormatBGRA8Unorm;
            
        case LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_BGRA8_SRGB:
            return MTLPixelFormatBGRA8Unorm_sRGB;

        // =========================================================================
        // HIGH DYNAMIC RANGE (Flotantes de 16-bit)
        // =========================================================================
        case LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_R16_FLOAT:
            return MTLPixelFormatR16Float;
            
        case LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_RG16_FLOAT:
            return MTLPixelFormatRG16Float;
            
        case LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_RGBA16_FLOAT:
            return MTLPixelFormatRGBA16Float;

        // =========================================================================
        // PRECISIÓN ALTA (Flotantes de 32-bit)
        // =========================================================================
        case LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_R32_FLOAT:
            return MTLPixelFormatR32Float;
            
        case LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_RG32_FLOAT:
            return MTLPixelFormatRG32Float;
            
        case LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_RGB32_FLOAT:
            // IMPORTANTE: Metal no tiene soporte nativo para RGB32Float (3 canales de 32 bits).
            // Por alineación de memoria en hardware, Apple te obliga a usar RGBA32Float.
            // Retornamos Invalid para que el motor obligue al Asset Pipeline a añadir un canal Alpha.
            return MTLPixelFormatInvalid; 
            
        case LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_RGBA32_FLOAT:
            return MTLPixelFormatRGBA32Float;

        // =========================================================================
        // PROFUNDIDAD Y STENCIL (Depth & Stencil)
        // =========================================================================
        case LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_DEPTH_16_UNORM:
            return MTLPixelFormatDepth16Unorm;
            
        case LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_DEPTH_32_FLOAT:
            return MTLPixelFormatDepth32Float;
            
        case LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_DEPTH_24_STENCIL_8:
#if LYNX_MACOS
        if(@available(macOS 27.0,*)) {
            return MTLPixelFormatDepth32Float_Stencil8;
        }else{
            //Reduciendo esa opcion para macOS 26 o inferior, por ser deprecada a partir del 27
            #pragma clang diagnostic push
            #pragma clang diagnostic ignored "-Wdeprecated-declarations"

            return MTLPixelFormatDepth24Unorm_Stencil8; // Solo existe en hardware Mac antiguo
            #pragma clang diagnostic pop
        }
#else
            return MTLPixelFormatDepth32Float_Stencil8; // Fallback automático para iOS/Apple Silicon
#endif
            
        case LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_DEPTH_32_FLOAT_STENCIL_8:
            return MTLPixelFormatDepth32Float_Stencil8;

        // =========================================================================
        // COMPRESIÓN DE BLOQUES (Assets de Juego)
        // =========================================================================
        case LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_BC7_UNORM:
#if LYNX_MACOS || TARGET_OS_SIMULATOR
            return MTLPixelFormatBC7_RGBAUnorm;
#else
            return MTLPixelFormatInvalid; // iOS nunca ha soportado BC7
#endif

        case LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_BC7_SRGB:
#if LYNX_MACOS || TARGET_OS_SIMULATOR
            return MTLPixelFormatBC7_RGBAUnorm_sRGB;
#else
            return MTLPixelFormatInvalid;
#endif

        case LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_ASTC_4X4_UNORM:
            // En la nomenclatura de Metal, "LDR" (Low Dynamic Range) es el equivalente a UNORM para ASTC.
            return MTLPixelFormatASTC_4x4_LDR;
            
        case LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_ASTC_4X4_SRGB:
            return MTLPixelFormatASTC_4x4_sRGB;

        // =========================================================================
        // FALLBACK
        // =========================================================================
        default:
            return MTLPixelFormatInvalid;
    }
}

MTLTextureUsage lynx::render::utils::metal::transformEngineToNativeTextureUsage(LynxTextureUsageFlags usage){
    MTLTextureUsage returnUsage = 0;

    //BitFlags operations:
    if(usage & LYNX_GPU_TEXTURE_USAGE_RENDERTARGET)
        returnUsage |= MTLTextureUsageRenderTarget;
    if(usage & LYNX_GPU_TEXTURE_USAGE_SHADER_READ)
        returnUsage |= MTLTextureUsageShaderRead;
    if(usage & LYNX_GPU_TEXTURE_USAGE_SHADER_WRITE)
        returnUsage |= MTLTextureUsageShaderWrite;

    return returnUsage;
}

MTLStorageMode lynx::render::utils::metal::transformEngineToNativeStorageMode(LYNX_GPU_MEMORY_PROPERTY_TYPE storageMemory){
    switch (storageMemory) {
        case LYNX_GPU_MEMORY_PROPERTY_TYPE::LYNX_GPU_MEMORY_PROPERTY_PRIVATE:
            return MTLStorageModePrivate;
        case LYNX_GPU_MEMORY_PROPERTY_TYPE::LYNX_GPU_MEMORY_PROPERTY_SHARED:
            return MTLStorageModeShared;
    }
}

MTLStoreAction lynx::render::utils::metal::transformEngineToNativeStoreOption(LYNX_GPU_RENDER_PASS_STORE_OPTION_TYPE option){
        switch (option) {
            case LYNX_GPU_RENDER_PASS_STORE_OPTION_TYPE::LYNX_GPU_RENDER_PASS_STORE_OPTION_STORE:
                return MTLStoreActionStore;
            case LYNX_GPU_RENDER_PASS_STORE_OPTION_TYPE::LYNX_GPU_RENDER_PASS_STORE_OPTION_DISCARD:
            case LYNX_GPU_RENDER_PASS_STORE_OPTION_TYPE::LYNX_GPU_RENDER_PASS_STORE_OPTION_DONT_CARE:
                return MTLStoreActionDontCare;
        }
    }

MTLLoadAction lynx::render::utils::metal::transformEngineToNativeLoadOption(LYNX_GPU_RENDER_PASS_LOAD_OPTION_TYPE option){
    switch (option) {
        case LYNX_GPU_RENDER_PASS_LOAD_OPTION_TYPE::LYNX_GPU_RENDER_PASS_LOAD_OPTION_LOAD:
            return MTLLoadActionLoad;
        case LYNX_GPU_RENDER_PASS_LOAD_OPTION_TYPE::LYNX_GPU_RENDER_PASS_LOAD_OPTION_CLEAR:
            return MTLLoadActionClear;
        case LYNX_GPU_RENDER_PASS_LOAD_OPTION_TYPE::LYNX_GPU_RENDER_PASS_LOAD_OPTION_DONT_CARE:
            return MTLLoadActionDontCare;
    }
}

MTLCompareFunction lynx::render::utils::metal::transformEngineToNativeCompareFunction(enumerations::LYNX_GPU_COMPARE_FUNCTION function){
    switch (function) {
        case enumerations::LYNX_GPU_COMPARE_FUNCTION::LYNX_GPU_COMPARE_NEVER:
            return MTLCompareFunctionNever;
        case enumerations::LYNX_GPU_COMPARE_FUNCTION::LYNX_GPU_COMPARE_LESS:
            return MTLCompareFunctionLess;
        case enumerations::LYNX_GPU_COMPARE_FUNCTION::LYNX_GPU_COMPARE_LESS_EQUAL:
            return MTLCompareFunctionLessEqual;
        case enumerations::LYNX_GPU_COMPARE_FUNCTION::LYNX_GPU_COMPARE_EQUAL:
            return MTLCompareFunctionEqual;
        case enumerations::LYNX_GPU_COMPARE_FUNCTION::LYNX_GPU_COMPARE_GREATER:
            return MTLCompareFunctionGreater;
        case enumerations::LYNX_GPU_COMPARE_FUNCTION::LYNX_GPU_COMPARE_GREATER_EQUAL:
            return MTLCompareFunctionGreaterEqual;
        case enumerations::LYNX_GPU_COMPARE_FUNCTION::LYNX_GPU_COMPARE_ALWAYS:
            return MTLCompareFunctionAlways;
        default:
            return MTLCompareFunctionAlways;
    }
}

MTLSamplerMinMagFilter lynx::render::utils::metal::transformEngineToNativeSamplerMinMagFilter(enumerations::LYNX_GPU_SAMPLER_FILTER_TYPE filter){
    switch (filter) {
        case LYNX_GPU_SAMPLER_FILTER_TYPE::LYNX_GPU_SAMPLER_FILTER_LINEAR:
            return MTLSamplerMinMagFilterLinear;
        case LYNX_GPU_SAMPLER_FILTER_TYPE::LYNX_GPU_SAMPLER_FILTER_NEAREST:
            return MTLSamplerMinMagFilterNearest;
    }
}


MTLSamplerAddressMode lynx::render::utils::metal::transformEngineToNativeSamplerAddressMode(enumerations::LYNX_GPU_SAMPLER_ADDRESS_MODE addressMode){
    switch (addressMode) {
        case LYNX_GPU_SAMPLER_ADDRESS_MODE::LYNX_GPU_SAMPLER_ADDRESS_REPEAT:
            return MTLSamplerAddressModeRepeat;
        case LYNX_GPU_SAMPLER_ADDRESS_MODE::LYNX_GPU_SAMPLER_ADDRESS_MIRRORED_REPEAT:
            return MTLSamplerAddressModeMirrorRepeat;
        case LYNX_GPU_SAMPLER_ADDRESS_MODE::LYNX_GPU_SAMPLER_ADDRESS_CLAMP_TO_EDGE:
            return MTLSamplerAddressModeClampToEdge;
        case LYNX_GPU_SAMPLER_ADDRESS_MODE::LYNX_GPU_SAMPLER_ADDRESS_CLAMP_TO_BORDER_COLOR:
            if (@available(iOS 14.0, *)) {
                return MTLSamplerAddressModeClampToBorderColor;
            } else {
                // Fallback on earlier versions
                return MTLSamplerAddressModeClampToEdge;
            }
    }
}

#endif
