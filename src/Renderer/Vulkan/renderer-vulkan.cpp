#if ILEN_LINUX || ILEN_WINDOWS || ILEN_ANDROID
#define VK_USE_PLATFORM_XCB_KHR
#include <renderer-vulkan.h>
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

VulkanRender::VulkanRender(const void *native_surface) {
#if ILEN_DEBUG
    enableValidationLayers = true;
    ILEN_WARNING_P(DEBUG_PREFIX, "Validation layers for Vulkan enabled.");
#endif

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

#if ILEN_DEBUG
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
#endif

    //Selección del dispositivo físico y creción del dispositivo lógico.
    auto devices = instance.enumeratePhysicalDevices();
    if (devices.empty()) {
        ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "No existe un dispositivo físico compatible con Vulkan.");
    }

    //Seleccionando dispositivo según capacidad.

    std::vector<const char*> deviceExtension = {
        vk::KHRSwapchainExtensionName, //Obligatorio
        vk::KHRSpirv14ExtensionName,
        vk::KHRSynchronization2ExtensionName,
        vk::KHRCreateRenderpass2ExtensionName
    };

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

        if (isUtilGPU) {
            physicalDevice = device.second;
            break;
        }
    }

    if (physicalDevice ==nullptr) {
        ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "Fallo la obtención del dispositivo físico válido para el motor");
    }

    //Determinar familias válidas para el uso de Vulkan.
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

    auto graphicsQueueFamilyProperty = std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(),
        [](vk::QueueFamilyProperties const& qfp){
            return qfp.queueFlags & vk::QueueFlagBits::eGraphics;
        });

    uint32_t graphicsQueueFamily = static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyProperty));

    //Información de las Queues a crear en cada familia para la creación del dispositivo lógico.
    float queuePriority = 0.5f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
        .queueFamilyIndex = graphicsQueueFamily,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
    };

    //Información para crear el dispositivo lógico.
    vk::DeviceCreateInfo deviceCreateInfo {
        .pNext = nullptr,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos =  &deviceQueueCreateInfo,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtension.size()),
        .ppEnabledExtensionNames = deviceExtension.data(),
    };

    //Creación del dispositivo lógico.
    logicalDevice = vk::raii::Device(physicalDevice, deviceCreateInfo);
    //Handler del Queueu para los gráficos.
    graphicsQueue = vk::raii::Queue(logicalDevice, graphicsQueueFamily, 0);


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

VulkanRender::~VulkanRender() {

}

void VulkanRender::beginFrame(){

}

void VulkanRender::endFrame() {

}

std::shared_ptr<IGPUBuffer> VulkanRender::createBuffer(const IlenBufferDescriptor &descriptor, const void *data){

}

std::unique_ptr<IGPUTexture> VulkanRender::createNewPlatformTexture(const IlenGPUTextureDescriptor &descriptor){

}

std::shared_ptr<IGPURenderPipeline> VulkanRender::createRenderPipeline(const ilen::render::IlenRenderPipelineDescriptor &pipelineDescriptor, ilen::resources::shaders::ShaderManager *shaderManager){

}

std::shared_ptr<IShader> VulkanRender::createShader(const ilen::graphics::IlenShader &shaderDescriptor){

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

#if ILEN_DEBUG
    VKAPI_ATTR vk::Bool32 VKAPI_CALL VulkanRender::debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity, vk::DebugUtilsMessageTypeFlagsEXT messageType,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void*) {
    std::string VALIDATION_PREFIX = "VULKAN_VALIDATION(" + to_string(messageType) + ")";
    switch (messageSeverity) {
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
            ILEN_ERROR_P(VALIDATION_PREFIX.c_str(), pCallbackData->pMessage);
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
            ILEN_WARNING_P(VALIDATION_PREFIX.c_str(), pCallbackData->pMessage);
        default:
            ILEN_LOG_P(VALIDATION_PREFIX.c_str(), pCallbackData->pMessage);
    }

    return vk::False;
}
#endif


#endif
