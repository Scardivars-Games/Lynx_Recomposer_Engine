#if ILEN_LINUX || ILEN_WINDOWS || ILEN_ANDROID
#define VK_USE_PLATFORM_XCB_KHR
#include <Vulkan/renderer_vulkan.h>
#include <algorithm>
#include <cstring>

#if ILEN_LINUX
#include <vulkan/vulkan_xcb.h> //Adicionar al TODO de las instancias.
#endif
using namespace ilen::render;

std::vector<const char*> instanceExtensions = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    //TODO: Al usar Wayland, comprobar cual extension en especifico es la necesaria.
#if ILEN_LINUX
    VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#endif
};

//Validation layer code
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};


       //Seleccionando dispositivo según capacidad.
std::vector<const char*> deviceExtension = {
        vk::KHRSwapchainExtensionName, //Obligatorio
        vk::KHRSpirv14ExtensionName, //De momento, también
};

std::vector<const char*> optionalDeviceExtensions {
        vk::KHRSynchronization2ExtensionName,
        vk::KHRCreateRenderpass2ExtensionName
};

VulkanRender::VulkanRender(const void *native_surface) {
#if ILEN_DEBUG
    enableValidationLayers = true;
    ILEN_WARNING_P(DEBUG_PREFIX, "Validation layers for Vulkan enabled.");
#endif

    createVulkanInstance();

#if ILEN_DEBUG
    setVulkanDebugCalls();
#endif
    createVulkanSurface(native_surface);
    selectVulkanPhysicalDevice();
    createVulkanLogicalDevice();
    createSwapchain();
    createVulkanImageViews();
    ILEN_LOG_P(DEBUG_PREFIX, "Inicialización de Vulkan completada de forma exitosa.");
}

void VulkanRender::createVulkanInstance(){
    lastUsedGraphicsOptions = ilen::core::options::GraphicsOptions::graphicsOptionsFlags;

    //Obtenemos la version de Vulkan que posee el usuario
    auto vkEnumerateInstanceVersion = (PFN_vkEnumerateInstanceVersion)vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion");  //En caso de ser Vulkan 1.0, cargamos manualmente este dato, para evitar que el loader no lo conozca.

    if(vkEnumerateInstanceVersion){
        vkEnumerateInstanceVersion(&vulkanTargetVersion);
    }else{
        vulkanTargetVersion = vk::ApiVersion10; //Por seguridad
    }

    if(vulkanTargetVersion >= vk::ApiVersion14){
        vulkanTargetVersion = vk::ApiVersion14;
    }else if(vulkanTargetVersion >= vk::ApiVersion13){
        vulkanTargetVersion = vk::ApiVersion13;
    }else if(vulkanTargetVersion >= vk::ApiVersion12){
        vulkanTargetVersion = vk::ApiVersion12;
    }else if(vulkanTargetVersion >= vk::ApiVersion11){
        vulkanTargetVersion = vk::ApiVersion11;
    }else{
        vulkanTargetVersion = vk::ApiVersion10; //
    }

    ILEN_LOG_P(DEBUG_PREFIX, "Vulkan Version %i", vulkanTargetVersion);

           //El objetivo de esto es generar hasta el dispositivo logico de la app, o sea, muchas cosas
           //Creemos los datos de la instancia
    vk::ApplicationInfo appInfo {
            .pApplicationName = application::Application::getApplicationName().c_str(),
            .applicationVersion = VK_MAKE_VERSION(application::Application::getMajorVersion(), application::Application::getMinorVersion(), application::Application::getPatchVersion()),
            .pEngineName = "Ilusion's Engine",
            .engineVersion =  VK_MAKE_VERSION(1,0,0),
            .apiVersion =  vulkanTargetVersion,
    };

           //Validation layers if required
    std::vector<const char*> requiredLayers;
    if (enableValidationLayers) {
        instanceExtensions.push_back(vk::EXTDebugUtilsExtensionName);
        requiredLayers.assign(validationLayers.begin(), validationLayers.end());
    }

    auto extensionPropperties = context.enumerateInstanceExtensionProperties();
    for (size_t i = 0; i < instanceExtensions.size(); i++) {
        if (std::ranges::none_of(extensionPropperties, [ext = instanceExtensions[i]](auto const& extensionProp) {
                return strcmp(extensionProp.extensionName, ext) == 0;
            })) {
            ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "Error al inicializar instancia de Vulkan. Extensión no soportada: %s", instanceExtensions[i]);
        }
    }

    auto layerProperties = context.enumerateInstanceLayerProperties();
    if (std::ranges::any_of(requiredLayers,[&layerProperties](auto const& requiredLayer) {
            return std::ranges::none_of(layerProperties, [requiredLayer](auto const& layerProp) {
                return strcmp(layerProp.layerName, requiredLayer) == 0;
            });
        })) {
        ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "No se encontraron las capas de validación necesarias para ejecución");
    }

    vk::InstanceCreateInfo instanceCreateInfo {
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
            .ppEnabledLayerNames = requiredLayers.data(),
            .enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()),
            .ppEnabledExtensionNames = instanceExtensions.data(),
    };

    instance = vk::raii::Instance(context, instanceCreateInfo);

    ILEN_LOG_P(DEBUG_PREFIX, "Creación de la instancia de Vulkan correcta");
}

void VulkanRender::createVulkanSurface(const void* native_surface){
    //Genración de la superficie para dibujar.
#if ILEN_LINUX
    //Si estamos en XCB
    auto xcbSurfaceData = static_cast<const ilen::window::XCBSurfaceCreationInfo*>(native_surface);
    VkXcbSurfaceCreateInfoKHR surfaceCreateInfo{};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.connection = xcbSurfaceData->_connection;
    surfaceCreateInfo.flags = 0;
    surfaceCreateInfo.window = xcbSurfaceData->_window;

           //Creando superficie XCB
    VkSurfaceKHR _surface;
    if(vkCreateXcbSurfaceKHR(*instance, &surfaceCreateInfo, nullptr, &_surface) != VK_SUCCESS){
        ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "Error al crear la superficie desde XCB en Linux");
    }

    surface = vk::raii::SurfaceKHR(instance, _surface);
#endif
}

void VulkanRender::selectVulkanPhysicalDevice(){
    //Selección del dispositivo físico y creción del dispositivo lógico.
    auto devices = instance.enumeratePhysicalDevices();
    if (devices.empty()) {
        ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "No existe un dispositivo físico compatible con Vulkan.");
    }

    std::multimap<int, vk::raii::PhysicalDevice> candidates;
    for (const auto& device : devices) {
        auto deviceProperties = device.getProperties();
        auto deviceFeatures = device.getFeatures();
        uint32_t score = 0;

               //Preferir gráficas externas
        if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            score += 1000;
        }

        score += deviceProperties.limits.maxImageDimension2D;

        if (!deviceFeatures.geometryShader) {
            continue; //Si no renderiza no tiene sentido en ese pedazo.
        }

        candidates.insert(std::make_pair(score, device));
    }

           //Comprobamos que nuestras GPU candidates sean viables para el su uso en el motor.
    for (const auto& device: candidates) {
        auto queueFamilies = device.second.getQueueFamilyProperties();
        const auto queueFilter = std::ranges::find_if(queueFamilies, [](vk::QueueFamilyProperties const& qfp) {
            return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0);
        });

        bool isUtilGPU = (queueFilter != queueFamilies.end());

        auto extensions = device.second.enumerateDeviceExtensionProperties();
        bool founded = true;

        for(auto const& extension: deviceExtension){
            auto extensionIter = std::ranges::find_if(extensions, [extension](auto const& ext){
                return strcmp(ext.extensionName, extension) == 0;
            });
            founded = founded && extensionIter != extensions.end();
        }

        isUtilGPU = isUtilGPU && founded;

        //Comprobando extensiones opcionales
        //vk::KHRSynchronization2ExtensionName,
        //vk::KHRCreateRenderpass2ExtensionName
        
        for(auto const& extension: optionalDeviceExtensions){
            auto extensionIter = std::ranges::find_if(extensions, [extension](auto const& ext){
                return strcmp(ext.extensionName, extension) == 0;
            });
            if(extensionIter != extensions.end()){
                if(extension == vk::KHRCreateRenderpass2ExtensionName){
                    availableOptionalFeatures | VK_FEATURE_RENDER_PASS2;
                    deviceExtension.push_back(vk::KHRCreateRenderpass2ExtensionName);
                }else if(extension == vk::KHRSynchronization2ExtensionName){
                    availableOptionalFeatures | VK_FEATURE_SYNC2;
                    deviceExtension.push_back(vk::KHRSynchronization2ExtensionName);
                }
            }
        }

        if (isUtilGPU) {
            physicalDevice = device.second;
            break;
        }
    }

    if (physicalDevice ==nullptr) {
        ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "Fallo la obtención del dispositivo físico válido para el motor");
    }
}

void VulkanRender::setVulkanQueueFamilies(){
    //Determinar familias válidas para el uso de Vulkan.
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

    auto graphicsQueueFamilyProperty = std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(),
                                                    [](vk::QueueFamilyProperties const& qfp){
                                                        return qfp.queueFlags & vk::QueueFlagBits::eGraphics;
                                                    });

    auto graphicsIndex = static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyProperty));

    auto presentIndex = physicalDevice.getSurfaceSupportKHR(graphicsIndex, *surface) ? graphicsIndex : UINT32_MAX;
    
    uint32_t transfernIndex = -1;
    for(size_t i = 0; i < queueFamilyProperties.size(); i++){
        auto flags = queueFamilyProperties[i].queueFlags;
        if(flags & vk::QueueFlagBits::eTransfer){
            transfernIndex = i;
            break;
        }
    }

    if(presentIndex == UINT32_MAX){
        //Graphics queue no soporta presentación
        for(size_t i = 0; i < queueFamilyProperties.size(); i++){
            if((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) && physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), *surface)){
                graphicsIndex = static_cast<uint32_t>(i);
                presentIndex = graphicsIndex;
                break;
            }
        }

        if(presentIndex == UINT32_MAX){
            for(size_t i = 0; i < queueFamilyProperties.size(); i++){
                if(physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), *surface)){
                    presentIndex = static_cast<uint32_t>(i);
                    break;
                }
            }
        }
    }

    graphicsQueueFamily = graphicsIndex;
    presentQueueFamily = presentIndex;
    transferQueueFamily = transfernIndex;
}

void VulkanRender::createVulkanLogicalDevice(){
    setVulkanQueueFamilies(); //Determinamos las familias

    if(graphicsQueueFamily == UINT32_MAX && presentQueueFamily == UINT32_MAX){
        ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "Es imposible definir familias de queue validas para el motor");
    }

    //Información de las Queues a crearen cada familia para la creación del dispositivo lógico.
    float queuePriority = 0.5f;
    std::vector<vk::DeviceQueueCreateInfo> deviceQueueCreateInfo;

    if((graphicsQueueFamily == presentQueueFamily) && (graphicsQueueFamily == transferQueueFamily)){
        vk::DeviceQueueCreateInfo graphicsQueueCreateInfo{
                .queueFamilyIndex = graphicsQueueFamily,
                .queueCount = 1,
                .pQueuePriorities = &queuePriority,
        };
        deviceQueueCreateInfo = { graphicsQueueCreateInfo };
    }else{
        vk::DeviceQueueCreateInfo graphicsQueueCreateInfo{
                .queueFamilyIndex = graphicsQueueFamily,
                .queueCount = 1,
                .pQueuePriorities = &queuePriority,
        };

        vk::DeviceQueueCreateInfo presentQueueCreateInfo{
                .queueFamilyIndex = presentQueueFamily,
                .queueCount = 1,
                .pQueuePriorities = &queuePriority,
        };

        vk::DeviceQueueCreateInfo transferQueueCreateInfo{
                .queueFamilyIndex = transferQueueFamily,
                .queueCount = 1,
                .pQueuePriorities = &queuePriority,
        };

        deviceQueueCreateInfo = {graphicsQueueCreateInfo, presentQueueCreateInfo, transferQueueCreateInfo};
    }


           //Información para crear el dispositivo lógico.
    vk::DeviceCreateInfo deviceCreateInfo {
            .pNext = nullptr,
            .queueCreateInfoCount = deviceQueueCreateInfo.size(),
            .pQueueCreateInfos =  deviceQueueCreateInfo.data(),
            .enabledExtensionCount = static_cast<uint32_t>(deviceExtension.size()),
            .ppEnabledExtensionNames = deviceExtension.data(),
    };

           //Creación del dispositivo lógico.
    logicalDevice = vk::raii::Device(physicalDevice, deviceCreateInfo);
    //Handler del Queueu para los gráficos.
    graphicsQueue = vk::raii::Queue(logicalDevice, graphicsQueueFamily, 0);
    presentQueue = vk::raii::Queue(logicalDevice, presentQueueFamily, 0);
    transferQueue = vk::raii::Queue(logicalDevice, transferQueueFamily, 0);
}

vk::SurfaceFormatKHR selectSurfaceFormat(std::vector<vk::SurfaceFormatKHR> formats) {
    for (const auto& availableFormat: formats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear){
            return availableFormat;
        }
    }

    return formats[0];
}

vk::PresentModeKHR selectPresentMode(std::vector<vk::PresentModeKHR> presentModes) {
    //Mailbox consume mucha energia. En caso de estar en ahoro de energia en moviles, este debería FIFO.
    for (const auto& availablePresentMode: presentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            return availablePresentMode;
        }
    }

    return vk::PresentModeKHR::eFifo; //Este es el unico que Vulkan asegura
}

vk::Extent2D selectSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    auto height =  ilen::window::IWindow::getHeight();
    auto width = ilen::window::IWindow::getWidth();

    return {
        std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

void VulkanRender::createSwapchain() {
    auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    std::vector<vk::SurfaceFormatKHR> availableFormats = physicalDevice.getSurfaceFormatsKHR(surface);
    std::vector<vk::PresentModeKHR> availablePresentModes = physicalDevice.getSurfacePresentModesKHR(surface);

    //Seleccionar un formato de color para la superficie, de preferecia 32 bits de profundidad, SRGB
    auto selectedFormat = selectSurfaceFormat(availableFormats);
    auto selectedPresentMode = selectPresentMode(availablePresentModes);
    auto selectedExtent = selectSwapExtent(surfaceCapabilities);

    auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
    minImageCount = (surfaceCapabilities.maxImageCount > 0 && minImageCount > surfaceCapabilities.maxImageCount) ?
        surfaceCapabilities.maxImageCount : minImageCount;

    vk::SwapchainCreateInfoKHR swapChainCreateInfo {};
    swapChainCreateInfo.surface = surface;
    swapChainCreateInfo.minImageCount = minImageCount;
    swapChainCreateInfo.imageFormat = selectedFormat.format;
    swapChainCreateInfo.imageColorSpace = selectedFormat.colorSpace;
    swapChainCreateInfo.imageExtent = selectedExtent;
    swapChainCreateInfo.imageArrayLayers = 1; //Esto siempre es 1, a menos que se necesiten más capas por imagen, para una app 3D estereoscópico, por ejemplo
    swapChainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment; //TODO: En Post-Procesado esto debe ser cambiado por VK_IMAGE_USAGE_TRANSFER_DST_BIT. La configuración actual es solo cuando se renderiza directamente a pantalla
    swapChainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    swapChainCreateInfo.presentMode = selectedPresentMode;
    swapChainCreateInfo.clipped = true;
    swapChainCreateInfo.oldSwapchain = nullptr;

    uint32_t queueFamilyIndexes[] = {graphicsQueueFamily, presentQueueFamily};

    //Si la familia de presentación y gráficos son diferentes, tenemos que permitir que el Swapchain intercambie la imagen entre familias.
    if (graphicsQueueFamily != presentQueueFamily) {
        swapChainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndexes;
    }else {
        swapChainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
        swapChainCreateInfo.queueFamilyIndexCount = 0;
        swapChainCreateInfo.pQueueFamilyIndices = nullptr;
    }

    swapchain = vk::raii::SwapchainKHR(logicalDevice, swapChainCreateInfo);
    swapchainImages = swapchain.getImages();

    swapchainExtent = selectedExtent;
    swapchainImageFormat = selectedFormat.format;
}

void VulkanRender::createVulkanImageViews() {
    swapchainTextures.resize(swapchainImages.size());

    for (size_t i = 0; i < swapchainImages.size(); i++) {
        swapchainTextures[i] = std::make_unique<VulkanTexture>(swapchainImages[i], swapchainImageFormat, logicalDevice);
    }
}

VulkanRender::~VulkanRender() {
    logicalDevice.waitIdle();
}

void VulkanRender::beginFrame(){
    //Los vk::RenderPass no se deben hacer cada frame, sino una vez por cada configuración de renderizado, y enviarlo al generar los PSO, en vez de mi struct interno.
    generateRenderPasses();
}

void VulkanRender::generateRenderPasses(){
    //Primero vemos si existe el Render Pass principal, y si es necesario recrearlo.
    using namespace ilen::window;

    const bool recreateSwapchain = (swapchainExtent.width != IWindow::getWidth() || swapchainExtent.height != IWindow::getHeight());
    const bool modifiedGraphicsOptions = (lastUsedGraphicsOptions != ilen::core::options::GraphicsOptions::graphicsOptionsFlags);

    if(renderPass == nullptr || recreateSwapchain || modifiedGraphicsOptions){
        if(recreateSwapchain){
            createSwapchain();
        }
        if(modifiedGraphicsOptions){
            lastUsedGraphicsOptions = ilen::core::options::GraphicsOptions::graphicsOptionsFlags;
        }


        std::vector<vulkan::VulkanAttachmentData> attachData;
        //creando el render pass.
        if(lastUsedGraphicsOptions & ILEN_GRAPHICS_OPTIONS_USE_POSTPROCESS){
            vulkan::VulkanAttachmentData colorAttach;
            colorAttach.format =  swapchainImageFormat;
            colorAttach.initialLayout = vk::ImageLayout::eUndefined;
            colorAttach.samples = vk::SampleCountFlagBits::e1;
            colorAttach.loadOp = vk::AttachmentLoadOp::eClear;
            colorAttach.storeOp = vk::AttachmentStoreOp::eStore;
            colorAttach.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
            colorAttach.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
            colorAttach.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            colorAttach.attachSlot = slots::RenderPassAttachment_Color;
            attachData.push_back(colorAttach);

            if(lastUsedGraphicsOptions & ILEN_GRAPHICS_OPTIONS_USE_MOTION_BLUR){
                vulkan::VulkanAttachmentData velocityBufferAttach;
                velocityBufferAttach.format = vulkan::utils::transformEngineToNativePixelFormat(ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_RG16_FLOAT, swapchainImageFormat);
                velocityBufferAttach.initialLayout = vk::ImageLayout::eUndefined;
                velocityBufferAttach.samples = vk::SampleCountFlagBits::e1;
                velocityBufferAttach.loadOp = vk::AttachmentLoadOp::eClear;
                velocityBufferAttach.storeOp = vk::AttachmentStoreOp::eStore;
                velocityBufferAttach.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
                velocityBufferAttach.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
                velocityBufferAttach.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                velocityBufferAttach.attachSlot = slots::RenderPassAttachment_VelocityBuffer;

                attachData.push_back(velocityBufferAttach);
            }
        }else{
            vulkan::VulkanAttachmentData colorAttach;
            colorAttach.format =  swapchainImageFormat;
            colorAttach.initialLayout = vk::ImageLayout::eUndefined;
            colorAttach.samples = vk::SampleCountFlagBits::e1;
            colorAttach.loadOp = vk::AttachmentLoadOp::eClear;
            colorAttach.storeOp = vk::AttachmentStoreOp::eStore;
            colorAttach.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
            colorAttach.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
            colorAttach.finalLayout = vk::ImageLayout::ePresentSrcKHR;
            colorAttach.attachSlot = slots::RenderPassAttachment_Color;
            attachData.push_back(colorAttach);
        }

        if(vulkanTargetVersion >= vk::ApiVersion12){
            //Usamos Render Pass 2
            std::vector<vk::AttachmentDescription2> finalAttachData;
            std::vector<vk::AttachmentReference2> attachReference;
            finalAttachData.resize(8);

            for(auto& attach: attachData){
                finalAttachData[attach.attachSlot] = attach.getV2Descriptor();
                attachReference.push_back(attach.getV2Reference());
            }

            vk::SubpassDescription2 subpass{};
            subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
            subpass.colorAttachmentCount = attachReference.size();
            subpass.pColorAttachments = attachReference.data();

            vk::RenderPassCreateInfo2 renderPassCreateInfo{};
            renderPassCreateInfo.attachmentCount = finalAttachData.size();
            renderPassCreateInfo.pAttachments = finalAttachData.data();
            renderPassCreateInfo.subpassCount = 1;
            renderPassCreateInfo.pSubpasses = &subpass;

            renderPass = vk::raii::RenderPass(logicalDevice, renderPassCreateInfo);
        }else if(availableOptionalFeatures & VULKAN_OPTIONAL_FEATURES_BIT_FLAG::VK_FEATURE_RENDER_PASS2){
            //Usamos Render Pass 2 KHR
            std::vector<vk::AttachmentDescription2KHR> finalAttachData;
            std::vector<vk::AttachmentReference2KHR> attachReference;
            finalAttachData.resize(8);

            for(auto& attach: attachData){
                finalAttachData[attach.attachSlot] = attach.getV2Descriptor();
                attachReference.push_back(attach.getV2Reference());
            }

            vk::SubpassDescription2KHR subpass{};
            subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
            subpass.colorAttachmentCount = attachReference.size();
            subpass.pColorAttachments = attachReference.data();

            vk::RenderPassCreateInfo2KHR renderPassCreateInfo{};
            renderPassCreateInfo.attachmentCount = finalAttachData.size();
            renderPassCreateInfo.pAttachments = finalAttachData.data();
            renderPassCreateInfo.subpassCount = 1;
            renderPassCreateInfo.pSubpasses = &subpass;

            renderPass = vk::raii::RenderPass(logicalDevice, renderPassCreateInfo);
        }else{
            //Usar V1
                        //Usamos Render Pass 2
            std::vector<vk::AttachmentDescription> finalAttachData;
            std::vector<vk::AttachmentReference> attachReference;
            finalAttachData.resize(8);

            for(auto& attach: attachData){
                finalAttachData[attach.attachSlot] = attach.getV1Descriptor();
                attachReference.push_back(attach.getV1Reference());
            }

            vk::SubpassDescription subpass{};
            subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
            subpass.colorAttachmentCount = attachReference.size();
            subpass.pColorAttachments = attachReference.data();

            vk::RenderPassCreateInfo renderPassCreateInfo{};
            renderPassCreateInfo.attachmentCount = finalAttachData.size();
            renderPassCreateInfo.pAttachments = finalAttachData.data();
            renderPassCreateInfo.subpassCount = 1;
            renderPassCreateInfo.pSubpasses = &subpass;

            renderPass = vk::raii::RenderPass(logicalDevice, renderPassCreateInfo);
        }
    }
}

void VulkanRender::endFrame() {

}

std::shared_ptr<IGPUBuffer> VulkanRender::createBuffer(const IlenBufferDescriptor &descriptor, const void *data){
    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eTransient;
    poolInfo.queueFamilyIndex = transferQueueFamily;

    vk::CommandPool commandPool = logicalDevice.createCommandPool(poolInfo);
    
    return std::make_shared<VulkanBuffer>(vulkanTargetVersion, logicalDevice, physicalDevice, commandPool, transferQueue, descriptor, data);
}

std::unique_ptr<IGPUTexture> VulkanRender::createNewPlatformTexture(const IlenGPUTextureDescriptor &descriptor){

}

std::shared_ptr<IGPURenderPipeline> VulkanRender::createRenderPipeline(const ilen::render::IlenRenderPipelineDescriptor &pipelineDescriptor, ilen::resources::shaders::ShaderManager *shaderManager){
    return std::make_shared<VulkanRenderPipeline>(logicalDevice, pipelineDescriptor, shaderManager, swapchainExtent, actualMainDescriptor, renderPass);
}

std::shared_ptr<IShader> VulkanRender::createShader(const ilen::graphics::IlenShader &shaderDescriptor){
    return std::make_shared<VulkanShader>(shaderDescriptor, logicalDevice);
}

void VulkanRender::endEncoding(IGPUParallelCommandEncoder *encoder){

}

IGPUInstanceBuffer * VulkanRender::getInstanceBuffer(){

}

std::unique_ptr<IGPUParallelCommandEncoder> VulkanRender::getParallelCommandEncoder(){

}

void VulkanRender::returnParallelCommandEncoder(std::unique_ptr<IGPUParallelCommandEncoder> encoder){

}

std::unique_ptr<IGPUParallelCommandEncoder> VulkanRender::createNewParallelCommandEncoder(IlenParallelEncoderDescriptor&& descriptor){

}

std::unique_ptr<IGPUInstanceBuffer>  VulkanRender::createInstanceBuffer(size_t bufferSize, std::string bufferName){

}

void VulkanRender::regenerateEncoderDescriptor(){

}

IGPUTexture* VulkanRender::getDrawableTexturePtr() {

}

std::vector<IGPUTexture*> VulkanRender::getAllSceneTexturesPtr(){

}

std::shared_ptr<IGPUTexture> VulkanRender::createTexture(const IlenGPUTextureDescriptor& descriptor, const void* initialData) {

}

std::shared_ptr<IGPUSampler> VulkanRender::createSampler(const IlenGPUSamplerDescriptor& descriptor) {

}

void VulkanRender::defineSupportedGPUCompression(){

}

#if ILEN_DEBUG
    VKAPI_ATTR vk::Bool32 VKAPI_CALL VulkanRender::debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity, vk::DebugUtilsMessageTypeFlagsEXT messageType,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void*) {
    std::string VALIDATION_PREFIX = "VULKAN_VALIDATION(" + to_string(messageType) + ")";
    switch (messageSeverity) {
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
            ILEN_ERROR_P(VALIDATION_PREFIX.c_str(), pCallbackData->pMessage);
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
            ILEN_WARNING_P(VALIDATION_PREFIX.c_str(), pCallbackData->pMessage);
            break;
        default:
            ILEN_LOG_P(VALIDATION_PREFIX.c_str(), pCallbackData->pMessage);
    }

    return vk::False;
}

    void VulkanRender::setVulkanDebugCalls(){
        if (enableValidationLayers == true) {
            vk::DebugUtilsMessageSeverityFlagsEXT severityFlags (vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                                                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
            vk::DebugUtilsMessageTypeFlagsEXT messageTypes (/*vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |*/
                                                           vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

            vk::DebugUtilsMessengerCreateInfoEXT createDebugUtils {
                    .messageSeverity = severityFlags,
                    .messageType = messageTypes,
                    .pfnUserCallback = &debugCallback,
            };

            debugUtilsMessenger = instance.createDebugUtilsMessengerEXT(createDebugUtils);
        }
    }
#endif


#endif
