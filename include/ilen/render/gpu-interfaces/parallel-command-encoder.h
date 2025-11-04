#pragma once

#include <ilen/render/gpu-interfaces/buffers.h>
#include <ilen/render/gpu-interfaces/render-encoders.h>

namespace ilen::render {
    class IRenderer;

    //Al inicio de cada frame, la GPU genera uno de esto. Luego cada thread del submision le pide un command encoder para codificar.
    //En caso de que la API no soporte esto, ese hijo será unico y se protegera por un mutex.
    class IGPUParallelCommandEncoder {
        friend class ilen::render::IRenderer;

        public:
            virtual ~IGPUParallelCommandEncoder() = default;
            virtual void SetGlobalBuffers(uint32_t slot, IGPUBuffer* buffer) = 0;
            virtual IGPURenderEncoder* GetThreadEncoder() = 0;
        protected:
            virtual void EndEncoding() = 0; //Indica que debe cerrar el encoding.
    };
}
