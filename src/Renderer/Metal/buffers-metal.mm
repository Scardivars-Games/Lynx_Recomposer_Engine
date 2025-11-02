#if defined (__APPLE__)

#include "buffers-metal.h"
#import <Metal/Metal.h>


namespace ilen::render {
    MetalBuffer::MetalBuffer(id<MTLDevice> device, const IlenBufferDescriptor& descriptor, const void* data, id<MTLCommandQueue> commandQueue) {
        @autoreleasepool {
            id<MTLBuffer> buffer;
            
            if(data == nullptr){
                //Si no hay datos al momento de crear significa que se poblara a futuro en la app. Lo mejor en este caso es que comparta la misma memoria que la CPU.
                buffer = [device newBufferWithLength:descriptor.bufferSize options: MTLResourceStorageModeShared];
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
                
                _buffer = buffer;
                _descriptor = descriptor;
            }
        }
    }

    const IlenBufferDescriptor& MetalBuffer::getDescriptor() const {
        return _descriptor;
    }

    void* MetalBuffer::getNativeHandle() const {
        return (__bridge void*)_buffer;
    }
}

#endif
