#if defined (__APPLE__)
#pragma once

#import <Metal/Metal.h>    // <-- Correct import for MTLVertexFormat and MTLVertexDescriptor

#include <lynx/render/gpu_interfaces/render_pipelines.h>
#include <lynx/headers/enumerations.h>
#include <lynx/resources/shaders_manager.h>
#include <Metal_Common/common_structures_metal.h>
#include <mutex>

@protocol MTLDevice;
@protocol MTLVertexDescriptor; // OK to forward declare for pointers
@protocol MTLRenderPipelineState;

namespace lynx::render {
    using namespace lynx::enumerations;
    using namespace lynx::render::utils::metal;

    //De momento es exclusivo de Metal, ya que las demas API esta horneado dentro del _pso, y no es separado
    struct CachedDepthState {
        bool writeEnabled;
        LYNX_GPU_COMPARE_FUNCTION compareFunction;
        id<MTLDepthStencilState> state;
    };

    class MetalRenderPipeline final: public IGPURenderPipeline {
        public:
            explicit MetalRenderPipeline(id<MTLDevice> device, const lynx::render::LynxRenderPipelineDescriptor& descriptor, lynx::resources::shaders::ShaderManager* shaderManager, LynxParallelEncoderDescriptor inUseGeneralEncoder, MetalViewPredefinedValues predefinedVal);
            ~MetalRenderPipeline() override;
            const void* getNativeRenderPipeline() const override;
            id<MTLDepthStencilState> getNativeDepthStencilState() const {return _depthStencilState; }

            static size_t getFormatSize(LYNX_GPU_VERTEX_BUFFER_FORMATS_TYPE format);
            static MTLVertexFormat getMTLFormat(LYNX_GPU_VERTEX_BUFFER_FORMATS_TYPE format);
        private:
            id<MTLRenderPipelineState> _pso;
            id<MTLDepthStencilState> _depthStencilState;
            //Cache de los VertexBufferLayout
            static std::map<VertexBufferLayout, void*> m_vertexBufferDescriptors;
            static std::mutex _psoMutex;

            //Cache de los DepthStencilState
            static std::vector<CachedDepthState> _depthStencilStateCache;
            static std::mutex _depthStencilCacheMutex;

            static inline id<MTLDepthStencilState> getOrCreateDepthStencilState(id<MTLDevice> device, bool writeEnabled, LYNX_GPU_COMPARE_FUNCTION compareFunction);

            static inline auto DEBUG_PREFIX = "METAL_RENDER_PIPELINE";
            friend class IGPURenderPipeline;
    };
}

#endif