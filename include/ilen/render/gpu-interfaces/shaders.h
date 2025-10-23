#pragma once

#include <ilen/graphics/shader.h>

namespace ilen::render {
    class IShader {
        public:
            virtual ~IShader() = default;
            virtual const ilen::graphics::IlenShader& getShaderMetadata() const = 0;
            virtual const void* getNativeShaderFromFunctionName(std::string functionName) = 0;
        protected:
            ilen::graphics::IlenShader shaderData = {"", "", ""};
    };
}
