#if defined (__APPLE__)

#include <Metal/parallel_command_encoder_metal.h>
#include <Metal_Common/engine_to_native_converters.h>
#include <ilen/headers/ilen_logs.h>

namespace ilen::render {
    MetalParallelCommandEncoder::MetalParallelCommandEncoder(id<MTLCommandBuffer> buffer, IlenParallelEncoderDescriptor&& descriptor){
        using namespace ilen::render::utils::metal;
        
        MTLRenderPassDescriptor* renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
        
        //Asignando los color attachment del render pass
        for(size_t i = 0; i < descriptor.colorAttachments.size(); i++){
            if(descriptor.colorAttachments[i] != nullptr){
                id<MTLTexture> texture = (__bridge id<MTLTexture>)descriptor.colorAttachments[i]->texture->getNativeTexture();
                renderPassDescriptor.colorAttachments[i].texture = texture;
                auto clearColor = descriptor.colorAttachments[i]->clearColor;
                renderPassDescriptor.colorAttachments[i].clearColor = MTLClearColor(clearColor.red, clearColor.green, clearColor.blue, 1.0); //Esto debe venir normalmente de la camara.
                renderPassDescriptor.colorAttachments[i].storeAction = transformEngineToNativeStoreOption(descriptor.colorAttachments[i]->storeOption);
                renderPassDescriptor.colorAttachments[i].loadAction = transformEngineToNativeLoadOption(descriptor.colorAttachments[i]->loadOption);
            }
        }
        
        //Asignando el attachment para la profundidad
        if(descriptor.depthAttachment != nullptr){
            id<MTLTexture> texture = (__bridge id<MTLTexture>)descriptor.depthAttachment->texture->getNativeTexture();
            renderPassDescriptor.depthAttachment.texture = texture;
            renderPassDescriptor.depthAttachment.clearDepth = 1;
            renderPassDescriptor.depthAttachment.loadAction = transformEngineToNativeLoadOption(descriptor.depthAttachment->loadOption);
            renderPassDescriptor.depthAttachment.storeAction = transformEngineToNativeStoreOption(descriptor.depthAttachment->storeOption);
        }
        
        //Asignando el attachment para el stencil
        if(descriptor.stencilAttachment != nullptr){
            id<MTLTexture> texture = (__bridge id<MTLTexture>)descriptor.stencilAttachment->texture->getNativeTexture();
            renderPassDescriptor.stencilAttachment.texture = texture;
            renderPassDescriptor.stencilAttachment.clearStencil = 0;
            renderPassDescriptor.stencilAttachment.loadAction = transformEngineToNativeLoadOption(descriptor.stencilAttachment->loadOption);
            renderPassDescriptor.stencilAttachment.storeAction = transformEngineToNativeStoreOption(descriptor.stencilAttachment->storeOption);
        }
        
        _parallelEncoder = [buffer parallelRenderCommandEncoderWithDescriptor:renderPassDescriptor];
        _parallelEncoder.label = [NSString stringWithUTF8String: descriptor.label];
        [_parallelEncoder retain];
        
        _descriptor = std::move(descriptor);
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
}

#endif
