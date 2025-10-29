#pragma once

#include <cstdint>
#include <string>
#include <ilen/headers/enumerations.h>
#include <fstream>
//#include <ilen/core/filemanager.h>
//#include <external/json/json.hpp>
//#include <ilen/resources/assets-manager.h>
#include <ilen/maths/vector.h>
#include <ilen/graphics/color.h>
#include <unordered_map>
#include <variant>
#include <ilen/resources/binary_headers.h>
/*
namespace ilen::resources {
    using namespace ilen::enumerations;

    template <ILEN_ASSET_TYPE T>
    struct AssetElement;

    using MaterialAsset = AssetElement<ILEN_ASSET_TYPE_MATERIAL>;
}*/

namespace ilen::graphics {
    // Propiedades que un material puede enviar a un shader.
    // Usamos std::variant para poder almacenar diferentes tipos de datos en un mismo mapa.

    using namespace ilen::enumerations;

    using MaterialProperty = std::variant<
        float,
        ilen::graphics::Color,
        ilen::maths::vector::vec2,
        ilen::maths::vector::vec3,
        ilen::maths::vector::vec4,
        uint32_t // Para IDs de textura
    >;

    struct IlenMaterial {
        std::string materialName = "NO_NAME";
        uint32_t vertexShaderID = 4; //Error shader
        uint32_t fragmentShaderID = 4; //Error shader

        std::string vertexFunctionName = "error_vertex";
        std::string fragmentFunctionName = "error_fragment";

        ILEN_GPU_RENDER_QUEUE_TYPE renderQueue = ILEN_GPU_RENDER_QUEUE_TYPE::ILEN_GPU_RENDER_QUEUE_OPAQUE;
        
        //Asigna a cada propiedad su valor correspondiente, siempre y cuando sea de algún tipo permitido.
        std::unordered_map<std::string, MaterialProperty> properties;
        explicit IlenMaterial() = default;

        explicit IlenMaterial(std::ifstream& stream, const ilen::binary::headers::IlenMaterialHeader& header) :
            vertexShaderID(4), fragmentShaderID(4)
        {
            materialName.resize(header.materialNameSize);
            vertexFunctionName.resize(header.vertexFunctionSize);
            fragmentFunctionName.resize(header.fragmentFunctionSize);

            stream.read(reinterpret_cast<char*>(&vertexShaderID), sizeof(uint32_t));
            stream.read(reinterpret_cast<char*>(&fragmentShaderID), sizeof(uint32_t));
            stream.read(reinterpret_cast<char*>(&renderQueue), sizeof(ILEN_GPU_RENDER_QUEUE_TYPE));

            stream.read(materialName.data(), header.materialNameSize);
            stream.read(vertexFunctionName.data(), header.vertexFunctionSize);
            stream.read(fragmentFunctionName.data(), header.fragmentFunctionSize);
        }
    };

    /*
    class IlenMaterialHelper {
        public:
            static IlenMaterial generateIlenMaterialFromAsset(const ilen::resources::MaterialAsset& asset);

        private:
            static const inline char* DEBUG_PREFIX = "MATERIAL_HELPER";
    };*/
}
