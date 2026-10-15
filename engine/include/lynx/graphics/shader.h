#pragma once

#include <string>
#include <filesystem>
#include <istream>
#include <lynx/headers/enumerations.h>
//#include <external/json/json.hpp>
//#include <lynx/resources/assets-manager.h>
#include <lynx/resources/binary_headers.h>
#include <lynx/resources/assets_elements.h>


/*namespace lynx::binary::headers
{
    struct LynxShaderHeader;
}*/
/*
namespace lynx::resources {
    using namespace lynx::enumerations;

    template<LYNX_ASSET_TYPE T>
    struct AssetElement;
    
    using ShaderAsset = AssetElement<LYNX_ASSET_TYPE_SHADER>; // El 4 corresponde a LYNX_ASSET_TYPE_SHADER
}*/


namespace lynx::graphics {
    //Estructura de alto nivel que gestiona los shaders del motor. Luego una implementación segun Graphic Pipeline consultara esta información para generar los shaders.
    struct LynxShader {
        std::string shaderFileName = ""; //Nombre del archivo fuente o bytecode del shader (vidrio.spv, carroceria.spv, etc)
        std::filesystem::path shaderRoute = ""; //Ruta relativa desde shader-code hasta la carpeta superior al archivo de shader. (assets/shader-code// [carros/ford] //vidrio.spv)
        std::string shaderName = ""; //Nombre legible del shader... Solo para la UI del editor

        LynxShader(std::string shaderFileName, std::string shaderRoute, std::string humanName) : shaderFileName(shaderFileName), shaderRoute(shaderRoute), shaderName(humanName) {};
        LynxShader(std::istream& stream, const lynx::binary::headers::LynxShaderHeader& header)
        {
            shaderFileName.resize(header.fileNameSize);
            stream.read(shaderFileName.data(), header.fileNameSize);

            std::string tempRoute;
            tempRoute.resize(header.fileRouteSize);
            stream.read(tempRoute.data(), header.fileRouteSize);
            shaderRoute = tempRoute;

            shaderName.resize(header.shaderNameSize);
            stream.read(shaderName.data(), header.shaderNameSize);
        }
    };

    //TODO: Añadir métodos para desde el editor, crear y gestionar shaders en esta capa de abstracción.
    /*class LynxShaderUtils {
        public:
            static LynxShader generateLynxShaderFromAsset(const lynx::resources::ShaderAsset& asset);
            static LynxShader getShaderFromID(const uint32_t assetID);
        private:
            static inline const char* DEBUG_PREFIX = "SHADER_HELPER";
    };*/

   // static const LynxShader ErrorShader{"error", "predefined/error-shader", ""};
}