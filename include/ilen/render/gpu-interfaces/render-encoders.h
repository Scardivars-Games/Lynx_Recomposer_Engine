#pragma once

#include <ilen/render/gpu-interfaces/render-pipelines.h>
#include <ilen/render/gpu-interfaces/buffers.h>
namespace ilen::render {
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

            virtual void DrawIndexed(uint32_t indexCount) = 0;
            virtual void DrawIndexedInstanced(size_t indexCount, size_t instanceCount, size_t baseInstanceOffset) = 0;

            virtual void EndEncoding() = 0;
    };
}