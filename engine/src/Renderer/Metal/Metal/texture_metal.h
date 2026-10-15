#pragma once

#if defined(__APPLE__)
#include <lynx/render/gpu_interfaces/texture.h>
#include <lynx/headers/enumerations.h>
#include <cstdint>
#include <lynx/headers/lynx_logs.h>
#include <Metal_Common/common_structures_metal.h>

#import<Metal/Metal.h>

@protocol MTLTexture;

namespace lynx::render {
    using namespace lynx::render::utils::metal;

    class MetalTexture final: public IGPUTexture {
        public:
            MetalTexture(const LynxGPUTextureDescriptor& descriptor,  MetalViewPredefinedValues predefinedVal, id<MTLDevice> device, const void* initialData = nullptr, id<MTLCommandQueue> cmdQueue = nullptr);
            MetalTexture(id<MTLTexture> givenNativeTexture); //Necesario para por ejemplo, usar la textura del CAMetalLayer sin tener que hacer el código del inicio del frame un infierno.
            ~MetalTexture() override;
            void* getNativeTexture() override;
        private:
            id<MTLTexture> _texture = nullptr;

            static inline const char* DEBUG_PREFIX = "METAL_TEXTURE";
    };
}

#endif
