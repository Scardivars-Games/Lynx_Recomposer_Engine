#pragma once

#include <ilen/graphics/shader.h>
#include <ilen/render/gpu-interfaces/render-pipelines.h>

namespace ilen::render {
    class IShader {
        public:
            virtual ~IShader() = default;
            virtual const ilen::graphics::IlenShader& getShaderMetadata() const = 0;
            virtual const void* getNativeShaderFromFunctionName(std::string functionName) = 0;
            virtual const void* getNativeShaderFromFunctionNameAndUberActivations(std::string functionName, std::vector<IlenConstantValuesForPipeline> activations) = 0;
        protected:
            ilen::graphics::IlenShader shaderData = {"", "", ""};
    };
}
