#pragma once

#include <lynx/graphics/shader.h>
#include <lynx/render/gpu_interfaces/render_pipelines.h>

namespace lynx::render {
    class IShader {
        public:
            virtual ~IShader() = default;
            virtual const lynx::graphics::LynxShader& getShaderMetadata() const = 0;
            virtual const void* getNativeShaderFromFunctionName(std::string functionName) = 0;
            virtual const void* getNativeShaderFromFunctionNameAndUberActivations(std::string functionName, std::vector<LynxConstantValuesForPipeline> activations) = 0;
        protected:
            lynx::graphics::LynxShader shaderData = {"", "", ""};
    };
}
