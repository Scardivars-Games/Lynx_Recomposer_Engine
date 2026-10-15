#pragma once

#if defined (__APPLE__)

#import <Metal/Metal.h>
#include <lynx/render/gpu_interfaces/parallel_command_encoder.h>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
#include <Metal/render_encoders_metal.h>
#include <Metal/renderer_metal.h>

namespace lynx::render {
    class MetalParallelCommandEncoder final: public IGPUParallelCommandEncoder {
        friend class MetalRender;

        public:
            MetalParallelCommandEncoder(id<MTLCommandBuffer> buffer, LynxParallelEncoderDescriptor&& descriptor);
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
    };
}

#endif
