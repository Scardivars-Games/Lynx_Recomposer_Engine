#pragma once

#if defined(__APPLE__)
#include <ilen/render/gpu-interfaces/texture.h>
#include <ilen/headers/enumerations.h>
#include <cstdint>
#include <ilen/headers/ilen-logs.h>

#import<Metal/Metal.h>

@protocol MTLTexture;
@class MTKView;

namespace ilen::render {
    class MetalTexture final: public IGPUTexture {
        public:
            MetalTexture(const IlenGPUTextureDescriptor& descriptor, MTKView* view, id<MTLDevice> device);
            MetalTexture(id<MTLTexture> givenNativeTexture); //Necesario para por ejemplo, usar la textura del CAMetalLayer sin tener que hacer el código del inicio del frame un infierno.
            ~MetalTexture() override;
            void* getNativeTexture() override;
        private:
            MTLPixelFormat transformEngineToNativePixelFormat(enumerations::ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE format, MTKView* view);
            MTLTextureUsage transformEngineToNativeTextureUsage(IlenTextureUsageFlags usage);
            MTLStorageMode transformEngineToNativeStorageMode(enumerations::ILEN_GPU_MEMORY_PROPERTY_TYPE storageMemory);
            id<MTLTexture> _texture = nullptr;

            static inline const char* DEBUG_PREFIX = "METAL_TEXTURE";
    };
}

#endif
