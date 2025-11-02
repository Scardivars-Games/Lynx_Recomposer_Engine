#if defined (__APPLE__)
#pragma once

#import <Metal/Metal.h>    // <-- Correct import for MTLVertexFormat and MTLVertexDescriptor

#include <ilen/render/gpu-interfaces/render-pipelines.h>
#include <ilen/headers/enumerations.h>
#include <ilen/resources/shaders-manager.h>
#include <mutex>

@protocol MTLDevice;
@protocol MTLVertexDescriptor; // OK to forward declare for pointers
@protocol MTLRenderPipelineState;
@class MTKView;

namespace ilen::render {
    using namespace ilen::enumerations;

    class MetalRenderPipeline: public IGPURenderPipeline {
        public:
            explicit MetalRenderPipeline(id<MTLDevice> device, const ilen::render::IlenRenderPipelineDescriptor& descriptor, ilen::resources::shaders::ShaderManager* shaderManager, MTKView* view);
            ~MetalRenderPipeline() override;
            const void* getNativeRenderPipeline() const override;

            static size_t getFormatSize(ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE format);
            static MTLVertexFormat getMTLFormat(ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE format);
        private:
            id<MTLRenderPipelineState> _pso;
            static std::map<VertexBufferLayout, void*> m_vertexBufferDescriptors;
            static std::mutex _psoMutex;

            static inline auto DEBUG_PREFIX = "METAL_RENDER_PIPELINE";
            friend class IGPURenderPipeline;
    };
}

#endif
