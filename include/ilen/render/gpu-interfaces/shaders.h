#pragma once

#include <ilen/graphics/shader.h>

namespace ilen::render {
    class IShader {
        public:
            virtual ~IShader() = default;

            virtual const graphics::IlenShader& getShaderMetadata() const = 0;
    };
}