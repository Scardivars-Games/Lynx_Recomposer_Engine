#pragma once
#if defined(__APPLE__)

#include <ilen/render/gpu_interfaces/sampler.h>
#include <vector>
#include <mutex>
#include <memory>

#import <Metal/Metal.h>

namespace ilen::render {
    class MetalSampler final : public IGPUSampler {
        public:
            static std::shared_ptr<IGPUSampler> getOrCreate(const IlenGPUSamplerDescriptor& descriptor, id<MTLDevice> device);
            ~MetalSampler() override;
            void* getNativeSampler() override;
        private:
            //De esta forma, solo getOrCreate puede instanciar la clase
            MetalSampler(id<MTLSamplerState> nativeSampler, const IlenGPUSamplerDescriptor& descriptor);
            id<MTLSamplerState> _sampler = nil;

            static std::vector<std::shared_ptr<IGPUSampler>> _cachedSamplers;
            static std::mutex _samplerMutex;
        
            static inline const char* DEBUG_PREFIX = "METAL_SAMPLER";
    };
}

#endif