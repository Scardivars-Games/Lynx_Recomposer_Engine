#pragma once

//Esto solo es válido en las plataformas de Apple.
#if defined (__APPLE__)

#include <ilen/render/gpu-interfaces/shaders.h>
#include <ilen/render/gpu-interfaces/render-pipelines.h>
#include <ilen/headers/ilen-logs.h>
#include <unordered_map>

@protocol MTLFunction;
@protocol MTLLibrary;

namespace  ilen::render {
    class MetalShader final: public IShader {
    public:
        MetalShader(id<MTLLibrary> globalLibrary, ilen::graphics::IlenShader shaderRecipe);

        ~MetalShader() override;

        const ilen::graphics::IlenShader& getShaderMetadata() const override;
        const void* getNativeShaderFromFunctionName(std::string functionName) override;
        const void* getNativeShaderFromFunctionNameAndUberActivations(std::string functionName, std::vector<IlenConstantValuesForPipeline> activations) override;
    private:
        std::unordered_map<std::string, void*> _shaderFunctions;
        id<MTLLibrary> _shaderLibrary;
        const char* DEBUG_PREFIX = "METAL_SHADER";
    };
}


#endif
