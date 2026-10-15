#if defined (__APPLE__)

#include <Metal/buffers_metal.h>
#include <lynx/headers/lynx_logs.h>
#import <Metal/Metal.h>


namespace lynx::render {
    MetalBuffer::MetalBuffer(id<MTLDevice> device, const LynxBufferDescriptor& descriptor, const void* data, id<MTLCommandQueue> commandQueue) {
        @autoreleasepool {
            id<MTLBuffer> buffer;
            
            if(data == nullptr){
                //Si no hay datos al momento de crear significa que se poblara a futuro en la app. Lo mejor en este caso es que comparta la misma memoria que la CPU.
                buffer = [device newBufferWithLength:descriptor.bufferSize options: MTLResourceStorageModeShared];
                buffer.label = [NSString stringWithUTF8String: descriptor.bufferLabelName.c_str()];
                _buffer = buffer;
                _descriptor = descriptor;
            }else{
                id<MTLBuffer> staggingBuffer = [device newBufferWithBytes:data length:descriptor.bufferSize options: MTLResourceStorageModeShared];
                staggingBuffer.label = @"Stagging Buffer";
                
                buffer = [device newBufferWithLength:descriptor.bufferSize options:MTLResourceStorageModePrivate];
                buffer.label = [NSString stringWithUTF8String: descriptor.bufferLabelName.c_str()];
                
                id<MTLCommandBuffer> blitCommandBuffer = [commandQueue commandBuffer];
                blitCommandBuffer.label = @"Blit Command Buffer";
                
                id<MTLBlitCommandEncoder> blitCommandEncoder = [blitCommandBuffer blitCommandEncoder];
                blitCommandEncoder.label = @"Blit Command Encoder";
                
                [blitCommandEncoder copyFromBuffer:staggingBuffer sourceOffset:0 toBuffer:buffer destinationOffset:0 size:descriptor.bufferSize];
                [blitCommandEncoder endEncoding];
                
                [blitCommandBuffer commit];
                
                [staggingBuffer release];
                
                _buffer = buffer;
                _descriptor = descriptor;
            }
        }
    }

    const LynxBufferDescriptor& MetalBuffer::getDescriptor() const {
        return _descriptor;
    }

    void* MetalBuffer::getNativeHandle() const {
        return (__bridge void*)_buffer;
    }

    void MetalBuffer::writeDataToBuffer(const void *data, size_t size, size_t offset) {
        
        MTLStorageMode storageMode = [_buffer storageMode];
        
#if LYNX_MACOS
        if (storageMode == MTLStorageModeShared || storageMode == MTLStorageModeManaged) {
            uint8_t* bufferStart;
            uint8_t* destination;
            size_t copySize;
            
            if(size == 0){
                destination = (uint8_t*)[_buffer contents];
                copySize = _descriptor.bufferSize;
            }else{
                bufferStart = (uint8_t*)[_buffer contents];
                destination = bufferStart + offset;
                copySize = size;
            }
                  
            memcpy(destination, data, copySize);
            
            if(storageMode == MTLStorageModeManaged){
                [_buffer didModifyRange:NSMakeRange(offset, copySize)];
            }
        } else {
            LYNX_WARNING_P(DEBUG_PREFIX, "Tratando de actualizar datos en un buffer privado!!!");
        }
#else
        if (storageMode == MTLStorageModeShared) {
            uint8_t* bufferStart;
            uint8_t* destination;
            size_t copySize;
            
            if(size == 0){
                destination = (uint8_t*)[_buffer contents];
                copySize = _descriptor.bufferSize;
            }else{
                bufferStart = (uint8_t*)[_buffer contents];
                destination = bufferStart + offset;
                copySize = size;
            }
                  
            memcpy(destination, data, copySize);
        } else {
            LYNX_WARNING_P(DEBUG_PREFIX, "Tratando de actualizar datos en un buffer privado!!!");
        }
#endif
    }

    MetalBuffer::~MetalBuffer() {
        [_buffer release];
        _buffer = nil;
    };
}
#endif

