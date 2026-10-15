#pragma once

//Esto solo es válido en las plataformas de Apple.
#if defined (__APPLE__)

#include <lynx/render/gpu_interfaces/shaders.h>
#include <lynx/render/gpu_interfaces/render_pipelines.h>
#include <lynx/headers/lynx_logs.h>
#include <unordered_map>

@protocol MTLFunction;
@protocol MTLLibrary;

namespace  lynx::render {
    class MetalShader final: public IShader {
    public:
        MetalShader(id<MTLLibrary> globalLibrary, lynx::graphics::LynxShader shaderRecipe);

        ~MetalShader() override;

        const lynx::graphics::LynxShader& getShaderMetadata() const override;
        const void* getNativeShaderFromFunctionName(std::string functionName) override;
        const void* getNativeShaderFromFunctionNameAndUberActivations(std::string functionName, std::vector<LynxConstantValuesForPipeline> activations) override;
    private:
        std::unordered_map<std::string, void*> _shaderFunctions;
        id<MTLLibrary> _shaderLibrary;
        const char* DEBUG_PREFIX = "METAL_SHADER";
    };
}


#endif
