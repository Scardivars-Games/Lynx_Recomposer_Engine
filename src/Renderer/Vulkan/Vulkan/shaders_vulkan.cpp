#if ILEN_LINUX || ILEN_WINDOWS || ILEN_ANDROID
#include <Vulkan/shaders-vulkan.h>
#include <ilen/core/filemanager.h>

#include <ilen/headers/ilen-logs.h>

using namespace ilen::render;
using namespace ilen::core;

VulkanShader::VulkanShader(ilen::graphics::IlenShader shaderRecipe, vk::raii::Device &device) {
    //Primer paso, leer el bytecode desde el disco.
    shaderData = std::move(shaderRecipe);
    auto filename = shaderData.shaderRoute / shaderData.shaderFileName;
    filename.replace_extension(".spv");
    auto fileRoute = FileManager::fileRoute(filename, ILEN_FILE_TYPE_NATIVE_SHADER);

    std::string vertNameString = this->shaderData.shaderFileName + "_vertex";
    std::string fragNameString = this->shaderData.shaderFileName + "_fragment";
    std::string moduleName = vertNameString + "/" + fragNameString;

    std::ifstream file(fileRoute, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "ERROR AL LEER EL ARCHIVO BINARIO DEL SHADER %s en la ruta %s", shaderRecipe.shaderName.c_str(), fileRoute.c_str());
    }

    std::vector <char> buffer(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    file.close();

    //Creación de los módulos
    vk::ShaderModuleCreateInfo createInfo{};
    createInfo.codeSize = buffer.size() * sizeof(char);
    createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

    _shaderModules.emplace(moduleName, device.createShaderModule(createInfo));

    //Creamos los stage a partir del módulo.
    auto module = _shaderModules[moduleName];

    vk::PipelineShaderStageCreateInfo defaultVertexStage{};
    defaultVertexStage.stage = vk::ShaderStageFlagBits::eVertex;
    defaultVertexStage.module = module;
    defaultVertexStage.pName = vertNameString.c_str();

    vk::PipelineShaderStageCreateInfo defaultFragmentStage{};
    defaultFragmentStage.stage = vk::ShaderStageFlagBits::eFragment;
    defaultFragmentStage.module = module;
    defaultFragmentStage.pName = fragNameString.c_str();

    _shaderStageFunctions.emplace(vertNameString, defaultVertexStage);
    _shaderStageFunctions.emplace(fragNameString, defaultFragmentStage);

    _bytecodeBuffer = buffer;
}

VulkanShader::~VulkanShader() {
   // ILEN_LOG_P(DEBUG_PREFIX, "Eliminando shaders");
    for (auto& module : _shaderModules) {
        if (module.second) {
            _device.destroyShaderModule(module.second);
        }
    }
    _shaderModules.clear();
}

const ilen::graphics::IlenShader& VulkanShader::getShaderMetadata() const {
    return shaderData;
}

const void* VulkanShader::getNativeShaderFromFunctionName(std::string functionName){
    auto it = _shaderStageFunctions.find(functionName);
    if (it != _shaderStageFunctions.end()) {
        return &it->second;
    }
    //Si no existe, se genera la libreria y añade
    vk::PipelineShaderStageCreateInfo newShaderStage{};

    //Obtener el stage a partir del nombre
    std::stringstream ss(functionName);
    std::string segment;
    std::vector<std::string> parts;
    while (std::getline(ss, segment, '_')) {
        parts.push_back(segment);
    }

    if (parts.empty()) {
        ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "NOMBRE DE FUNCIÓN INVALIDO. NO CUMPLE LA NORMA DE SUFIJO _vertex/_fragment. Nombre incorrecto: %s", functionName.c_str());
    }

    const std::string& last = parts.back();
    if (last == "vertex") {
        newShaderStage.stage = vk::ShaderStageFlagBits::eVertex;
    }else if (last == "fragment") {
        newShaderStage.stage = vk::ShaderStageFlagBits::eFragment;
    }else {
        ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "NOMBRE DE FUNCIÓN INVALIDO. NO CUMPLE LA NORMA DE SUFIJO _vertex/_fragment. Nombre incorrecto: %s", functionName.c_str());
    }
    //No tiene variantes, cargamos el modulo por defecto
    std::string vertNameString = this->shaderData.shaderFileName + "_vertex";
    std::string fragNameString = this->shaderData.shaderFileName + "_fragment";
    std::string moduleName = vertNameString + "/" + fragNameString;
    auto module = _shaderModules[moduleName];

    newShaderStage.module = module;
    newShaderStage.pName = functionName.c_str();

    _shaderStageFunctions.emplace(functionName, newShaderStage);
    return &_shaderStageFunctions[functionName];
}

const void* VulkanShader::getNativeShaderFromFunctionNameAndUberActivations(std::string functionName, std::vector<IlenConstantValuesForPipeline> activations){
    
}

#endif
