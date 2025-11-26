#ifdef __APPLE__

#include <Metal-Common/engine-to-native-converters.h>

using namespace ilen::enumerations;

MTLPixelFormat ilen::render::utils::metal::transformEngineToNativePixelFormat(ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE format, MTKView* view){
    switch (format) {
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_SCREEN:
            return view.colorPixelFormat;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_DEPTH:
            return MTLPixelFormatDepth32Float;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_DEPTH_STENCIL:
            return MTLPixelFormatDepth32Float_Stencil8;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_FLOAT_HALF:
            return MTLPixelFormatR16Float;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_FLOAT2_HALF:
            return MTLPixelFormatRG16Float;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_FLOAT3_HALF:
            return MTLPixelFormatRGBA16Float;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_FLOAT:
            return MTLPixelFormatR32Float;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_FLOAT2:
            return MTLPixelFormatRG32Float;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_FLOAT3:
            return MTLPixelFormatRGBA32Float;
        case ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_INVALID:
            return MTLPixelFormatInvalid;
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

#endif
