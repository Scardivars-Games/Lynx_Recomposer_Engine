#if defined (__APPLE__)

#include "parallel-command-encoder-metal.h"
#include <ilen/headers/ilen-logs.h>

namespace ilen::render {
    MetalParallelCommandEncoder::MetalParallelCommandEncoder(id<MTLCommandBuffer> buffer, MTLRenderPassDescriptor* descriptor){
        _parallelEncoder = [buffer parallelRenderCommandEncoderWithDescriptor:descriptor];
        [_parallelEncoder retain];
    }

    MetalParallelCommandEncoder::~MetalParallelCommandEncoder() {
        if(_parallelEncoder){
            ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "Llamado al destructor del Parallel Encoder de Metal antes de terminar el codificado");
            [_parallelEncoder endEncoding];
            [_parallelEncoder release];
        }
    }

    void MetalParallelCommandEncoder::SetGlobalBuffers(uint32_t slot, IGPUBuffer *buffer){
        //Como el Parallel Encoder no puede colocar buffer globales para todos los encoder que genere, lo que se hace es almacenar los datos de esos buffers, y al crear los encoders, se les pasa para
        //que los hijos los usen como buffers globales.
        _globalBuffers[slot] = buffer;
    }

    IGPURenderEncoder* MetalParallelCommandEncoder::GetThreadEncoder(){
        @autoreleasepool {
            std::lock_guard lock(_mutex);
            
            id<MTLRenderCommandEncoder> threadEncoder = [_parallelEncoder renderCommandEncoder];
            auto engineEncoder = std::make_unique<MetalRenderEncoder>(threadEncoder);
            
            for(auto const& [slot, buffer]: _globalBuffers){
                //ILEN_LOG_P(DEBUG_PREFIX, "Setting global buffer for the single thread encoder");
                engineEncoder->SetGlobalBuffer(slot, buffer);
            }
            
            IGPURenderEncoder* rawPtr = engineEncoder.get();
            
            _childEncoders.push_back(std::move(engineEncoder));
            
            return rawPtr;
        }
    }

    void MetalParallelCommandEncoder::EndEncoding(){
        @autoreleasepool {
            if(_parallelEncoder){
                _childEncoders.clear();
                
                [_parallelEncoder endEncoding];
                [_parallelEncoder release];
                _parallelEncoder = nil;
            }
        }
    }
}

#endif
