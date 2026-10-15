#pragma once
#ifdef __APPLE__

#include <lynx/headers/enumerations.h>
#include <lynx/core/bitflags.h>

#import <Metal/Metal.h>

namespace lynx::render::utils::metal {
    //Convertes required for textures
    MTLPixelFormat transformEngineToNativePixelFormat(enumerations::LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE format, MTLPixelFormat viewFormat);
    MTLTextureUsage transformEngineToNativeTextureUsage(LynxTextureUsageFlags usage);
    MTLStorageMode transformEngineToNativeStorageMode(enumerations::LYNX_GPU_MEMORY_PROPERTY_TYPE storageMemory);

    //Converters requeridos por los Command encoders y PSO
    MTLStoreAction transformEngineToNativeStoreOption(enumerations::LYNX_GPU_RENDER_PASS_STORE_OPTION_TYPE option);
    MTLLoadAction transformEngineToNativeLoadOption(enumerations::LYNX_GPU_RENDER_PASS_LOAD_OPTION_TYPE option);
    MTLCompareFunction transformEngineToNativeCompareFunction(enumerations::LYNX_GPU_COMPARE_FUNCTION function);

    //Converters para el sampler
    MTLSamplerMinMagFilter transformEngineToNativeSamplerMinMagFilter(enumerations::LYNX_GPU_SAMPLER_FILTER_TYPE filter);
    MTLSamplerAddressMode transformEngineToNativeSamplerAddressMode(enumerations::LYNX_GPU_SAMPLER_ADDRESS_MODE addressMode);
}

#endif
