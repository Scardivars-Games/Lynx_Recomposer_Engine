#pragma once
#if defined (__APPLE__)

#include <ilen/render/gpu-interfaces/render-encoders.h>
#import <Metal/Metal.h>

@protocol MTLRenderCommandEncoder;
@protocol MTLBuffer;

namespace ilen::render {
    class MetalRenderEncoder final: public IGPURenderEncoder {
        public:
            MetalRenderEncoder(id<MTLRenderCommandEncoder> encoder);
            virtual ~MetalRenderEncoder();
            
            void SetPipelineState(IGPURenderPipeline* pso) override;
            void SetGlobalBuffer(uint32_t slot, IGPUBuffer* buffer) override;

            void SetVertexBuffer(uint32_t slot, IGPUBuffer* buffer) override;
            void SetIndexBuffer(IGPUBuffer* buffer) override;
            void SetInstancedBuffer(uint32_t slot, IGPUBuffer* buffer) override;
            void SetFragmentTexture(uint32_t slot, IGPUTexture* texture) override;
            void SetFragmentSampler(uint32_t slot, IGPUSampler* sampler) override;
            
            void DrawIndexed(uint32_t indexCount) override;
            void DrawIndexedInstanced(size_t indexCount, size_t instanceCount, size_t baseInstanceOffset, size_t instanceDataSize) override;
            void DrawWithoutMesh() override;
            
            void EndEncoding() override;
        private:
            id<MTLRenderCommandEncoder> _commandEncoder;
            id<MTLBuffer> _indexBuffer;
            MTLIndexType _indexType; //Si los valores del index buffer son uint32_t o uint16_t
            MTLPrimitiveType _primitiveType;
            static inline const char* DEBUG_PREFIX = "METAL_RENDER_ENCODER";
    };
}

#endif
