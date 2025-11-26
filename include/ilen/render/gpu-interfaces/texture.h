#pragma once

#include <cstdint>
#include <ilen/headers/enumerations.h>
#include <ilen/core/bitflags.h>

using namespace ilen::enumerations;

namespace ilen::render {

    struct IlenGPUTextureDescriptor {
        uint32_t height = 0;
        uint32_t width = 0;

        uint32_t samplesCount = 1; //For multisampling

        //Obligatory required by Vulkan/DirectX12
        uint32_t mipLevels = 1;
        uint32_t arrayLayers = 1;

        ILEN_GPU_TEXTURE_PRESENTATION_TYPE presentation = ILEN_GPU_TEXTURE_PRESENTATION_TYPE::ILEN_GPU_TEXTURE_PRESENTATION_NOTPRESENT;
        ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE pixelFormat = ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_SCREEN;
        IlenTextureUsageFlags usage = ILEN_GPU_TEXTURE_USAGE_TYPE::ILEN_GPU_TEXTURE_USAGE_RENDERTARGET;
        ILEN_GPU_MEMORY_PROPERTY_TYPE memoryProperty = ILEN_GPU_MEMORY_PROPERTY_TYPE::ILEN_GPU_MEMORY_PROPERTY_PRIVATE;

        const char* label = nullptr;
    };

    class IGPUTexture {
        public:
            virtual ~IGPUTexture() = default;
            virtual void* getNativeTexture() = 0;
            uint32_t getTexWidth() const {return _texWidth;}
            uint32_t getTexHeight() const {return _texHeight; }
            IlenGPUTextureDescriptor getDescriptor() const { return _descriptor;}
        protected:
            uint32_t _texWidth = 0;
            uint32_t _texHeight = 0;
            IlenGPUTextureDescriptor _descriptor;
    };
}
