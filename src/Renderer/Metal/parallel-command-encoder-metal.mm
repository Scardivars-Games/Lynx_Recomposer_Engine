#if defined (__APPLE__)

#include "parallel-command-encoder-metal.h"
#include <ilen/headers/ilen-logs.h>

namespace ilen::render {
    MetalParallelCommandEncoder::MetalParallelCommandEncoder(id<MTLCommandBuffer> buffer, const IlenParallelEncoderDescriptor& descriptor){
        MTLRenderPassDescriptor* renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
        /*for(size_t i = 0; i < descriptor.colorAttachments.size(); i++){
            renderPassDescriptor.colorAttachments[i] =
        }*/
        
        _parallelEncoder = [buffer parallelRenderCommandEncoderWithDescriptor:descriptor];
        [_parallelEncoder retain];
    }

    MetalParallelCommandEncoder::~MetalParallelCommandEncoder() {
        if(_parallelEncoder){
            [_parallelEncoder endEncoding];
            [_parallelEncoder release];
            ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "Llamado al destructor del Parallel Encoder de Metal antes de terminar el codificado");
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

    MTLStoreAction MetalParallelCommandEncoder::transformEngineToNativeStoreOption(enumerations::ILEN_GPU_RENDER_PASS_STORE_OPTION_TYPE option){
        switch (option) {
            case ILEN_GPU_RENDER_PASS_STORE_OPTION_TYPE::ILEN_GPU_RENDER_PASS_STORE_OPTION_STORE:
                return MTLStoreActionStore;
            case ILEN_GPU_RENDER_PASS_STORE_OPTION_TYPE::ILEN_GPU_RENDER_PASS_STORE_OPTION_DISCARD:
            case ILEN_GPU_RENDER_PASS_STORE_OPTION_TYPE::ILEN_GPU_RENDER_PASS_STORE_OPTION_DONT_CARE:
                return MTLStoreActionDontCare;
        }
    }

    MTLLoadAction MetalParallelCommandEncoder::transformEngineToNativeLoadOption(enumerations::ILEN_GPU_RENDER_PASS_LOAD_OPTION_TYPE option){
        switch (option) {
            case ILEN_GPU_RENDER_PASS_LOAD_OPTION_TYPE::ILEN_GPU_RENDER_PASS_LOAD_OPTION_LOAD:
                return MTLLoadActionLoad;
            case ILEN_GPU_RENDER_PASS_LOAD_OPTION_TYPE::ILEN_GPU_RENDER_PASS_LOAD_OPTION_CLEAR:
                return MTLLoadActionClear;
            case ILEN_GPU_RENDER_PASS_LOAD_OPTION_TYPE::ILEN_GPU_RENDER_PASS_LOAD_OPTION_DONT_CARE:
                return MTLLoadActionDontCare;
        }
    }
}

#endif
