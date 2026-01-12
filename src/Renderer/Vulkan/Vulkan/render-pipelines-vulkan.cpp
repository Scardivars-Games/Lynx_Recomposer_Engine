#if ILEN_LINUX || ILEN_WINDOWS || ILEN_ANDROID
#include <Vulkan/render-pipelines-vulkan.h>

using namespace ilen::render;

std::map<VertexBufferLayout, std::unique_ptr<VulkanVertexInputResources>> VulkanRenderPipeline::_vertexBufferDescriptors;
std::map<VulkanBindingKey, vk::DescriptorSetLayoutBinding> VulkanRenderPipeline::_layoutBindings;
std::map<VulkanDescriptorSetKey, vk::DescriptorSetLayout> VulkanRenderPipeline::_descriptorSetLayouts;
std::map<VulkanPipelineLayoutkey, vk::PipelineLayout> VulkanRenderPipeline::_pipelineLayouts;
std::mutex VulkanRenderPipeline::_psoMutex;

VulkanRenderPipeline::VulkanRenderPipeline(vk::raii::Device& device, const IlenRenderPipelineDescriptor& descriptor, resources::shaders::ShaderManager* shaderManager, vk::Extent2D swapchainExtent, IlenParallelEncoderDescriptor inUseGeneralEncoder, vk::raii::RenderPass& renderPass) {
    //Shadersf
    auto vertexPtr = shaderManager->getShaderData(descriptor.vertexShaderID).lock();
    auto fragmentPtr = shaderManager->getShaderData(descriptor.fragmentShaderID).lock();
    
    if(!vertexPtr || !fragmentPtr){
        ILEN_ERROR_P(DEBUG_PREFIX, "No fue posible obtener los shader con ID %i(vertex) y %i(fragment). Nombre de la Pipeline: %s", descriptor.vertexShaderID, descriptor.fragmentShaderID, descriptor.pipelineName.c_str());
        return;
    }
    
    const vk::PipelineShaderStageCreateInfo* vertexFunction;
    const vk::PipelineShaderStageCreateInfo* fragmentFunction;

    if(descriptor.constantValuesForPipelines.size() == 0){
        vertexFunction = static_cast<const vk::PipelineShaderStageCreateInfo*>(vertexPtr.get()->getNativeShaderFromFunctionName(descriptor.vertexFunctionName));
        fragmentFunction = static_cast<const vk::PipelineShaderStageCreateInfo*>(fragmentPtr.get()->getNativeShaderFromFunctionName(descriptor.fragmentFunctionName));
    }/*else{
        vertexFunction = (__bridge id<MTLFunction>)vertexPtr.get()->getNativeShaderFromFunctionNameAndUberActivations(descriptor.vertexFunctionName, descriptor.constantValuesForPipelines);
        fragmentFunction = (__bridge id<MTLFunction>)fragmentPtr.get()->getNativeShaderFromFunctionNameAndUberActivations(descriptor.fragmentFunctionName, descriptor.constantValuesForPipelines);
    }*/

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = {
        *vertexFunction,
        *fragmentFunction,
    };
    
    if(!vertexFunction || !fragmentFunction){
        ILEN_ERROR_P(DEBUG_PREFIX, "Las funciones %s y %s no fueron encontradas en su respectivo shader!!!.", descriptor.vertexFunctionName.c_str(), descriptor.fragmentFunctionName.c_str());
        return;
    }
    
    
    //Estructura de los vertices del PSO
    VertexBufferLayout layout;
    switch (descriptor.vertexLayout) {
        case ILEN_VERTEX_LAYOUT_TYPE::ILEN_VERTEX_LAYOUT_GLOBAL:
            layout = ILEN_GLOBAL_VERTEX_LAYOUT;
            break;
        default:
            layout = ILEN_GLOBAL_VERTEX_LAYOUT;
            break;;
    }
    auto vertexInputInfo = static_cast<vk::PipelineVertexInputStateCreateInfo*>(generateVertexDescriptor(layout));

    //Topología del PSO
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;

    //Viewport del PSO (que region del framebuffer debe escribir el PSO, mapeando el resultado. En otras palabras, transforma)
    vk::Viewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchainExtent.width);
    viewport.height = static_cast<float>(swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    //Scissor del PSO (indica que área del framebuffer el PSO puede dibujar, tipo una máscara)
    vk::Rect2D scissor{};
    scissor.offset = vk::Offset2D {0 ,0};
    scissor.extent = swapchainExtent;

    //Ahora sí, la estrucutra que une lo anterior
    vk::PipelineViewportStateCreateInfo viewportState = {};
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    //Rasterizer
    vk::PipelineRasterizationStateCreateInfo rasterization = {};
    rasterization.depthClampEnable = false;
    rasterization.rasterizerDiscardEnable = false; //Si esto es true, detiene el proceso al llegar a la etapa de rasterizado
    rasterization.polygonMode = vk::PolygonMode::eFill;
    rasterization.cullMode = vk::CullModeFlagBits::eBack;
    rasterization.frontFace = vk::FrontFace::eClockwise;
    rasterization.depthBiasEnable = false;
    rasterization.depthBiasSlopeFactor = 1.0f;
    rasterization.lineWidth = 1.0;

    //Multisampling
    vk::PipelineMultisampleStateCreateInfo multisampling{};
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
    multisampling.sampleShadingEnable = false;

    //Color blending
    //TODO unificar esto agnósticamente para no reescribir lo mismo en cada api, y evitar que se cambie en un lado y aquí no.
    vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
    switch (descriptor.renderQueue) {
        case enumerations::ILEN_GPU_RENDER_QUEUE_TYPE::ILEN_GPU_RENDER_QUEUE_OPAQUE:
        case enumerations::ILEN_GPU_RENDER_QUEUE_TYPE::ILEN_GPU_RENDER_QUEUE_ALPHA_TEST:
            colorBlendAttachment.blendEnable = false;
            break;
        case enumerations::ILEN_GPU_RENDER_QUEUE_TYPE::ILEN_GPU_RENDER_QUEUE_TRANSPARENT:
        case enumerations::ILEN_GPU_RENDER_QUEUE_TYPE::ILEN_GPU_RENDER_QUEUE_OVERLAY:
            colorBlendAttachment.blendEnable = true;
            colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
            colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
            colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
            colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
            colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
            colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
            break;
        default:
            break;
    }

    //Color operations para el resto de attachments
    auto encoderDesc = descriptor.encoderDescriptor;
    if(descriptor.encoderDescriptor == nullptr){
        encoderDesc = &inUseGeneralEncoder;
        //ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "Solicitud de creación de un PSO especial sin el descriptor con los datos del encoder.");
    }

    vk::PipelineDepthStencilStateCreateInfo depthStencilCreateInfo{};
    //TODO: Adaptar esto de Metal para Vulkan. Depth and Stencil Attachments
    /*
        if(ilen::core::options::GraphicsOptions::graphicsOptionsFlags & ILEN_GRAPHICS_OPTIONS_USE_DEPTH){
            pipelineDescriptor.depthAttachmentPixelFormat = view.depthStencilPixelFormat;
        }else{
            pipelineDescriptor.depthAttachmentPixelFormat = MTLPixelFormatInvalid;
        }

            if(ilen::core::options::GraphicsOptions::graphicsOptionsFlags & ILEN_GRAPHICS_OPTIONS_USE_STENCIL){
                pipelineDescriptor.stencilAttachmentPixelFormat = view.depthStencilPixelFormat;
            }else{
                pipelineDescriptor.stencilAttachmentPixelFormat = MTLPixelFormatInvalid;
            }*/
            /* En METAL siempre hay que definir los descriptores, auque el PSO no lo use.
            if(descriptor.isSpecialPipeline){

            }*/

        if(descriptor.isSpecialPipeline) {
            //Special Pipelines son aquellos que no escriben en el buffer 0 (dibujado normal) Esto es principalmente para PSO de Post-Procesado
            colorBlendAttachment.colorWriteMask = {}; //No escribe en ningún canal
        }else {
            colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                    vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        }

    std::vector<vk::PipelineColorBlendAttachmentState> allPSOAttachments;
    allPSOAttachments.resize(encoderDesc->colorAttachments.size());
    allPSOAttachments[0] = colorBlendAttachment;

    //Empezamos en 1 porque el color 0 siempre esta reservada para el dibujo de la pantalla, y para nada mas :)
    for(size_t i = 1; i < encoderDesc->colorAttachments.size(); i++){
        if(encoderDesc->colorAttachments[i] != nullptr){
           //auto texFormat =  encoderDesc->colorAttachments[i]->texture->getDescriptor().pixelFormat;
            //allPSOAttachments[i].pixelFormat = transformEngineToNativePixelFormat(texFormat, view);
            allPSOAttachments[i].colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;;
            allPSOAttachments[i].blendEnable = false;
        }
    }

    //El que gobierna todos los attach
    vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo = {};
    pipelineColorBlendStateCreateInfo.logicOpEnable = false;
    pipelineColorBlendStateCreateInfo.logicOp = vk::LogicOp::eCopy;
    pipelineColorBlendStateCreateInfo.attachmentCount = allPSOAttachments.size();
    pipelineColorBlendStateCreateInfo.pAttachments = allPSOAttachments.data();

    _psoDescriptor = descriptor;

    auto pipelineLayout = getCachedPipelineLayout();

    //Creando ahora si el PSO
    vk::GraphicsPipelineCreateInfo createInfo{};
    createInfo.stageCount = shaderStages.size();
    createInfo.pStages = shaderStages.data();
    createInfo.pVertexInputState = vertexInputInfo;
    createInfo.pInputAssemblyState = &inputAssembly;
    createInfo.pViewportState = &viewportState;
    createInfo.pRasterizationState = &rasterization;
    createInfo.pMultisampleState = &multisampling;
    createInfo.pDepthStencilState = &depthStencilCreateInfo;
    createInfo.pColorBlendState = &pipelineColorBlendStateCreateInfo;
    createInfo.layout = pipelineLayout;
    createInfo.renderPass = renderPass;
    createInfo.subpass = 0;
    createInfo.basePipelineHandle = nullptr;
    createInfo.basePipelineIndex = -1;

    _pso = vk::raii::Pipeline(device, nullptr, createInfo);
}

void* IGPURenderPipeline::generateVertexDescriptor(VertexBufferLayout layout) {
    std::lock_guard lock(VulkanRenderPipeline::_psoMutex);
    //Buscamos si esta cacheado el layout
    auto it = VulkanRenderPipeline::_vertexBufferDescriptors.find(layout);

    if (it != VulkanRenderPipeline::_vertexBufferDescriptors.end()) {
        return static_cast<void*>(&VulkanRenderPipeline::_vertexBufferDescriptors[layout]->createInfo);
    }

    //Si no esta cacheado, generamos un nuevo cache con la struct.
    auto resource = std::make_unique<VulkanVertexInputResources>();

    vk::VertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0; //El motor solo maneja un binding, así que no tiene sentido esto
    bindingDescription.stride = layout.stride;
    bindingDescription.inputRate = vk::VertexInputRate::eVertex;

    resource->bindingDescriptions.push_back(bindingDescription);

    //std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
    //attributeDescriptions.resize(layout.attributes.size());

    for (int i = 0; i < layout.attributes.size(); i++) {
        auto actualAttribute = layout.attributes[i];
        vk::VertexInputAttributeDescription attributeDescription = {};

        auto format = VulkanRenderPipeline::getVertexFormat(actualAttribute);
        attributeDescription.binding = 0;
        attributeDescription.location = layout.attributeIndex[i];
        attributeDescription.format = format;
        attributeDescription.offset = layout.offsets[i];

        resource->attributeDescriptions.push_back(attributeDescription);
    }

    resource->sync();

    void* nativePtr = static_cast<void*>(&resource->createInfo);
    VulkanRenderPipeline::_vertexBufferDescriptors[layout] = std::move(resource);

    return nativePtr;
}

vk::PipelineLayout VulkanRenderPipeline::getCachedPipelineLayout() {
    //Buscamos todos los bindings de los uniforms globales del motor.
    std::lock_guard lock(_psoMutex);
    auto listOfGlobalUniforms = _psoDescriptor.psoGlobalUniformIdentifiers;

    if (!listOfGlobalUniforms.empty() && listOfGlobalUniforms[0] == structs::NOT_DEFINED) {
        ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "Pasado descriptor de PSO sin definir que Uniforms Globales deben vincularse. En caso de que no se requieran, limpie este std::vector");
    }

    auto globalListMetadata = structs::getGlobalShaderStructs();
    std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings;

    for (auto idToAdd : listOfGlobalUniforms) {
        for (auto metadata : globalListMetadata) {
            if (metadata.identifier == idToAdd) {
                VulkanBindingKey key{};
                key.binding = metadata.bindingSlot;
                key.count = 1;
                key.descriptorType = getDescriptorType(metadata.type);
                key.stages = vk::ShaderStageFlagBits::eAllGraphics;

                //Obteniendo version cacheada
                auto it = _layoutBindings.find(key);
                if (it != _layoutBindings.end()) {
                    descriptorSetLayoutBindings.push_back(it->second);
                    break;
                }

                //No existe, cacheando.
                vk::DescriptorSetLayoutBinding binding{};
                binding.binding = key.binding;
                binding.descriptorType = key.descriptorType;
                binding.descriptorCount = key.count;
                binding.stageFlags = key.stages;

                _layoutBindings[key] = binding;
                descriptorSetLayoutBindings.push_back(_layoutBindings[key]);
                break;
            }
        }
    }

    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
    VulkanDescriptorSetKey setKey{};

    //TODO: Al implementar una escena con varios PSO, revisar que esta estructura realmente este funcionando.

    setKey.bindings = descriptorSetLayoutBindings;

    auto it = _descriptorSetLayouts.find(setKey);
    if (it != _descriptorSetLayouts.end()) {
        descriptorSetLayouts.push_back(it->second);
    }else {
        vk::DescriptorSetLayoutCreateInfo createInfo{};
        createInfo.bindingCount = setKey.bindings.size();
        createInfo.pBindings = setKey.bindings.data();

        auto newDescriptorSetLayout = device->createDescriptorSetLayout(createInfo);
        _descriptorSetLayouts[setKey] = newDescriptorSetLayout;
        descriptorSetLayouts.push_back(_descriptorSetLayouts[setKey]);
    }


    vk::PipelineLayout pipelineLayout;
    VulkanPipelineLayoutkey layoutKey{};

    layoutKey.setLayouts = descriptorSetLayouts;
    layoutKey.pushConstants.clear();

    auto itLayoutKeys = _pipelineLayouts.find(layoutKey);
    if (itLayoutKeys != _pipelineLayouts.end()) {
        pipelineLayout = itLayoutKeys->second;
    }else {
        vk::PipelineLayoutCreateInfo createInfo{};
        createInfo.setLayoutCount = descriptorSetLayouts.size();
        createInfo.pSetLayouts = descriptorSetLayouts.data();
        createInfo.pushConstantRangeCount = 0;

        auto final = device->createPipelineLayout(createInfo);
        _pipelineLayouts[layoutKey] = final;
        pipelineLayout = _pipelineLayouts[layoutKey];
    }

    return pipelineLayout;
}

const void* VulkanRenderPipeline::getNativeRenderPipeline() const {
    return *_pso;
}

size_t VulkanRenderPipeline::getFormatSize(ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE format){
    switch(format){
        case ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE::ILEN_GPU_VERTEX_BUFFER_FORMAT_INT: return sizeof(int);
        case ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE::ILEN_GPU_VERTEX_BUFFER_FORMAT_FLOAT: return sizeof(float);
        case ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE::ILEN_GPU_VERTEX_BUFFER_FORMAT_FLOAT2: return sizeof(float) * 2;
        case ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE::ILEN_GPU_VERTEX_BUFFER_FORMAT_FLOAT3: return sizeof(float) * 3;
        case ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE::ILEN_GPU_VERTEX_BUFFER_FORMAT_FLOAT4: return sizeof(float) * 4;
        default: return 0;
    }
}

vk::Format VulkanRenderPipeline::getVertexFormat(ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE format){
    switch(format){
        case ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE::ILEN_GPU_VERTEX_BUFFER_FORMAT_INT: return vk::Format::eR32Sint;
        case ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE::ILEN_GPU_VERTEX_BUFFER_FORMAT_FLOAT: return vk::Format::eR32Sfloat;
        case ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE::ILEN_GPU_VERTEX_BUFFER_FORMAT_FLOAT2: return vk::Format::eR32G32Sfloat;
        case ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE::ILEN_GPU_VERTEX_BUFFER_FORMAT_FLOAT3: return vk::Format::eR32G32B32Sfloat;
        case ILEN_GPU_VERTEX_BUFFER_FORMATS_TYPE::ILEN_GPU_VERTEX_BUFFER_FORMAT_FLOAT4: return vk::Format::eR32G32B32A32Sfloat;
        default: return vk::Format::eUndefined;
    }
}

vk::DescriptorType VulkanRenderPipeline::getDescriptorType(structs::ILEN_VULKAN_DESCRIPTOR_TYPE type) {
    switch (type) {
        case structs::ILEN_VULKAN_DESCRIPTOR_STORAGE_BUFFER:
            return vk::DescriptorType::eStorageBuffer;
            break;
        case structs::ILEN_VULKAN_DESCRIPTOR_UNIFORM_BUFFER:
            return vk::DescriptorType::eUniformBuffer;
            break;
        default:
            return vk::DescriptorType::eUniformBuffer;
            break;
    }
}

#endif
