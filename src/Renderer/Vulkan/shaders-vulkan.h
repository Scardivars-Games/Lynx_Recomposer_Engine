#pragma once
#if ILEN_LINUX || ILEN_WINDOWS || ILEN_ANDROID
#include <ilen/render/gpu-interfaces/shaders.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <unordered_map>

namespace ilen::render {
    class VulkanShader final: public IShader {
    public:
        VulkanShader(ilen::graphics::IlenShader shaderRecipe, vk::raii::Device& device);
        ~VulkanShader();
        const ilen::graphics::IlenShader& getShaderMetadata() const override;
        const void* getNativeShaderFromFunctionName(std::string functionName) override;
        const void* getNativeShaderFromFunctionNameAndUberActivations(std::string functionName, std::vector<IlenConstantValuesForPipeline> activations) override;
    private:
        vk::Device _device;
        std::unordered_map<std::string, vk::ShaderModule> _shaderModules; //Mapa que vincula cada funcion con su respectivo módulo.
        std::unordered_map<std::string, vk::PipelineShaderStageCreateInfo> _shaderStageFunctions; //Esto es lo equivalente a los MTLFuction que envio desde Metal. Es el módulo por cada función del shader.
        std::vector <char> _bytecodeBuffer; //Necesario para generar especificaciones sin volver a leer.

        static inline const char* DEBUG_PREFIX = "VULKAN_SHADER";
    };
}

#endif
