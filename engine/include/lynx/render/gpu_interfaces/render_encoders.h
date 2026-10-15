#pragma once

#include <lynx/render/gpu_interfaces/render_pipelines.h>
#include <lynx/render/gpu_interfaces/buffers.h>
#include <lynx/render/gpu_interfaces/texture.h>
#include <lynx/render/gpu_interfaces/sampler.h>

namespace lynx::render {
    class IGPURenderPipeline;
    class IGPURenderEncoder {
        public:
            virtual ~IGPURenderEncoder() = default;

            //Modificaciones de estado
            virtual void SetPipelineState(IGPURenderPipeline* pso) = 0;
            virtual void SetGlobalBuffer(uint32_t slot, IGPUBuffer* buffer) = 0;
            //virtual void SetMaterialBindings(uint32_t materialID); //TODO: Objetivo es bindiarlos los datos del material a la GPU, usando su respectivo buffer

            //Bindings
            virtual void SetVertexBuffer(uint32_t slot, IGPUBuffer* buffer) = 0;
            virtual void SetIndexBuffer(IGPUBuffer* buffer) = 0;
            virtual void SetInstancedBuffer(uint32_t slot, IGPUBuffer* buffer) = 0;
            virtual void SetFragmentTexture(uint32_t slot, IGPUTexture* texture) = 0;
            virtual void SetFragmentSampler(uint32_t slot, IGPUSampler* sampler) = 0;


            virtual void DrawIndexed(uint32_t indexCount) = 0;
            virtual void DrawIndexedInstanced(size_t indexCount, size_t instanceCount, size_t baseInstanceOffset, size_t instanceDataSize) = 0;
            virtual void DrawWithoutMesh() = 0; //Util para el post-procesado.

            virtual void EndEncoding() = 0;
    };
}
