#if defined (__APPLE__)
#include <Metal/texture-metal.h>
#include <Metal-Common/engine-to-native-converters.h>
#include <Metal-Common/common-Structures-Metal.h>

#import <Metal/Metal.h>

using namespace ilen::enumerations;

namespace ilen::render {
MetalTexture::MetalTexture(const IlenGPUTextureDescriptor& descriptor, MetalViewPredefinedValues predefinedVal, id<MTLDevice> device, const void* initialData, id<MTLCommandQueue> cmdQueue){
    using namespace ilen::render::utils::metal;
    @autoreleasepool {
        if(descriptor.presentation == ILEN_GPU_TEXTURE_PRESENTATION_TYPE::ILEN_GPU_TEXTURE_PRESENTATION_PRESENT){
            ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "Receibed the order to generate a Present Texture. By engine filosophy in Metal, before texture creation, you check if is a Present texture, and if it is true, you asign a CAMetalLayer from the view instead.");
        }
        
        MTLPixelFormat format = transformEngineToNativePixelFormat(descriptor.pixelFormat, predefinedVal.screenPixelFormat);
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
        
        //Esto es si resulta que hay datos que subir de la textura.
        if(initialData != nullptr){
            MTLRegion region = MTLRegionMake2D(0, 0, descriptor.width, descriptor.height);
            NSUInteger bytesPerRow = 0;
            NSUInteger totalBytes = 0;
            
            //Calcular el tamaño de los bytes por fila (pitch)
            bool isCompressedBlock4x4 = (
                    descriptor.pixelFormat == enumerations::ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_ASTC_4X4_UNORM ||
                    descriptor.pixelFormat == enumerations::ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_ASTC_4X4_SRGB ||
                     descriptor.pixelFormat == enumerations::ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_BC7_SRGB ||
                     descriptor.pixelFormat == enumerations::ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_BC7_UNORM
                                         );
            //En texturas comprimidas, cada bloque es de 16 bytes
            if(isCompressedBlock4x4) {
                NSUInteger blocksPerRow = (descriptor.width + 3) / 4;
                NSUInteger blocksPerCol = (descriptor.height + 3) / 4;
                bytesPerRow = blocksPerRow * 16;
                totalBytes = bytesPerRow * blocksPerCol;
            }else{
                NSUInteger bytesPerPixel = 4;
                if (descriptor.pixelFormat == ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_R8_UNORM) {
                    bytesPerPixel = 1;
                } else if (descriptor.pixelFormat == ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_RG8_UNORM ||
                       descriptor.pixelFormat == ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_R16_FLOAT) {
                    bytesPerPixel = 2;
                } else if (descriptor.pixelFormat == ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_RGBA16_FLOAT ||
                           descriptor.pixelFormat == ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_RG32_FLOAT) {
                    bytesPerPixel = 8;
                } else if (descriptor.pixelFormat == ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_RGBA32_FLOAT) {
                    bytesPerPixel = 16;
                }

                bytesPerRow = descriptor.width * bytesPerPixel;
                totalBytes = bytesPerRow * descriptor.height;
            }
            
            //Creamos un stagging buffer para copiar los datos a la GPU
            id<MTLBuffer> stagingBuffer = [device newBufferWithBytes:initialData length:totalBytes options:MTLResourceStorageModeShared];
            
            id<MTLCommandBuffer> commandBuffer = [cmdQueue commandBuffer];
            id<MTLBlitCommandEncoder> blitEncoder = [commandBuffer  blitCommandEncoder];
            
            MTLSize sourceSize = MTLSizeMake(descriptor.width, descriptor.height, 1);
            
            [blitEncoder copyFromBuffer:stagingBuffer
                           sourceOffset:0 sourceBytesPerRow:bytesPerRow
                    sourceBytesPerImage:totalBytes sourceSize:sourceSize
                              toTexture:_texture destinationSlice:0
                       destinationLevel:0 destinationOrigin:MTLOriginMake(0, 0, 0)];
            
            [blitEncoder endEncoding];
            
            [commandBuffer commit];
            [commandBuffer waitUntilCompleted];
            
            [stagingBuffer release];
                       
            
            //[_texture replaceRegion:region mipmapLevel:0 withBytes:initialData bytesPerRow:bytesPerRow];
        }
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

