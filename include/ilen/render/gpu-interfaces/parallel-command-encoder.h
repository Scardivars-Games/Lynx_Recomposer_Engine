#pragma once

#include <ilen/graphics/color.h>
#include <ilen/render/gpu-interfaces/buffers.h>
#include <ilen/render/gpu-interfaces/render-encoders.h>
#include <ilen/render/gpu-interfaces/texture.h>

namespace ilen::render {
    class IRenderer;

    struct IlenEncoderAttachmentDescriptor {
        IGPUTexture *texture;
        graphics::Color clearColor;
        ILEN_GPU_RENDER_PASS_STORE_OPTION_TYPE storeOption;
        ILEN_GPU_RENDER_PASS_LOAD_OPTION_TYPE loadOption;
    };

    struct IlenParallelEncoderDescriptor {
        // Color attachment
        std::vector<std::unique_ptr<IlenEncoderAttachmentDescriptor>> colorAttachments;

        // Depth/Stencil
        std::unique_ptr<IlenEncoderAttachmentDescriptor> depthAttachment;
        std::unique_ptr<IlenEncoderAttachmentDescriptor> stencilAttachment;

        const char *label = nullptr;
    };

    // Al inicio de cada frame, la GPU genera uno de esto. Luego cada thread del submision le pide un command encoder
    // para codificar. En caso de que la API no soporte esto, ese hijo será unico y se protegera por un mutex. Si se
    // requiere algun render pass extra, la CPU puede generarlos.
    class IGPUParallelCommandEncoder {
        friend class ilen::render::IRenderer;

    public:
        virtual ~IGPUParallelCommandEncoder() = default;
        virtual void SetGlobalBuffers(uint32_t slot, IGPUBuffer *buffer) = 0;
        virtual IGPURenderEncoder *GetThreadEncoder() = 0;

    protected:
        virtual void EndEncoding() = 0; // Indica que debe cerrar el encoding.
    };
} // namespace ilen::render
