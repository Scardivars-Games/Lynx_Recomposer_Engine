#if defined (__APPLE__)

#include "render-encoders-metal.h"
#include "render-pipelines-metal.h"
#include "buffers-metal.h"
#include <ilen/headers/ilen-logs.h>
#include <ilen/render/render-constants.h>

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

namespace ilen::render {
    MetalRenderEncoder::MetalRenderEncoder(id<MTLRenderCommandEncoder> encoder) : _commandEncoder(encoder), _indexBuffer(nil), _indexType(MTLIndexTypeUInt32), _primitiveType(MTLPrimitiveTypeTriangle){
        [_commandEncoder retain];
    }

    MetalRenderEncoder::~MetalRenderEncoder(){
        if(_commandEncoder){
            ILEN_ERROR_P(DEBUG_PREFIX, "Llamado el destructor de un Render Encoder antes del encoding. Un error impedió completar la codificación.");
            [_commandEncoder endEncoding];
            [_commandEncoder release];
        }
    }

    void MetalRenderEncoder::SetPipelineState(IGPURenderPipeline *pso){
        MetalRenderPipeline* pipeline = static_cast<MetalRenderPipeline*>(pso);
        id<MTLRenderPipelineState> pipelineState = (__bridge id<MTLRenderPipelineState>)pipeline->getNativeRenderPipeline();
        [_commandEncoder setRenderPipelineState:pipelineState];
    }

    void MetalRenderEncoder::SetGlobalBuffer(uint32_t slot, IGPUBuffer *buffer){
        using namespace ilen::render::slots;
        
        MetalBuffer* nativeBuffer = static_cast<MetalBuffer*>(buffer);
        id<MTLBuffer> metalBuffer = (__bridge id<MTLBuffer>)nativeBuffer->getNativeHandle();
        
        //uint32_t metalSlot = slot + GlobalBufferSlot_Base; //Estaba bonita esta idea, pero no sirve si doy un buffer predefinido (como la cámara). Reescribir la lógica a futuro. De momento, se pasa directamente el slot, y solo se verifica que este en el rango de slots para uniform.
        if(slot < GlobalBufferSlot_Base || slot > UserBufferEnd){
            ILEN_ERROR_P(DEBUG_PREFIX, "Buffer slot de metal fuera de rango. El valor de slot para un buffer personalizado es entre 7 y 14, valor recibido: %i", slot);
        }
        
        [_commandEncoder setVertexBuffer:metalBuffer offset:0 atIndex:slot];
        [_commandEncoder setFragmentBuffer:metalBuffer offset:0 atIndex:slot];
    }

    void MetalRenderEncoder::SetVertexBuffer(uint32_t slot, IGPUBuffer *buffer){
        MetalBuffer* nativeBuffer = static_cast<MetalBuffer*>(buffer);
        id<MTLBuffer> metalBuffer = (__bridge id<MTLBuffer>)nativeBuffer->getNativeHandle();
        
        [_commandEncoder setVertexBuffer:metalBuffer offset:0 atIndex:slot];
    }

    void MetalRenderEncoder::SetIndexBuffer(IGPUBuffer *buffer){
        MetalBuffer* nativeBuffer = static_cast<MetalBuffer*>(buffer);
        id<MTLBuffer> metalBuffer = (__bridge id<MTLBuffer>)nativeBuffer->getNativeHandle();
        
        auto descriptor = nativeBuffer->getDescriptor();
        
        _indexBuffer = metalBuffer;
        _indexType = MTLIndexTypeUInt32; //Ilen solo use uint32_t para los indices de los IndexedDraw
    }

    void MetalRenderEncoder::SetInstancedBuffer(uint32_t slot, IGPUBuffer *buffer){
        //En metal el Instanced Rendering consiste en usar el mismo vertex buffer y el PSO hace el trabajo.
        this->SetVertexBuffer(slot, buffer);
    }

    void MetalRenderEncoder::DrawIndexed(uint32_t indexCount){
        if(!_indexBuffer){
            ILEN_ERROR_P(DEBUG_PREFIX, "Ilen engine necesita un Index Buffer para dibujar!!! Revisar el orden de llamadas para detectar el error.");
            return;
        }
        
        [_commandEncoder drawIndexedPrimitives:_primitiveType indexCount:indexCount indexType:_indexType indexBuffer:_indexBuffer indexBufferOffset:0];
    }

    void MetalRenderEncoder::DrawIndexedInstanced(size_t indexCount, size_t instanceCount, size_t baseInstanceOffset){
        if(!_indexBuffer){
            ILEN_ERROR_P(DEBUG_PREFIX, "Ilen engine necesita un Index Buffer para dibujar!!! Revisar el orden de llamadas para detectar el error.");
            return;
        }
        
        size_t baseInstance = baseInstanceOffset / sizeof(maths::matrix::mat4);
        [_commandEncoder drawIndexedPrimitives:_primitiveType indexCount:indexCount indexType:_indexType
                                   indexBuffer:_indexBuffer indexBufferOffset:0 instanceCount:instanceCount baseVertex:0 baseInstance:baseInstance];
    }

    void MetalRenderEncoder::EndEncoding(){
        if(_commandEncoder){
            [_commandEncoder endEncoding];
            [_commandEncoder release];
            _commandEncoder = nil;
        }
    }

}

#endif
