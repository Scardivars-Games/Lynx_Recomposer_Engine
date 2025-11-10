#if defined (__APPLE__)
#include "instance-buffer-metal.h"
#include <ilen/headers/ilen-logs.h>
#import <Metal/Metal.h>

namespace ilen::render {
    MetalInstanceBuffer::MetalInstanceBuffer(id<MTLDevice> device, size_t bufferSize) : _currentOffset(0), _totalSize(bufferSize){
        IlenBufferDescriptor descriptor;
        descriptor.bufferType = enumerations::ILEN_RENDER_GPU_BUFFER_TYPE::ILEN_RENDER_GPU_BUFFER_UNIFORM;
        descriptor.bufferSize = bufferSize;
        descriptor.bufferLabelName = "Instance Ring Buffer";
        
        _buffer = std::make_unique<MetalBuffer>(device, descriptor, nullptr, nil);
        _nativeBuffer = (__bridge id<MTLBuffer>)_buffer->getNativeHandle();
        
        [_nativeBuffer retain];
        //[_nativeBuffer setPurgeableState:MTLPurgeableStateVolatile];//Hacemos a Metal feliz, de esa forma pueda usar esa memoria si se queda sin espacio cuando terminemos de trabajar
        //¡QUE METAL ESTE BRAVO TODO EL TIEMP! Si es volatil no puedo hacer uso en el commit, pues entonces, que sea normal, aunque a metal le molesto, igual esto se usa en todos los frames.
    }

    MetalInstanceBuffer::~MetalInstanceBuffer(){
        [_nativeBuffer release];
        _nativeBuffer = nil;
    }

    uint32_t MetalInstanceBuffer::UploadInstanceData(const void *data, size_t size){
        const size_t alignedSize = Align(size);
        
        const size_t offset = _currentOffset.fetch_add(alignedSize, std::memory_order_relaxed);
        if(offset + alignedSize > _totalSize){
            ILEN_ERROR_P(DEBUG_PREFIX, "Instance buffer sobrepasado. Favor de aumentar el tamaño en las opciones del motor, o actualizar la lógica de renderizado");
            return static_cast<uint32_t>(offset);
        }
        
        _buffer->writeDataToBuffer(data, size, offset);
        return (uint32_t)offset;
    }

    IGPUBuffer* MetalInstanceBuffer::GetBuffer() const {
        return _buffer.get();
    }

    void MetalInstanceBuffer::OnBeginFrame(){
        _currentOffset.store(0, std::memory_order_relaxed);
    }
    
    //Dado un tamaño, se encarga de regresar el próximo tamaño que sea divisible por el alignmet que exige Metal (multiplos de 256 para instance buffers).
    size_t MetalInstanceBuffer::Align(size_t size){
        return (size + bufferAlign - 1) & ~(bufferAlign - 1);
    }
}
#endif
