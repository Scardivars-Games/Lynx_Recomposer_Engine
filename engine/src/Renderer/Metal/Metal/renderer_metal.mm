#if defined (__APPLE__)
//Este código es uniforme para todas las plataformas de Apple (macOS y iOS de momento)
#include <Metal/renderer_metal.h>
#include <Metal/buffers_metal.h>
#include <Metal/shaders_metal.h>
#include <Metal/render_pipelines_metal.h>
#include <Metal/parallel_command_encoder_metal.h>
#include <Metal/texture_metal.h>
#include <Metal/instance_buffer_metal.h>
#include <Metal/sampler_metal.h>
#include <lynx/headers/lynx_logs.h>
#include <lynx/window/window.h>

#include <TargetConditionals.h>

#import <QuartzCore/QuartzCore.h>
#import <Metal/Metal.h>

namespace lynx::render {

    //TODO: ADD Metal 4 support to use the more efficients encoders!!!
    MetalRender::MetalRender(const void* native_surface){
        @autoreleasepool {
            //lastUsedGraphicsOptions = defaultOptions;
            p_runtimeConfiguration = application::Application::getEngineContext()->runtimeConfiguration.get();
#ifdef LYNX_MACOS
            _metalView = (__bridge NSView*)native_surface;
#elif defined LYNX_IOS || LYNX_TVOS
            _metalView = (__bridge UIView*)native_surface;
            
#endif
            _metalLayer = (CAMetalLayer*)_metalView.layer;
            _device = _metalLayer.device;
            if(!_device){
                //Es imposible crear el dispositivo de Metal!!! (Fallback a OpenGL)
                LYNX_EXECUTE_ERROR_P(DEBUG_PREFIX, "No se logro crear el contexto de Metal. Dispositivo inválido");
            }
            
            defineSupportedGPUCompression();

            LYNX_LOG("GPU compression: Apple2=%d BC=%d selected=%u", [_device supportsFamily:MTLGPUFamilyApple2],
            #if LYNX_MACOS || TARGET_OS_SIMULATOR
                [_device supportsBCTextureCompression],
            #else
                0,
            #endif
                static_cast<uint32_t>(compressionActualFormat)
            );

            
            LYNX_LOG("Device: %p", _metalLayer.device);
            LYNX_LOG("DrawableSize: %.0f x %.0f", _metalLayer.drawableSize.width, _metalLayer.drawableSize.height);
            
           
            _commandQueue = [_device newCommandQueue];
            
            _predefinedValues.screenPixelFormat = _metalLayer.pixelFormat;
            _predefinedValues.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
            #if LYNX_MACOS
            _predefinedValues.clearColor = MTLClearColor(0.1, 0.1, 0.2, 1.0);
            #elif LYNX_IOS
            _predefinedValues.clearColor = MTLClearColor(0.5, 0.2, 0.2, 1.0);
            #elif LYNX_TVOS
            _predefinedValues.clearColor = MTLClearColor(0.83, 0.68, 0.21, 1.0);
            #endif //TARGET_OS_IOS
            
            #if LYNX_MACOS
            //Cargar .metallib
            NSString* libraryPath = [[NSBundle mainBundle] pathForResource:@"lynx" ofType:@"metallib" inDirectory:@"shader_code"];
            #elif LYNX_IOS || LYNX_TVOS
            NSString* libraryPath = [[NSBundle mainBundle] pathForResource:@"lynx" ofType:@"metallib" inDirectory:@"assets/shader_code"];
            #endif
            
            if(!libraryPath){
                LYNX_EXECUTE_ERROR_P(DEBUG_PREFIX, "No se encontro libreria por defecto de Metal");
                generateNewMetalLibrary();
            }else{
                NSURL* libraryURL = [NSURL fileURLWithPath:libraryPath];
                NSError *localError = nil;
                _lynxMetalLibrary = [_device newLibraryWithURL:libraryURL error:&localError];
                if(!_lynxMetalLibrary){
                    LYNX_EXECUTE_ERROR_P(DEBUG_PREFIX, "Imposible cargar libreria por defecto del motor %@", localError);
                }
            }
            
            _instanceBuffer.resize(MAX_FRAMES_IN_FLIGHT);
            _depthTexture.resize(MAX_FRAMES_IN_FLIGHT);
            _commandBuffer.resize(MAX_FRAMES_IN_FLIGHT);
            _drawables.resize(MAX_FRAMES_IN_FLIGHT);
            _drawableTexture.resize(MAX_FRAMES_IN_FLIGHT);
            _parallelCommandEncoder.resize(MAX_FRAMES_IN_FLIGHT);

            
            size_t instanceBufferSize = p_runtimeConfiguration->graphics.instanceBufferSizeInMB * 1024 * 1024; //Obtenemos el tamaño del buffer, de acuerdo a las opciones, y llevamos ese valor a los bytes que corresponden.
            for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
                _instanceBuffer[i] = createInstanceBuffer(instanceBufferSize, "Instance Main Ring Buffer -> Frame " + std::to_string(_actualFrame));
            }
            
            _frameSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
            for (auto& semaphore : _frameSemaphores) {
                semaphore = dispatch_semaphore_create(1);
            }
            
            
            //Es importante que los command encoder
            
            int totalOfCustomTextures = 0;
            if(p_runtimeConfiguration->graphics.flags & LYNX_GRAPHICS_OPTIONS_USE_POSTPROCESS){
                totalOfCustomTextures += MAX_FRAMES_IN_FLIGHT; //Estas texturas son los render target de los Pass Descriptor. La idea es, por cada pass que no dibuje en pantalla, crea las texturas, y luego se usa un índice que viene en el Constructor que doy para saber en que bloque de texturas leer. 0 sería la textura de pantalla (i * 0 = 0), así que es buena idea, y a partir del 1, pues se obtiene el índice de la custom texture.
                if(p_runtimeConfiguration->graphics.flags & LYNX_GRAPHICS_OPTIONS_USE_MOTION_BLUR){
                    totalOfCustomTextures += MAX_FRAMES_IN_FLIGHT; //Para almacenar el velocity buffer en al attachment 1.
                }
            }
            
            _sceneTexture.resize(totalOfCustomTextures);
            generateFrameTextures();
            regenerateEncoderDescriptor();
        }
    }

    MetalRender::~MetalRender() {
        // Esperar a los lectores de cada slot antes de liberar sus recursos.
        for (auto semaphore : _frameSemaphores) {
            dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
            dispatch_semaphore_signal(semaphore);
            dispatch_release(semaphore);
        }
        for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
            if(_parallelCommandEncoder[i]){
                IRenderer::FinalizeEncoding(_parallelCommandEncoder[i].get());
                _parallelCommandEncoder[i].reset();
            }
        }

        _device = nil;
        _commandQueue = nil;
        _lynxMetalLibrary = nil;
    }

void MetalRender::beginFrame() {
   /* 
    //        LYNX_LOG("Waitin for frame %i. Start time %.8f", _actualFrame, lynx::time::Time::getTimeSinceCurrentFrameStart());
    dispatch_semaphore_wait(_frameSemaphores[_actualFrame], DISPATCH_TIME_FOREVER);
    //LYNX_LOG("Getted frame %i. End time %.8f", _actualFrame, lynx::time::Time::getTimeSinceCurrentFrameStart());
    
    _frameAutoReleasePool = [[NSAutoreleasePool alloc] init];
    generateFrameTextures();
    _instanceBuffer[_actualFrame]->OnBeginFrame();
    
    //_currentDrawable = [metalView currentDrawable];
    
    if(!_currentDrawable){
     _commandBuffer = nil;
     [(__bridge NSAutoreleasePool*)_frameAutoReleasePool drain];
     _frameAutoReleasePool = nil;
     return;
     }
    
    //_drawables[_actualFrame] = [metalView currentDrawable];
    
    //CAMetalLayer* layer = (CAMetalLayer*)[metalView layer];
    if(_metalLayer.drawableSize.width < 1 || _metalLayer.drawableSize.height < 1){
        _commandBuffer[_actualFrame] = nil;
        _drawables[_actualFrame] = nil;
        [(__bridge NSAutoreleasePool*)_frameAutoReleasePool drain];
        _frameAutoReleasePool = nil;
        return;
    }
    
    if(lastUsedGraphicsOptions != GraphicsOptions::graphicsOptionsFlags){
        regenerateEncoderDescriptor();
    }
    
    LynxParallelEncoderDescriptor parallelEncoderDescriptor = actualMainDescriptor;
    
    if(GraphicsOptions::graphicsOptionsFlags & LYNX_GRAPHICS_OPTIONS_USE_POSTPROCESS){
        parallelEncoderDescriptor.colorAttachments[slots::RenderPassAttachment_Color]->texture = _sceneTexture[_actualFrame].get();
        uint32_t actualIndexForNextTexture = MAX_FRAMES_IN_FLIGHT;

        if(GraphicsOptions::graphicsOptionsFlags & LYNX_GRAPHICS_OPTIONS_USE_MOTION_BLUR){
            parallelEncoderDescriptor.colorAttachments[slots::RenderPassAttachment_VelocityBuffer]->texture = _sceneTexture[_actualFrame + actualIndexForNextTexture].get();
            actualIndexForNextTexture += MAX_FRAMES_IN_FLIGHT;
        }
    }else{
        parallelEncoderDescriptor.colorAttachments[slots::RenderPassAttachment_Color]->texture = _drawableTexture[_actualFrame].get();
    }
    
    if(GraphicsOptions::graphicsOptionsFlags & LYNX_GRAPHICS_OPTIONS_USE_DEPTH){
        parallelEncoderDescriptor.depthAttachment->texture = _depthTexture[_actualFrame].get();
    }
    
            
    _commandBuffer[_actualFrame] = [_commandQueue commandBuffer];
    _parallelCommandEncoder[_actualFrame] = std::make_unique<MetalParallelCommandEncoder>(_commandBuffer[_actualFrame], std::move(parallelEncoderDescriptor));*/

    dispatch_semaphore_wait(_frameSemaphores[_actualFrame], DISPATCH_TIME_FOREVER);

    _frameAutoReleasePool = [[NSAutoreleasePool alloc] init];
    generateFrameTextures();
    _instanceBuffer[_actualFrame]->OnBeginFrame();
    _drawableAcquired = false; // flag interno

    if (_metalLayer.drawableSize.width < 1 || _metalLayer.drawableSize.height < 1) {
        _commandBuffer[_actualFrame] = nil;
        [(__bridge NSAutoreleasePool*)_frameAutoReleasePool drain];
        _frameAutoReleasePool = nullptr;
        return;
    }

    // Command buffer si, encoder NO todavia
    _commandBuffer[_actualFrame] = [_commandQueue commandBuffer];

    if (lastUsedGraphicsOptions != p_runtimeConfiguration->graphics.flags) {
        regenerateEncoderDescriptor();
    }
}

bool MetalRender::acquireDrawable(){
   if (_drawableAcquired) return true; // idempotente, se puede llamar varias veces

    _drawables[_actualFrame] = [_metalLayer nextDrawable];
    if (!_drawables[_actualFrame]) {
        _commandBuffer[_actualFrame] = nil;
        [(__bridge NSAutoreleasePool*)_frameAutoReleasePool drain];
        _frameAutoReleasePool = nullptr;
        return false;
    }

    _drawableTexture[_actualFrame] = std::make_unique<MetalTexture>(_drawables[_actualFrame].texture);

    // Ahora si construimos el encoder con el render target correcto
    LynxParallelEncoderDescriptor parallelEncoderDescriptor = actualMainDescriptor;

    if (p_runtimeConfiguration->graphics.flags & LYNX_GRAPHICS_OPTIONS_USE_POSTPROCESS) {
        parallelEncoderDescriptor.colorAttachments[slots::RenderPassAttachment_Color]->texture
            = _sceneTexture[_actualFrame].get();

        uint32_t actualIndexForNextTexture = MAX_FRAMES_IN_FLIGHT;
        if (p_runtimeConfiguration->graphics.flags & LYNX_GRAPHICS_OPTIONS_USE_MOTION_BLUR) {
            parallelEncoderDescriptor.colorAttachments[slots::RenderPassAttachment_VelocityBuffer]->texture
                = _sceneTexture[_actualFrame + actualIndexForNextTexture].get();
        }
    } else {
        // Sin post-process: el drawable ES el render target final
        parallelEncoderDescriptor.colorAttachments[slots::RenderPassAttachment_Color]->texture
            = _drawableTexture[_actualFrame].get();
    }

    if (p_runtimeConfiguration->graphics.flags & LYNX_GRAPHICS_OPTIONS_USE_DEPTH) {
        parallelEncoderDescriptor.depthAttachment->texture = _depthTexture[_actualFrame].get();
    }

    _parallelCommandEncoder[_actualFrame] = std::make_unique<MetalParallelCommandEncoder>(
        _commandBuffer[_actualFrame], std::move(parallelEncoderDescriptor));

    _drawableAcquired = true;
    return true;
}

    void MetalRender::endFrame() {
        //LYNX_LOG("Drawable: %i", _actualFrame);
        if(!_drawableAcquired){
            acquireDrawable();
        }
        
        if(isParallelEncoderInOtherObject){
            [(__bridge NSAutoreleasePool*)_frameAutoReleasePool drain];
            _frameAutoReleasePool = nullptr;
            LYNX_EXECUTE_ERROR_P(DEBUG_PREFIX, "The parallel encoder was given to another process, but never returned a parallel encoder to the renderer");
            return;
        }
        
        if(!_commandBuffer[_actualFrame]){
            [(__bridge NSAutoreleasePool*)_frameAutoReleasePool drain];
            _frameAutoReleasePool = nullptr;
            if(_drawables[_actualFrame] == nil){
                dispatch_semaphore_signal(_frameSemaphores[_actualFrame]);
                _actualFrame = (_actualFrame + 1) % MAX_FRAMES_IN_FLIGHT;
            }
            return;
        }
    
        if(_parallelCommandEncoder[_actualFrame]){
            IRenderer::FinalizeEncoding(_parallelCommandEncoder[_actualFrame].get());
            _parallelCommandEncoder[_actualFrame].reset();
        }else{
            LYNX_ERROR_P(DEBUG_PREFIX, "Llamado el end frame sin un válido parallel encoder creado");
        }
        
        dispatch_semaphore_t semaphore = _frameSemaphores[_actualFrame];
    
        [_commandBuffer[_actualFrame] addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
            dispatch_semaphore_signal(semaphore);
        }];
            
        [_commandBuffer[_actualFrame] presentDrawable:_drawables[_actualFrame]];
        [_commandBuffer[_actualFrame] commit];
//        LYNX_LOG("Showing frame %i", _actualFrame);
        
        _commandBuffer[_actualFrame] = nil;
       // [_drawables[_actualFrame] release];
        _drawables[_actualFrame] = nil;
        //_currentDrawable = nil;
    
        [(__bridge NSAutoreleasePool*)_frameAutoReleasePool drain];
        _frameAutoReleasePool = nullptr;
    
        _actualFrame = (_actualFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void MetalRender::endEncoding(IGPUParallelCommandEncoder *encoder){
        if(encoder == nullptr){
            LYNX_ERROR_P(DEBUG_PREFIX, "Called to end the encoding to a null Parallel command encoder.");
        }
        
        IRenderer::FinalizeEncoding(encoder);
        
    }
        
    std::shared_ptr<IGPUBuffer> MetalRender::createBuffer(const LynxBufferDescriptor& descriptor, const void* data) {
        //Vamos a crear primero un stagging buffer y luego crear el buffer real que manejara la app
        return std::make_shared<MetalBuffer>(_device, descriptor, data, _commandQueue);
    }

    std::shared_ptr<IShader> MetalRender::createShader(const lynx::graphics::LynxShader& shaderDescriptor){
        return std::make_shared<MetalShader>(_lynxMetalLibrary, shaderDescriptor);
    }

    std::shared_ptr<IGPURenderPipeline> MetalRender::createRenderPipeline(const lynx::render::LynxRenderPipelineDescriptor &pipelineDescriptor, lynx::resources::shaders::ShaderManager *shaderManager){
        return std::make_shared<MetalRenderPipeline>(_device, pipelineDescriptor, shaderManager, actualMainDescriptor, _predefinedValues);
    }

    std::unique_ptr<IGPUParallelCommandEncoder> MetalRender::getParallelCommandEncoder(){
        auto result = acquireDrawable();
        if(result == false) return nullptr;
        
        isParallelEncoderInOtherObject = true;
        return std::move(_parallelCommandEncoder[_actualFrame]);
    }

    void MetalRender::returnParallelCommandEncoder(std::unique_ptr<IGPUParallelCommandEncoder> encoder){
        _parallelCommandEncoder[_actualFrame] = std::move(encoder);
        isParallelEncoderInOtherObject = false;
    }

    IGPUInstanceBuffer* MetalRender::getInstanceBuffer() {
        return _instanceBuffer[_actualFrame].get();
    }


    void MetalRender::generateNewMetalLibrary() {
        //Método fallback que busca todos los archivos fuente de .metal y los compila como un único .metallib
    }

//ADVERTENCIA: La lógica de esto hay que reescribirla. Porque, y si el usuario activa el postprocesado, añade 3 texturas más a las scene texture, pero ya el valor [0] tiene algo escrito, porque por algun proceso el motor requeria otro Render Pass. En ese caso, al usar el indice de los target que necesita el post procesado, no encotrara nada!!!.
void MetalRender::generateFrameTextures(){
    @autoreleasepool {
        auto width = lynx::window::IWindow::getWidth();
        auto height = lynx::window::IWindow::getHeight();
        
        bool haveTextures = _sceneTexture.size() > 0;
        bool haveDepth = _depthTexture.size() > 0;
        
        if(!(p_runtimeConfiguration->graphics.flags & LYNX_GRAPHICS_OPTIONS_USE_POSTPROCESS) && !(p_runtimeConfiguration->graphics.flags & LYNX_GRAPHICS_OPTIONS_USE_DEPTH)
           && !(p_runtimeConfiguration->graphics.flags & LYNX_GRAPHICS_OPTIONS_USE_STENCIL)){
            return;
        }
        
        if((haveTextures && !_sceneTexture[0]) || (haveTextures && _sceneTexture[0]->getTexWidth() != width && _sceneTexture[0]->getTexHeight() != height)){
            for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){ //Se calcula con respecto a los Frames in flight por seguridad. Si aquí almaceno referencia a todas las texturas que usa mi motor, usando como distancia lógica los frames in flight, por cada frame in flight, y reglas claras de como generar las texturas, debo ir creándolas
                
                int advacedSteps = 0; //Para calcular se deben ser las texturas 0 - 2, o 3 - 5, o 6 - 8
                if(p_runtimeConfiguration->graphics.flags & LYNX_GRAPHICS_OPTIONS_USE_POSTPROCESS){
                    _sceneTexture[i + (MAX_FRAMES_IN_FLIGHT * advacedSteps)].reset(); //Para forzar la liberación de cualquier textura de antes.
                    std::string label = "Pre-PostProcess Texture Data - Frame " + std::to_string(i);
                    //Generamos la textura para dibujar
                    LynxGPUTextureDescriptor descriptor{};
                    descriptor.height = height;
                    descriptor.width = width;
                    descriptor.presentation = enumerations::LYNX_GPU_TEXTURE_PRESENTATION_TYPE::LYNX_GPU_TEXTURE_PRESENTATION_NOTPRESENT;
                    descriptor.pixelFormat = enumerations::LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_SCREEN;
                    descriptor.usage = LYNX_GPU_TEXTURE_USAGE_RENDERTARGET | LYNX_GPU_TEXTURE_USAGE_SHADER_READ;
                    descriptor.memoryProperty = enumerations::LYNX_GPU_MEMORY_PROPERTY_TYPE::LYNX_GPU_MEMORY_PROPERTY_PRIVATE;
                    descriptor.label = label;
                    
                    _sceneTexture[i + (MAX_FRAMES_IN_FLIGHT * advacedSteps)] = createNewPlatformTexture(descriptor);
                    advacedSteps++;
                    
                    //Post process effect requerided images!!!
                    if(p_runtimeConfiguration->graphics.flags & LYNX_GRAPHICS_OPTIONS_USE_MOTION_BLUR){
                        //Generate the motion texture
                        _sceneTexture[i + (MAX_FRAMES_IN_FLIGHT * advacedSteps)].reset(); //Para forzar la liberación de cualquier textura de antes.

                        label = "Motion Texture for Post Process - Frame " + std::to_string(i);
                        LynxGPUTextureDescriptor descriptor{};
                        descriptor.height = height;
                        descriptor.width = width;
                        descriptor.presentation = enumerations::LYNX_GPU_TEXTURE_PRESENTATION_TYPE::LYNX_GPU_TEXTURE_PRESENTATION_NOTPRESENT;
                        //descriptor.pixelFormat = enumerations::LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_FLOAT2_HALF;
                        descriptor.pixelFormat = enumerations::LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_RG16_FLOAT;
                        descriptor.usage = LYNX_GPU_TEXTURE_USAGE_RENDERTARGET | LYNX_GPU_TEXTURE_USAGE_SHADER_READ;
                        descriptor.memoryProperty = enumerations::LYNX_GPU_MEMORY_PROPERTY_TYPE::LYNX_GPU_MEMORY_PROPERTY_PRIVATE;
                        descriptor.label = label;
                        
                        _sceneTexture[i + (MAX_FRAMES_IN_FLIGHT * advacedSteps)] = createNewPlatformTexture(descriptor);
                    }
                }else{
                    _sceneTexture[i + (MAX_FRAMES_IN_FLIGHT * advacedSteps)].reset();
                }
            }
        }
        
        if((haveDepth && !_depthTexture[0]) || (haveDepth && _depthTexture[0]->getTexWidth() != width && _depthTexture[0]->getTexHeight() != height)){
            for(size_t i = 0; i < _depthTexture.size(); i++){
                if(p_runtimeConfiguration->graphics.flags & LYNX_GRAPHICS_OPTIONS_USE_DEPTH){
                    std::string label = "Depth texture - Frame " + std::to_string(i);

                    LynxGPUTextureDescriptor descriptor{};
                    descriptor.height = height;
                    descriptor.width = width;
                    descriptor.presentation = enumerations::LYNX_GPU_TEXTURE_PRESENTATION_TYPE::LYNX_GPU_TEXTURE_PRESENTATION_NOTPRESENT;
                    descriptor.pixelFormat = enumerations::LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_DEPTH_32_FLOAT;
                    descriptor.usage = LYNX_GPU_TEXTURE_USAGE_RENDERTARGET;
                    descriptor.memoryProperty = enumerations::LYNX_GPU_MEMORY_PROPERTY_TYPE::LYNX_GPU_MEMORY_PROPERTY_PRIVATE;
                    descriptor.label = label;
                    
                    _depthTexture[i] = createNewPlatformTexture(descriptor);
                }else{
                    _depthTexture[i].reset();
                }
            }
        }

    }
}

IGPUTexture* MetalRender::getDrawableTexturePtr(){
    return _drawableTexture[_actualFrame].get();
}

std::vector<IGPUTexture*> MetalRender::getAllSceneTexturesPtr() {
    std::vector<IGPUTexture*> toReturn; //Vector a regresar
    size_t differentTextures = _sceneTexture.size() / MAX_FRAMES_IN_FLIGHT; //Numero de texturas diferentes
    toReturn.resize(differentTextures); //Optimización
    
    //Por cada textura diferente, mandamos al del frame actual
    for(size_t i = 0; i < differentTextures; i++){
        toReturn[i] = _sceneTexture[_actualFrame + (i * MAX_FRAMES_IN_FLIGHT)].get();
    }
    
    return toReturn;
}


std::unique_ptr<IGPUParallelCommandEncoder> MetalRender::createNewParallelCommandEncoder(LynxParallelEncoderDescriptor&& descriptor){
    //Parte de su responsabilidad es asignar las texturas para el nuevo Parallel Encoder Descriptor.
        //CAUTION: Cuidar todos los objetos que no retornan nada, porque pueden hacer crashear el programa de la nada, como al usar esto.
    return std::make_unique<MetalParallelCommandEncoder>(_commandBuffer[_actualFrame], std::move(descriptor));
}

std::unique_ptr<IGPUTexture> MetalRender::createNewPlatformTexture(const LynxGPUTextureDescriptor &descriptor){
    return std::make_unique<MetalTexture>(descriptor, _predefinedValues, _device);
}

std::shared_ptr<IGPUSampler> MetalRender::createSampler(const LynxGPUSamplerDescriptor &descriptor){
    return MetalSampler::getOrCreate(descriptor, _device);
}

std::shared_ptr<IGPUTexture> MetalRender::createTexture(const LynxGPUTextureDescriptor &descriptor, const void* initialData){
    return std::make_shared<MetalTexture>(descriptor, _predefinedValues, _device, initialData, _commandQueue);
}

void MetalRender::regenerateEncoderDescriptor(){
    LynxParallelEncoderDescriptor parallelEncoderDescriptor {};
    parallelEncoderDescriptor.label = "Main Render Pass Encoder";
    parallelEncoderDescriptor.colorAttachments.resize(3); //Metal soporta hasta 8, pero creo que de momento 3 son más que suficientes.
    auto colorAttach = std::make_unique<LynxEncoderAttachmentDescriptor>();

    if(p_runtimeConfiguration->graphics.flags & LYNX_GRAPHICS_OPTIONS_USE_POSTPROCESS){
        colorAttach->texture = _sceneTexture[_actualFrame].get();
        uint32_t actualIndexForNextTexture = MAX_FRAMES_IN_FLIGHT;
        if(p_runtimeConfiguration->graphics.flags & LYNX_GRAPHICS_OPTIONS_USE_MOTION_BLUR){
            //TODO: asignar la textura para el velocity buffer.
            auto motionBlurAttach = std::make_unique<LynxEncoderAttachmentDescriptor>();
            motionBlurAttach->texture = _sceneTexture[_actualFrame + actualIndexForNextTexture].get();
            motionBlurAttach->loadOption = LYNX_GPU_RENDER_PASS_LOAD_OPTION_TYPE::LYNX_GPU_RENDER_PASS_LOAD_OPTION_CLEAR;
            motionBlurAttach->storeOption = LYNX_GPU_RENDER_PASS_STORE_OPTION_TYPE::LYNX_GPU_RENDER_PASS_STORE_OPTION_STORE;
            //Porque este encoder no tiene asignado el attachment?? Luego así vienen los errores random.
            parallelEncoderDescriptor.colorAttachments[lynx::render::slots::RenderPassAttachment_VelocityBuffer] = std::move(motionBlurAttach); // Slot 1 para el Velocity Buffer
            actualIndexForNextTexture += 3; // Se añade un stride de 3 para los indices de alguna nueva textura especial a añadir.
            //motionBlurAttach->texture
        }
    }else{
        colorAttach->texture = _drawableTexture[_actualFrame].get();
    }
    colorAttach->loadOption = LYNX_GPU_RENDER_PASS_LOAD_OPTION_TYPE::LYNX_GPU_RENDER_PASS_LOAD_OPTION_CLEAR;
    colorAttach->storeOption = LYNX_GPU_RENDER_PASS_STORE_OPTION_TYPE::LYNX_GPU_RENDER_PASS_STORE_OPTION_STORE;
    colorAttach->clearColor = graphics::Color(_predefinedValues.clearColor.red, _predefinedValues.clearColor.green, _predefinedValues.clearColor.blue);
    parallelEncoderDescriptor.colorAttachments[lynx::render::slots::RenderPassAttachment_Color] = std::move(colorAttach);

    if(p_runtimeConfiguration->graphics.flags & LYNX_GRAPHICS_OPTIONS_USE_DEPTH){
        //Depth buffer
        auto depthAttach = std::make_unique<LynxEncoderAttachmentDescriptor>();
        depthAttach->texture = _depthTexture[_actualFrame].get();
        depthAttach->loadOption = LYNX_GPU_RENDER_PASS_LOAD_OPTION_TYPE::LYNX_GPU_RENDER_PASS_LOAD_OPTION_CLEAR;
        depthAttach->storeOption = LYNX_GPU_RENDER_PASS_STORE_OPTION_TYPE::LYNX_GPU_RENDER_PASS_STORE_OPTION_DONT_CARE;
        
        parallelEncoderDescriptor.depthAttachment = std::move(depthAttach);
    }
    
    actualMainDescriptor = parallelEncoderDescriptor;
    lastUsedGraphicsOptions = p_runtimeConfiguration->graphics.flags;
}

std::unique_ptr<IGPUInstanceBuffer> MetalRender::createInstanceBuffer(size_t bufferSize, std::string bufferName){
    return std::make_unique<MetalInstanceBuffer>(_device, bufferSize, bufferName);
}

    /*
    Renderer* Renderer::create(void* native_surface){
        return new MetalRender(native_surface);
    }*/


void MetalRender::defineSupportedGPUCompression() {
#if LYNX_MACOS || TARGET_OS_SIMULATOR
    if([_device supportsBCTextureCompression]){
        compressionActualFormat = enumerations::LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_BC7_UNORM;
        return;
    }
#endif
    if([_device supportsFamily:MTLGPUFamilyApple2]){
        compressionActualFormat = enumerations::LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_ASTC_4X4_UNORM;
        return;
    }
    else {
        compressionActualFormat = enumerations::LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_INVALID;
    }
}

}


#endif // defined (__APPLE__)
