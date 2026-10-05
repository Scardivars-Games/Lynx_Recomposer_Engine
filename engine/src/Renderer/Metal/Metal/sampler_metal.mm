#include <Metal/sampler_metal.h>
#include <ilen/headers/ilen_logs.h>
#include <Metal_Common/engine_to_native_converters.h>

using namespace ilen::render::utils::metal;

namespace ilen::render {
    std::vector<std::shared_ptr<IGPUSampler>> MetalSampler::_cachedSamplers;
    std::mutex MetalSampler::_samplerMutex;
    
    std::shared_ptr<IGPUSampler> MetalSampler::getOrCreate(const IlenGPUSamplerDescriptor &descriptor, id<MTLDevice> device){
        std::lock_guard lock(_samplerMutex);
        
        //Buscando si esta cacheado el sampelr
        for(const auto& cachedSampler: _cachedSamplers){
            if(cachedSampler->getDescriptor() == descriptor){
                return cachedSampler;
            }
        }
        
        //Crear el Sampler si no existe
        @autoreleasepool {
            MTLSamplerDescriptor* mtlDescriptor = [MTLSamplerDescriptor new];
            
            mtlDescriptor.minFilter = transformEngineToNativeSamplerMinMagFilter(descriptor.minFilter);
            mtlDescriptor.magFilter = transformEngineToNativeSamplerMinMagFilter(descriptor.magFilter);
            
            mtlDescriptor.sAddressMode = transformEngineToNativeSamplerAddressMode(descriptor.addressModeU);
            mtlDescriptor.tAddressMode = transformEngineToNativeSamplerAddressMode(descriptor.addressModeV);
            mtlDescriptor.rAddressMode = transformEngineToNativeSamplerAddressMode(descriptor.addressModeW);
            
            if(!descriptor.label.empty())
                mtlDescriptor.label = [NSString stringWithUTF8String:descriptor.label.c_str()];
            
            id<MTLSamplerState> nativeSampler = [device newSamplerStateWithDescriptor:mtlDescriptor];
            [mtlDescriptor release];
            
            if(!nativeSampler) {
                ILEN_ERROR_P(DEBUG_PREFIX, "Error interno en Metal al tratar de crear el MTLSamplerState");
                return nullptr;
            }
            
            std::shared_ptr<MetalSampler> newSampler(new MetalSampler(nativeSampler, descriptor));
            
            _cachedSamplers.push_back(newSampler);
            return newSampler;
        }
    }

    //Constructor privado
    MetalSampler::MetalSampler(id<MTLSamplerState> nativeSampler, const IlenGPUSamplerDescriptor& descriptor){
        _sampler = nativeSampler;
        [_sampler retain];
        
        this->_descriptor = descriptor;
    }

    MetalSampler::~MetalSampler() {
        if(_sampler != nil) {
            [_sampler release];
            _sampler = nil;
        }
    }

    void* MetalSampler::getNativeSampler() {
        return (__bridge void*)_sampler;
    }
}
