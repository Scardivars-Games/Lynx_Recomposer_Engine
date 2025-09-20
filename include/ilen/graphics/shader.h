#pragma once

#include <string>
#include <filesystem>
#include <ilen/headers/enumerations.h>
#include <ilen/core/filemanager.h>
#include <external/json/json.hpp>
#include <ilen/core/assets-manager.h>

namespace ilen::core {
    using namespace ilen::enumerations;

    template<ILEN_ASSET_TYPE T>
    struct AssetElement;
    
    using ShaderAsset = AssetElement<ILEN_ASSET_TYPE_SHADER>; // El 4 corresponde a ILEN_ASSET_TYPE_SHADER
}


namespace ilen::graphics {
    //Estructura de alto nivel que gestiona los shaders del motor. Luego una implementación segun Graphic Pipeline consultara esta información para generar los shaders.
    struct IlenShader {
        std::string shaderFileName; //Nombre del archivo fuente o bytecode del shader (vidrio.spv, carroceria.spv, etc)
        std::filesystem::path shaderRoute; //Ruta relativa desde shader-code hasta la carpeta superior al archivo de shader. (assets/shader-code// [carros/ford] //vidrio.spv)
        std::string shaderName; //Nombre legible del shader... Solo para la UI del editor

        IlenShader(std::string shaderFileName, std::string shaderRoute, std::string humanName) : shaderFileName(shaderFileName), shaderRoute(shaderRoute), shaderName(humanName) {};
    };

    //TODO: Añadir métodos para desde el editor, crear y gestionar shaders en esta capa de abstracción.
    class IlenShaderUtils {
        public:
            static IlenShader generateIlenShaderFromAsset(const ilen::core::ShaderAsset& asset);
    };

    static const IlenShader ErrorShader{"error", "predefined/error-shader", ""};
}