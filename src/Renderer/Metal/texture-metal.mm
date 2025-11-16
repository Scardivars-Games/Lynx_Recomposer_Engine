#if defined (__APPLE__)
#include <texture-metal.h>

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

using namespace ilen::enumerations;

namespace ilen::render {
MetalTexture::MetalTexture(const IlenGPUTextureDescriptor& descriptor, MTKView* view, id<MTLDevice> device){
    @autoreleasepool {
        if(descriptor.presentation == ILEN_GPU_TEXTURE_PRESENTATION_TYPE::ILEN_GPU_TEXTURE_PRESENTATION_PRESENT){
            ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "Receibed the order to generate a Present Texture. By engine filosophy in Metal, before texture creation, you check if is a Present texture, and if it is true, you asign a CAMetalLayer from the view instead.");
        }
        
        MTLPixelFormat format = transformEngineToNativePixelFormat(descriptor.pixelFormat, view);
        bool mipMaps = descriptor.mipLevels > 1;
        
        MTLTextureDescriptor* texDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:format width:descriptor.width height:descriptor.height mipmapped:mipMaps];
        texDescriptor.usage = transformEngineToNativeTextureUsage(descriptor.usage);
        texDescriptor.storageMode = transformEngineToNativeStorageMode(descriptor.memoryProperty);
        
        _texture = [device newTextureWithDescriptor:texDescriptor];
        _texture.label = [NSString stringWithUTF8String:descriptor.label];
        //[_texture retain]; //al ser de un new, ya da una referencia, esto rompe el conteo.
        
        _texWidth = descriptor.width;
        _texHeight = descriptor.height;
    }
}

MetalTexture::~MetalTexture(){
    if(_texture != nullptr){
        [_texture release];
        _texture = nil;
    }
}

void* MetalTexture::getNativeTexture(){
    return (__bridge void*)_texture;
}

MTLPixelFormat MetalTexture::transformEngineToNativePixelFormat(ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE format, MTKView* view){
    switch (format) {
        case enumerations::ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_SCREEN:
            return view.colorPixelFormat;
        case enumerations::ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_DEPTH:
            return MTLPixelFormatDepth32Float;
        case enumerations::ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_DEPTH_STENCIL:
            return MTLPixelFormatDepth32Float_Stencil8;
        case enumerations::ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_FLOAT_HALF:
            return MTLPixelFormatR16Float;
        case enumerations::ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_FLOAT2_HALF:
            return MTLPixelFormatRG16Float;
        case enumerations::ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_FLOAT3_HALF:
            return MTLPixelFormatRGBA16Float;
        case enumerations::ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_FLOAT:
            return MTLPixelFormatR32Float;
        case enumerations::ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_FLOAT2:
            return MTLPixelFormatRG32Float;
        case enumerations::ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_FLOAT3:
            return MTLPixelFormatRGBA32Float;
        case enumerations::ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_INVALID:
            return MTLPixelFormatInvalid;
        default:
            return MTLPixelFormatInvalid;
    }
}

MTLTextureUsage MetalTexture::transformEngineToNativeTextureUsage(IlenTextureUsageFlags usage){
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

MTLStorageMode MetalTexture::transformEngineToNativeStorageMode(enumerations::ILEN_GPU_MEMORY_PROPERTY_TYPE storageMemory){
    switch (storageMemory) {
        case enumerations::ILEN_GPU_MEMORY_PROPERTY_TYPE::ILEN_GPU_MEMORY_PROPERTY_PRIVATE:
            return MTLStorageModePrivate;
        case enumerations::ILEN_GPU_MEMORY_PROPERTY_TYPE::ILEN_GPU_MEMORY_PROPERTY_SHARED:
            return MTLStorageModeShared;
    }
}
}

#endif
