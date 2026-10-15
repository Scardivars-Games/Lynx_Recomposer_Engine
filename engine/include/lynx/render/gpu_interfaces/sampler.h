#pragma once

#include <lynx/headers/enumerations.h>
#include <string>

using namespace lynx::enumerations;

namespace lynx::render {

    struct LynxGPUSamplerDescriptor {
        //Filtros
        LYNX_GPU_SAMPLER_FILTER_TYPE minFilter = LYNX_GPU_SAMPLER_FILTER_TYPE::LYNX_GPU_SAMPLER_FILTER_LINEAR;
        LYNX_GPU_SAMPLER_FILTER_TYPE magFilter = LYNX_GPU_SAMPLER_FILTER_TYPE::LYNX_GPU_SAMPLER_FILTER_LINEAR;
        LYNX_GPU_SAMPLER_MIP_FILTER_TYPE mipFilter = LYNX_GPU_SAMPLER_MIP_FILTER_TYPE::LYNX_GPU_SAMPLER_MIP_FILTER_LINEAR;

        //Modos de direccionamiento en cada eje (UVW).
        LYNX_GPU_SAMPLER_ADDRESS_MODE addressModeU = LYNX_GPU_SAMPLER_ADDRESS_MODE::LYNX_GPU_SAMPLER_ADDRESS_REPEAT;
        LYNX_GPU_SAMPLER_ADDRESS_MODE addressModeV = LYNX_GPU_SAMPLER_ADDRESS_MODE::LYNX_GPU_SAMPLER_ADDRESS_REPEAT;
        LYNX_GPU_SAMPLER_ADDRESS_MODE addressModeW = LYNX_GPU_SAMPLER_ADDRESS_MODE::LYNX_GPU_SAMPLER_ADDRESS_REPEAT;

        //Filtrado anisotrópico (1 es desactivado)
        uint16_t maxAnisotropy = 1;

        //Funcion de comparacion, necesario para leer ShadowMaps
        LYNX_GPU_COMPARE_FUNCTION compareFunction = LYNX_GPU_COMPARE_FUNCTION::LYNX_GPU_COMPARE_NEVER;
        bool compareEnable = false;//Activar si se va a leer una textura de profuncidad.

        std::string label; //Para debug

        bool operator==(const LynxGPUSamplerDescriptor& other) const {
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
            LynxGPUSamplerDescriptor getDescriptor() const { return _descriptor; }

            bool operator==(const IGPUSampler& other) const {
                return _descriptor == other._descriptor;
            }
        protected:
            LynxGPUSamplerDescriptor _descriptor;
    };
}