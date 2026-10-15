#pragma once

#include <string>
#include <lynx/headers/enumerations.h>
#include <fstream>
#include <lynx/maths/vector.h>
#include <lynx/graphics/color.h>
#include <unordered_map>
#include <variant>
#include <lynx/resources/binary_headers.h>
/*
namespace lynx::resources {
    using namespace lynx::enumerations;

    template <LYNX_ASSET_TYPE T>
    struct AssetElement;

    using MaterialAsset = AssetElement<LYNX_ASSET_TYPE_MATERIAL>;
}*/

namespace lynx::graphics {
    // Propiedades que un material puede enviar a un shader.
    // Usamos std::variant para poder almacenar diferentes tipos de datos en un mismo mapa.

    using namespace lynx::enumerations;

    using MaterialProperty = std::variant<
        float,
        lynx::graphics::Color,
        lynx::maths::vector::vec2,
        lynx::maths::vector::vec3,
        lynx::maths::vector::vec4,
        uint32_t // Para IDs de textura
    >;

    struct LynxMaterial {
        std::string materialName = "NO_NAME";
        uint32_t vertexShaderID = 4; //Error shader
        uint32_t fragmentShaderID = 4; //Error shader

        std::string vertexFunctionName = "error_vertex";
        std::string fragmentFunctionName = "error_fragment";

        LYNX_GPU_RENDER_QUEUE_TYPE renderQueue = LYNX_GPU_RENDER_QUEUE_TYPE::LYNX_GPU_RENDER_QUEUE_OPAQUE;
        
        //Asigna a cada propiedad su valor correspondiente, siempre y cuando sea de algún tipo permitido.
        std::unordered_map<std::string, MaterialProperty> properties;
        explicit LynxMaterial() = default;

        explicit LynxMaterial(std::ifstream& stream, const lynx::binary::headers::LynxMaterialHeader& header) :
            vertexShaderID(4), fragmentShaderID(4)
        {
            materialName.resize(header.materialNameSize);
            vertexFunctionName.resize(header.vertexFunctionSize);
            fragmentFunctionName.resize(header.fragmentFunctionSize);

            stream.read(reinterpret_cast<char*>(&vertexShaderID), sizeof(uint32_t));
            stream.read(reinterpret_cast<char*>(&fragmentShaderID), sizeof(uint32_t));
            stream.read(reinterpret_cast<char*>(&renderQueue), sizeof(LYNX_GPU_RENDER_QUEUE_TYPE));

            stream.read(materialName.data(), header.materialNameSize);
            stream.read(vertexFunctionName.data(), header.vertexFunctionSize);
            stream.read(fragmentFunctionName.data(), header.fragmentFunctionSize);
        }
    };

    /*
    class LynxMaterialHelper {
        public:
            static LynxMaterial generateLynxMaterialFromAsset(const lynx::resources::MaterialAsset& asset);

        private:
            static const inline char* DEBUG_PREFIX = "MATERIAL_HELPER";
    };*/
}
