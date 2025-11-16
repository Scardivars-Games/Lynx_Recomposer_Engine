#pragma once

#if defined (__APPLE__)

#import <Metal/Metal.h>
#include <ilen/render/gpu-interfaces/parallel-command-encoder.h>
#include "render-encoders-metal.h"
#include "renderer-metal.h"
#include <map>
#include <vector>
#include <memory>
#include <mutex>

namespace ilen::render {
    class MetalParallelCommandEncoder final: public IGPUParallelCommandEncoder {
        friend class MetalRender;

        public:
            MetalParallelCommandEncoder(id<MTLCommandBuffer> buffer, const IlenParallelEncoderDescriptor& descriptor);
            virtual ~MetalParallelCommandEncoder();
            
            void SetGlobalBuffers(uint32_t slot, IGPUBuffer* buffer) override;
            IGPURenderEncoder* GetThreadEncoder() override;

        protected:
            void EndEncoding() override;

        private:
            id<MTLParallelRenderCommandEncoder> _parallelEncoder;
            std::map<uint32_t, IGPUBuffer*> _globalBuffers;
            static inline const char* DEBUG_PREFIX = "METAL_PARALLEL_RENDER_ENCODER";
        
            //Almacena la propiedad de los encoders que crea para los hijos
            std::vector<std::unique_ptr<IGPURenderEncoder>> _childEncoders;
            std::mutex _mutex;
        
            //Conversiones
            MTLStoreAction transformEngineToNativeStoreOption(enumerations::ILEN_GPU_RENDER_PASS_STORE_OPTION_TYPE option);
            MTLLoadAction transformEngineToNativeLoadOption(enumerations::ILEN_GPU_RENDER_PASS_LOAD_OPTION_TYPE option);
    };
}

#endif
