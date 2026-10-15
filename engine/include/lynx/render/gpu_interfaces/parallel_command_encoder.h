#pragma once

#include <lynx/graphics/color.h>
#include <lynx/render/gpu_interfaces/buffers.h>
#include <lynx/render/gpu_interfaces/render_encoders.h>
#include <lynx/render/gpu_interfaces/texture.h>
#include <memory>
#include <cstring>

namespace lynx::render {
    class IRenderer;
    class IGPURenderEncoder;

    struct LynxEncoderAttachmentDescriptor {
        IGPUTexture *texture;
        graphics::Color clearColor;
        LYNX_GPU_RENDER_PASS_STORE_OPTION_TYPE storeOption;
        LYNX_GPU_RENDER_PASS_LOAD_OPTION_TYPE loadOption;

        bool operator==(const LynxEncoderAttachmentDescriptor &other) const {
            bool sameTex = *texture == *other.texture;
            bool sameClear = clearColor == other.clearColor;
            bool sameLoad = loadOption == other.loadOption;
            bool sameStore = storeOption == other.storeOption;
            return sameTex && sameClear && sameLoad && sameStore;
        }
    };

    struct LynxParallelEncoderDescriptor {
        // Color attachment
        std::vector<std::unique_ptr<LynxEncoderAttachmentDescriptor>> colorAttachments;
        
        // Depth/Stencil
        std::unique_ptr<LynxEncoderAttachmentDescriptor> depthAttachment;
        std::unique_ptr<LynxEncoderAttachmentDescriptor> stencilAttachment;
        
        const char *label = "";

        LynxParallelEncoderDescriptor() = default;

        LynxParallelEncoderDescriptor(LynxParallelEncoderDescriptor&& other) noexcept {
            label = other.label;
            colorAttachments = std::move(other.colorAttachments);
            depthAttachment = std::move(other.depthAttachment);
            stencilAttachment = std::move(other.stencilAttachment);
        }
        
        LynxParallelEncoderDescriptor& operator=(LynxParallelEncoderDescriptor&& other) noexcept {
            if(this == &other)
                return *this;
            
            label = other.label;
            colorAttachments = std::move(other.colorAttachments);
            depthAttachment = std::move(other.depthAttachment);
            stencilAttachment = std::move(other.stencilAttachment);
            
            return *this;
        }

        LynxParallelEncoderDescriptor(const LynxParallelEncoderDescriptor& other) {
            // Copy label (assuming it's a raw pointer, you might need to copy the string if needed)
            if (other.label) {
                label = strdup(other.label);  // Or copy to std::string if preferred
            } else {
                label = nullptr;
            }

            // Copy colorAttachments (deep copy of unique_ptrs)
            colorAttachments.clear();  // Empty the existing vector before copying
            colorAttachments.resize(other.colorAttachments.size());
            for (int i =0 ; i < other.colorAttachments.size() ; i++) {
                if (other.colorAttachments[i] == nullptr) {
                    this->colorAttachments[i] = nullptr;
                }else {
                    colorAttachments[i] = std::make_unique<LynxEncoderAttachmentDescriptor>(*other.colorAttachments[i]);
                }
            }

            // Copy depth and stencil attachments (deep copy of unique_ptrs)
            if (other.depthAttachment) {
                depthAttachment = std::make_unique<LynxEncoderAttachmentDescriptor>(*other.depthAttachment);
            } else {
                depthAttachment = nullptr;
            }

            if (other.stencilAttachment) {
                stencilAttachment = std::make_unique<LynxEncoderAttachmentDescriptor>(*other.stencilAttachment);
            } else {
                stencilAttachment = nullptr;
            }
        }

        LynxParallelEncoderDescriptor& operator=(const LynxParallelEncoderDescriptor& other) {
            if (this == &other) {
                return *this;  // Handle self-assignment
            }

            // Copy label (assuming it's a raw pointer, or use std::string if appropriate)
            if (other.label) {
                label = strdup(other.label);  // Or assign to a std::string if preferred
            } else {
                label = nullptr;
            }

            // Copy colorAttachments (deep copy of unique_ptrs)
            colorAttachments.clear();  // Empty the existing vector before copying
            colorAttachments.resize(other.colorAttachments.size());
            for (int i =0 ; i < other.colorAttachments.size() ; i++) {
                if (other.colorAttachments[i] == nullptr) {
                    this->colorAttachments[i] = nullptr;
                }else {
                    colorAttachments[i] = std::make_unique<LynxEncoderAttachmentDescriptor>(*other.colorAttachments[i]);
                }
            }

            // Copy depth and stencil attachments (deep copy of unique_ptrs)
            if (other.depthAttachment) {
                depthAttachment = std::make_unique<LynxEncoderAttachmentDescriptor>(*other.depthAttachment);
            } else {
                depthAttachment = nullptr;
            }

            if (other.stencilAttachment) {
                stencilAttachment = std::make_unique<LynxEncoderAttachmentDescriptor>(*other.stencilAttachment);
            } else {
                stencilAttachment = nullptr;
            }

            return *this;
        }



        bool operator==(const LynxParallelEncoderDescriptor &first) const {
            bool sameSize = colorAttachments.size() == first.colorAttachments.size();
            bool sameLabel = strcmp(first.label, label) == 0;
            bool sameAttach = true;
            size_t nulls = 0;
            for (int i = 0; i < colorAttachments.size() && sameSize; ++i) {
                if (colorAttachments[i] != nullptr && first.colorAttachments[i] != nullptr) {
                    if (*colorAttachments[i] != *first.colorAttachments[i]) {
                        sameAttach = false;
                        break;
                    }
                }else { nulls++; }
            }

            if (nulls == colorAttachments.size()) {
                sameSize = false;
            }
            bool sameDepth = false;
            if (depthAttachment != nullptr && first.depthAttachment != nullptr) {
                sameDepth = (*depthAttachment == *first.depthAttachment); // Desreferenciamos
            } else {
                sameDepth = (depthAttachment == first.depthAttachment); // Ambos nulos
            }

            // ---------------------------------------------------------
            // CORRECCIÓN: Comparación de Stencil Attachment
            // ---------------------------------------------------------
            bool sameStencil = false;
            if (stencilAttachment != nullptr && first.stencilAttachment != nullptr) {
                sameStencil = (*stencilAttachment == *first.stencilAttachment); // Desreferenciamos
            } else {
                sameStencil = (stencilAttachment == first.stencilAttachment); // Ambos nulos
            }

            return sameSize && sameLabel && sameDepth && sameStencil && sameAttach;
        }
    };

    // Al inicio de cada frame, la GPU genera uno de esto. Luego cada thread del submision le pide un command encoder
    // para codificar. En caso de que la API no soporte esto, ese hijo será unico y se protegera por un mutex. Si se
    // requiere algun render pass extra, la CPU puede generarlos.
    class IGPUParallelCommandEncoder {
        friend class lynx::render::IRenderer;

    public:
        virtual ~IGPUParallelCommandEncoder() = default;
        virtual void SetGlobalBuffers(uint32_t slot, IGPUBuffer *buffer) = 0;
        virtual IGPURenderEncoder *GetThreadEncoder() = 0;
        LynxParallelEncoderDescriptor* getParallelEncoderDescriptor() { return &_descriptor; }

    protected:
        virtual void EndEncoding() = 0; // Indica que debe cerrar el encoding.
        LynxParallelEncoderDescriptor _descriptor{};
    };
} // namespace lynx::render
