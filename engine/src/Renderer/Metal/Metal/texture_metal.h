#pragma once

#if defined(__APPLE__)
#include <ilen/render/gpu_interfaces/texture.h>
#include <ilen/headers/enumerations.h>
#include <cstdint>
#include <ilen/headers/ilen_logs.h>
#include <Metal_Common/common_structures_metal.h>

#import<Metal/Metal.h>

@protocol MTLTexture;

namespace ilen::render {
    using namespace ilen::render::utils::metal;

    class MetalTexture final: public IGPUTexture {
        public:
            MetalTexture(const IlenGPUTextureDescriptor& descriptor,  MetalViewPredefinedValues predefinedVal, id<MTLDevice> device, const void* initialData = nullptr, id<MTLCommandQueue> cmdQueue = nullptr);
            MetalTexture(id<MTLTexture> givenNativeTexture); //Necesario para por ejemplo, usar la textura del CAMetalLayer sin tener que hacer el código del inicio del frame un infierno.
            ~MetalTexture() override;
            void* getNativeTexture() override;
        private:
            id<MTLTexture> _texture = nullptr;

            static inline const char* DEBUG_PREFIX = "METAL_TEXTURE";
    };
}

#endif
