#if defined (__APPLE__)
#include <Metal/texture-metal.h>
#include <Metal-Common/engine-to-native-converters.h>

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

using namespace ilen::enumerations;

namespace ilen::render {
MetalTexture::MetalTexture(const IlenGPUTextureDescriptor& descriptor, MTKView* view, id<MTLDevice> device){
    using namespace ilen::render::utils::metal;
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
        _texture.label = [NSString stringWithUTF8String:descriptor.label.c_str()];
        //[_texture retain]; //al ser de un new, ya da una referencia, esto rompe el conteo.
        
        _texWidth = descriptor.width;
        _texHeight = descriptor.height;
        _descriptor = descriptor;
    }
}

MetalTexture::MetalTexture(id<MTLTexture> givenNativeTexture){
    _texture = givenNativeTexture;
    [_texture retain]; //Si no la retengo, al terminar el Drawable, se perderan todas las referencias y se destruira la textura, lo cual hace que crashee el programa al llamar al destructor de esta implementación.
    
    _texWidth = static_cast<uint32_t>(givenNativeTexture.width);
    _texHeight = static_cast<uint32_t>(givenNativeTexture.height);
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
}

#endif

