#if ILEN_LINUX || ILEN_WINDOWS || ILEN_ANDROID
#define VK_USE_PLATFORM_XCB_KHR
#include <renderer-vulkan.h>
#include <algorithm>
#include <cstring>

#include <vulkan/vulkan_xcb.h> //Adicionar al TODO de las instancias.
using namespace ilen::render;

std::vector<const char*> instanceExtensions = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    //TODO: Al usar Wayland, comprobar cual extension en especifico es la necesaria.
    VK_KHR_XCB_SURFACE_EXTENSION_NAME,
};

//Validation layer code
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

VulkanRender::VulkanRender(void *native_surface) {
#if ILEN_DEBUG
    enableValidationLayers = true;
    ILEN_WARNING_P(DEBUG_PREFIX, "Validation layers for Vulkan enabled.");
#endif

    //El objetivo de esto es generar hasta el dispositivo logico de la app, o sea, muchas cosas
    //Creemos los datos de la instancia
    vk::ApplicationInfo appInfo {
        .pApplicationName = application::Application::getApplicationName().c_str(),
        .applicationVersion = VK_MAKE_VERSION(application::Application::getMajorVersion(), application::Application::getMinorVersion(), application::Application::getPatchVersion()),
        .pEngineName = "Ilusion's Engine",
        .engineVersion =  VK_MAKE_VERSION(1,0,0),
        .apiVersion =  vk::ApiVersion10,
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
    if (enableValidationLayers) {
        vk::DebugUtilsMessageSeverityFlagsEXT severityFlags (vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
        vk::DebugUtilsMessageTypeFlagsEXT messageTypes (vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
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

    if (candidates.rbegin()->first > 0) {
        physicalDevice = candidates.rbegin()->second;
    }else {
        ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "Fallo la obtención del dispositivo físico válido para el motor");
    }
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
