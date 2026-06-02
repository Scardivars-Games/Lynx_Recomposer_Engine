#pragma once

#include <ilen/headers/enumerations.h>
#include <string>

using namespace ilen::enumerations;

namespace ilen::render {

    struct IlenGPUSamplerDescriptor {
        //Filtros
        ILEN_GPU_SAMPLER_FILTER_TYPE minFilter = ILEN_GPU_SAMPLER_FILTER_TYPE::ILEN_GPU_SAMPLER_FILTER_LINEAR;
        ILEN_GPU_SAMPLER_FILTER_TYPE magFilter = ILEN_GPU_SAMPLER_FILTER_TYPE::ILEN_GPU_SAMPLER_FILTER_LINEAR;
        ILEN_GPU_SAMPLER_MIP_FILTER_TYPE mipFilter = ILEN_GPU_SAMPLER_MIP_FILTER_TYPE::ILEN_GPU_SAMPLER_MIP_FILTER_LINEAR;

        //Modos de direccionamiento en cada eje (UVW).
        ILEN_GPU_SAMPLER_ADDRESS_MODE addressModeU = ILEN_GPU_SAMPLER_ADDRESS_MODE::ILEN_GPU_SAMPLER_ADDRESS_REPEAT;
        ILEN_GPU_SAMPLER_ADDRESS_MODE addressModeV = ILEN_GPU_SAMPLER_ADDRESS_MODE::ILEN_GPU_SAMPLER_ADDRESS_REPEAT;
        ILEN_GPU_SAMPLER_ADDRESS_MODE addressModeW = ILEN_GPU_SAMPLER_ADDRESS_MODE::ILEN_GPU_SAMPLER_ADDRESS_REPEAT;

        //Filtrado anisotrópico (1 es desactivado)
        uint16_t maxAnisotropy = 1;

        //Funcion de comparacion, necesario para leer ShadowMaps
        ILEN_GPU_COMPARE_FUNCTION compareFunction = ILEN_GPU_COMPARE_FUNCTION::ILEN_GPU_COMPARE_NEVER;
        bool compareEnable = false;//Activar si se va a leer una textura de profuncidad.

        std::string label; //Para debug

        bool operator==(const IlenGPUSamplerDescriptor& other) const {
            return (minFilter == other.minFilter) &&
                   (magFilter == other.magFilter) &&
                   (mipFilter == other.mipFilter) &&
                   (addressModeU == other.addressModeU) &&
                   (addressModeV == other.addressModeV) &&
                   (addressModeW == other.addressModeW) &&
                   (maxAnisotropy == other.maxAnisotropy) &&
                   (compareEnable == other.compareEnable) &&
                   (compareFunction == other.compareFunction);
        }
    };
    
    
    class IGPUSampler {
        public:
            virtual ~IGPUSampler() = default;
            virtual void* getNativeSampler() = 0;
            IlenGPUSamplerDescriptor getDescriptor() const { return _descriptor; }

            bool operator==(const IGPUSampler& other) const {
                return _descriptor == other._descriptor;
            }
        protected:
            IlenGPUSamplerDescriptor _descriptor;
    };
}