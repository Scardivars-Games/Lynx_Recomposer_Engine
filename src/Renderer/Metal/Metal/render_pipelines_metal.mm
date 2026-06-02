#if defined (__APPLE__)
#include <Metal/render_pipelines_metal.h>
#include <Metal/shaders_metal.h>
#include <Metal_Common/engine_to_native_converters.h>
#include <ilen/headers/ilen_logs.h>

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

namespace ilen::render {
    std::map<VertexBufferLayout, void*> MetalRenderPipeline::m_vertexBufferDescriptors;
    std::mutex MetalRenderPipeline::_psoMutex;
    std::vector<CachedDepthState> MetalRenderPipeline::_depthStencilStateCache;
    std::mutex MetalRenderPipeline::_depthStencilCacheMutex;

    MetalRenderPipeline::MetalRenderPipeline(id<MTLDevice> device, const ilen::render::IlenRenderPipelineDescriptor& descriptor, ilen::resources::shaders::ShaderManager* shaderManager, IlenParallelEncoderDescriptor inUseGeneralEncoder, MetalViewPredefinedValues predefinedVal) : _pso(nil){
        using namespace ilen::render::utils::metal;
        @autoreleasepool {
            
            MTLRenderPipelineDescriptor* pipelineDescriptor = [MTLRenderPipelineDescriptor new];
            
            auto vertexPtr = shaderManager->getShaderData(descriptor.vertexShaderID).lock();
            auto fragmentPtr = shaderManager->getShaderData(descriptor.fragmentShaderID).lock();
            
            if(!vertexPtr || !fragmentPtr){
                ILEN_ERROR_P(DEBUG_PREFIX, "No fue posible obtener los shader con ID %i(vertex) y %i(fragment). Nombre de la Pipeline: %s", descriptor.vertexShaderID, descriptor.fragmentShaderID, descriptor.pipelineName.c_str());
                return;
            }
            
            id<MTLFunction> vertexFunction, fragmentFunction;
            if(descriptor.constantValuesForPipelines.size() == 0){
                vertexFunction = (__bridge id<MTLFunction>)vertexPtr.get()->getNativeShaderFromFunctionName(descriptor.vertexFunctionName);
                fragmentFunction = (__bridge id<MTLFunction>)fragmentPtr.get()->getNativeShaderFromFunctionName(descriptor.fragmentFunctionName);
            }else{
                vertexFunction = (__bridge id<MTLFunction>)vertexPtr.get()->getNativeShaderFromFunctionNameAndUberActivations(descriptor.vertexFunctionName, descriptor.constantValuesForPipelines);
                fragmentFunction = (__bridge id<MTLFunction>)fragmentPtr.get()->getNativeShaderFromFunctionNameAndUberActivations(descriptor.fragmentFunctionName, descriptor.constantValuesForPipelines);
            }
            

            
            if(!vertexFunction || !fragmentFunction){
                ILEN_ERROR_P(DEBUG_PREFIX, "Las funciones %s y %s no fueron encontradas en su respectivo shader!!!.", descriptor.vertexFunctionName.c_str(), descriptor.fragmentFunctionName.c_str());
                return;
            }
            
            pipelineDescriptor.label = [NSString stringWithFormat:@"PSO - %s (%s / %s)", descriptor.pipelineName.c_str(), descriptor.vertexFunctionName.c_str(), descriptor.fragmentFunctionName.c_str()];
            pipelineDescriptor.vertexFunction = vertexFunction;
            pipelineDescriptor.fragmentFunction = fragmentFunction;
            
            
            VertexBufferLayout layout;
            
            switch(descriptor.vertexLayout){
                case enumerations::ILEN_VERTEX_LAYOUT_TYPE::ILEN_VERTEX_LAYOUT_GLOBAL:
                    layout = ILEN_GLOBAL_VERTEX_LAYOUT;
                    break;
                case enumerations::ILEN_VERTEX_LAYOUT_TYPE::ILEN_VERTEX_LAYOUT_VERTEXLESS:
                    layout = ILEN_VERTEXLESS_LAYOUT;
                    break;
                default:
                    layout = ILEN_GLOBAL_VERTEX_LAYOUT;
                    break;
            }


            void* cachedVertexDescriptor = nullptr;
            if(descriptor.vertexLayout != enumerations::ILEN_VERTEX_LAYOUT_TYPE::ILEN_VERTEX_LAYOUT_VERTEXLESS) {
                std::lock_guard lock(_psoMutex);
                
                auto it = m_vertexBufferDescriptors.find(layout);

                if(it != m_vertexBufferDescriptors.end()){
                    cachedVertexDescriptor = it->second;
                }else{
                    cachedVertexDescriptor = generateVertexDescriptor(layout);
                    m_vertexBufferDescriptors.emplace(layout, cachedVertexDescriptor);
                }
            }
            
            pipelineDescriptor.vertexDescriptor = (__bridge MTLVertexDescriptor*)cachedVertexDescriptor;
            
            auto* colorAttach = pipelineDescriptor.colorAttachments[0];
            
            //Blend operations segun el render pass.
            switch (descriptor.renderQueue) {
                case enumerations::ILEN_GPU_RENDER_QUEUE_TYPE::ILEN_GPU_RENDER_QUEUE_OPAQUE:
                case enumerations::ILEN_GPU_RENDER_QUEUE_TYPE::ILEN_GPU_RENDER_QUEUE_ALPHA_TEST:
                    colorAttach.blendingEnabled = NO;
                    break;
                case enumerations::ILEN_GPU_RENDER_QUEUE_TYPE::ILEN_GPU_RENDER_QUEUE_TRANSPARENT:
                case enumerations::ILEN_GPU_RENDER_QUEUE_TYPE::ILEN_GPU_RENDER_QUEUE_OVERLAY:
                    colorAttach.blendingEnabled = YES;
                    colorAttach.rgbBlendOperation = MTLBlendOperationAdd;
                    colorAttach.alphaBlendOperation = MTLBlendOperationAdd;
                    colorAttach.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
                    colorAttach.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
                    colorAttach.sourceAlphaBlendFactor = MTLBlendFactorOne;
                    colorAttach.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
                    break;
                default:
                    break;
            }
            //Colores del pixel
            auto encoderDesc = descriptor.encoderDescriptor;
            if(descriptor.encoderDescriptor == nullptr){
                encoderDesc = &inUseGeneralEncoder;
                //ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "Solicitud de creación de un PSO especial sin el descriptor con los datos del encoder.");
            }
            if(encoderDesc->colorAttachments.size() > 0){
                colorAttach.pixelFormat = transformEngineToNativePixelFormat(encoderDesc->colorAttachments[0]->texture->getDescriptor().pixelFormat, predefinedVal.screenPixelFormat);
            }else{
                colorAttach.pixelFormat = MTLPixelFormatBGRA8Unorm; //Esto debería ser invalid, pero el simulador de iOS no le gusta
            }
            
            if(ilen::core::options::GraphicsOptions::graphicsOptionsFlags & ILEN_GRAPHICS_OPTIONS_USE_DEPTH && !(descriptor.renderPipelineOptionFlags & ILEN_RENDER_PIPELINES_OPTIONS_DONT_USES_DEPTH_BUFFER )){
                pipelineDescriptor.depthAttachmentPixelFormat = predefinedVal.depthStencilPixelFormat;
            }else{
                pipelineDescriptor.depthAttachmentPixelFormat = MTLPixelFormatInvalid;
            }
            
            if(ilen::core::options::GraphicsOptions::graphicsOptionsFlags & ILEN_GRAPHICS_OPTIONS_USE_STENCIL && !(descriptor.renderPipelineOptionFlags & ILEN_RENDER_PIPELINES_OPTIONS_DONT_USES_STENCIL_BUFFER)){
                pipelineDescriptor.stencilAttachmentPixelFormat = predefinedVal.depthStencilPixelFormat;
            }else{
                pipelineDescriptor.stencilAttachmentPixelFormat = MTLPixelFormatInvalid;
            }
            /* En METAL siempre hay que definir los descriptores, auque el PSO no lo use.
            if(descriptor.isSpecialPipeline){
                
            }*/
            
            if(descriptor.renderPipelineOptionFlags & ILEN_RENDER_PIPELINES_OPTIONS_DONT_WRITE_SCREEN_TEXTURE){
                //Special Pipelines son aquellos que no escriben en el buffer 0 (dibujado normal) Esto es principalmente para PSO de Post-Procesado
                pipelineDescriptor.colorAttachments[0].writeMask = MTLColorWriteMaskNone;
            }
            
            //Empezamos en 1 porque el color 0 siempre esta reservada para el dibujo de la pantalla, y para nada mas :)
            for(size_t i = 1; i < encoderDesc->colorAttachments.size(); i++){
                if(encoderDesc->colorAttachments[i] != nullptr){
                   auto texFormat =  encoderDesc->colorAttachments[i]->texture->getDescriptor().pixelFormat;
                    pipelineDescriptor.colorAttachments[i].pixelFormat = transformEngineToNativePixelFormat(texFormat, predefinedVal.screenPixelFormat);
                    pipelineDescriptor.colorAttachments[i].writeMask = MTLColorWriteMaskAll;
                    pipelineDescriptor.colorAttachments[i].blendingEnabled = NO;
                }
            }
            
            NSError* error = nil;
            
            m_renderQueue = descriptor.renderQueue;
            
            _pso = [device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
            this->descriptor = descriptor;
            
            [pipelineDescriptor release];
            
            if(!_pso){
                ILEN_ERROR_P(DEBUG_PREFIX, "Error al crear el MTLRenderPipelineState. Error: %s.", [error.localizedDescription UTF8String]);
            }
            
            //Generando el DepthStateStencil correspondiente
            if(ilen::core::options::GraphicsOptions::graphicsOptionsFlags & ILEN_GRAPHICS_OPTIONS_USE_DEPTH || ilen::core::options::GraphicsOptions::graphicsOptionsFlags & ILEN_GRAPHICS_OPTIONS_USE_STENCIL){
                if(!(descriptor.renderPipelineOptionFlags & ILEN_RENDER_PIPELINES_OPTIONS_DONT_USES_DEPTH_BUFFER) || !(descriptor.renderPipelineOptionFlags & ILEN_RENDER_PIPELINES_OPTIONS_DONT_USES_STENCIL_BUFFER)){
                    bool depthWrite = !(descriptor.renderPipelineOptionFlags & ILEN_RENDER_PIPELINE_OPTIONS_DONT_WRITE_DEPTH_BUFFER);
                    _depthStencilState = getOrCreateDepthStencilState(device, depthWrite, descriptor.depthCompareFunction);
                }
            }
        }
    }

    MetalRenderPipeline::~MetalRenderPipeline() {
        [(_pso) release];
        _pso = nil;
    }

    const void* MetalRenderPipeline::getNativeRenderPipeline() const {
        return (__bridge void*)_pso;
    }

    void* IGPURenderPipeline::generateVertexDescriptor(VertexBufferLayout layout) {
        @autoreleasepool {
            MTLVertexDescriptor* vertexDescriptor = [MTLVertexDescriptor vertexDescriptor];
            vertexDescriptor.layouts[0].stride = layout.stride;
            
            for(int i = 0; i < layout.attributes.size(); i++){
                auto actualAttribute = layout.attributes[i];
                MTLVertexFormat mtlFormat = MetalRenderPipeline::getMTLFormat(actualAttribute);
                //size_t formatSize = MetalRenderPipeline::getFormatSize(actualAttribute);
                
                if(mtlFormat == MTLVertexFormatInvalid){
                    ILEN_ERROR_P(MetalRenderPipeline::DEBUG_PREFIX, "El atributo con índice %i no es válido para el layout dado.", i);
                    continue;
                }
                
                auto attributeIndex = layout.attributeIndex[i];
                
                vertexDescriptor.attributes[attributeIndex].format = mtlFormat;
                vertexDescriptor.attributes[attributeIndex].offset = layout.offsets[i];
                vertexDescriptor.attributes[attributeIndex].bufferIndex = 0;
                
            }
            
            return [vertexDescriptor retain];
        }
    }

    size_t MetalRenderPipeline::getFormatSize(ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE format){
        switch(format){
            case ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE::ILEN_GPU_VERTEX_BUFFER_FORMAT_INT: return sizeof(int);
            case ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE::ILEN_GPU_VERTEX_BUFFER_FORMAT_FLOAT: return sizeof(float);
            case ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE::ILEN_GPU_VERTEX_BUFFER_FORMAT_FLOAT2: return sizeof(float) * 2;
            case ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE::ILEN_GPU_VERTEX_BUFFER_FORMAT_FLOAT3: return sizeof(float) * 3;
            case ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE::ILEN_GPU_VERTEX_BUFFER_FORMAT_FLOAT4: return sizeof(float) * 4;
            default: return 0;
        }
    }

    MTLVertexFormat MetalRenderPipeline::getMTLFormat(ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE format){
        switch(format){
            case ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE::ILEN_GPU_VERTEX_BUFFER_FORMAT_INT: return MTLVertexFormatInt;
            case ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE::ILEN_GPU_VERTEX_BUFFER_FORMAT_FLOAT: return MTLVertexFormatFloat;
            case ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE::ILEN_GPU_VERTEX_BUFFER_FORMAT_FLOAT2: return MTLVertexFormatFloat2;
            case ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE::ILEN_GPU_VERTEX_BUFFER_FORMAT_FLOAT3: return MTLVertexFormatFloat3;
            case ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE::ILEN_GPU_VERTEX_BUFFER_FORMAT_FLOAT4: return MTLVertexFormatFloat4;
            default: return MTLVertexFormatInvalid;
        }
    }

    inline id<MTLDepthStencilState> MetalRenderPipeline::getOrCreateDepthStencilState(id<MTLDevice> device, bool writeEnabled, ILEN_GPU_COMPARE_FUNCTION compareFunction){
        std::lock_guard<std::mutex> lock(_depthStencilCacheMutex);
        
        for(const auto& cached: _depthStencilStateCache) {
            if(cached.writeEnabled == writeEnabled && cached.compareFunction == compareFunction){
                return cached.state;
            }
        }
        
        //Crear uno nuevo porque no existe
        @autoreleasepool {
            MTLDepthStencilDescriptor *descriptor = [[MTLDepthStencilDescriptor alloc] init];
            descriptor.depthWriteEnabled = writeEnabled;
            descriptor.depthCompareFunction = transformEngineToNativeCompareFunction(compareFunction);
            
            id<MTLDepthStencilState> newState = [device newDepthStencilStateWithDescriptor:descriptor];
            [descriptor release];
            
            if(!newState){
                ILEN_ERROR_P(DEBUG_PREFIX, "No se pudo crear el MTLDepthStencilState.");
                return nil;
            }
            
            _depthStencilStateCache.push_back({writeEnabled, compareFunction, newState});
            
            return newState;
        }
    }
}

#endif
