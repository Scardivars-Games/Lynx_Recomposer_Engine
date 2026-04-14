#pragma once
#ifdef __APPLE__

#include <ilen/headers/enumerations.h>
#include <ilen/core/bitflags.h>

#import <Metal/Metal.h>

namespace ilen::render::utils::metal {
    //Convertes required for textures
    MTLPixelFormat transformEngineToNativePixelFormat(enumerations::ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE format, MTLPixelFormat viewFormat);
    MTLTextureUsage transformEngineToNativeTextureUsage(IlenTextureUsageFlags usage);
    MTLStorageMode transformEngineToNativeStorageMode(enumerations::ILEN_GPU_MEMORY_PROPERTY_TYPE storageMemory);

    //Converters requeridos por los Command encoders y PSO
    MTLStoreAction transformEngineToNativeStoreOption(enumerations::ILEN_GPU_RENDER_PASS_STORE_OPTION_TYPE option);
    MTLLoadAction transformEngineToNativeLoadOption(enumerations::ILEN_GPU_RENDER_PASS_LOAD_OPTION_TYPE option);
}

#endif
