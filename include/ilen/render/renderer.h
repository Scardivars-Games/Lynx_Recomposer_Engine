#pragma once

#include <string>
namespace ilen::render {
    const std::string DEBUG_PREFIX = "RENDER";

    enum class GraphicsAPI {
        Metal,
        Vulkan,
        OpenGL,
        DirectX12,
    };

    class Renderer {
        public:
            virtual ~Renderer() = default;
            virtual void beginFrame() = 0;
            virtual void endFrame() = 0;

            static Renderer* create(void* native_surface);
    };
}