#if defined (__APPLE__)

#include "buffers-metal.h"
#import <Metal/Metal.h>


namespace ilen::render {
    MetalBuffer::MetalBuffer(id<MTLBuffer> buffer, const IlenBufferDescriptor& descriptor) : _buffer(buffer), _descriptor(descriptor) {};

    MetalBuffer::~MetalBuffer() {
        _buffer = nil;
    }

    const IlenBufferDescriptor& MetalBuffer::getDescriptor() const {
        return _descriptor;
    }

    void* MetalBuffer::getNativeHandle() const {
        return (__bridge void*)_buffer;
    }
}

#endif
