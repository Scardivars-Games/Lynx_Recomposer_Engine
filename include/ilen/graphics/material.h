#pragma once

#include <cstdint>
#include <string>
#include <ilen/headers/enumerations.h>
#include <ilen/core/filemanager.h>
#include <external/json/json.hpp>
#include <ilen/core/assets-manager.h>
#include <ilen/maths/vector.h>
#include <ilen/graphics/color.h>
#include <unordered_map>
#include <variant>

namespace ilen::core {
    using namespace ilen::enumerations;

    template <ILEN_ASSET_TYPE T>
    struct AssetElement;

    using MaterialAsset = AssetElement<ILEN_ASSET_TYPE_MATERIAL>;
}

namespace ilen::graphics {

    // Propiedades que un material puede enviar a un shader.
    // Usamos std::variant para poder almacenar diferentes tipos de datos en un mismo mapa.
    using MaterialProperty = std::variant<
        float,
        ilen::graphics::Color,
        ilen::maths::vector::vec2,
        ilen::maths::vector::vec3,
        ilen::maths::vector::vec4,
        uint32_t // Para IDs de textura
    >;

    struct IlenMaterial {
        std::string materialName;
        uint32_t fragmentShaderID;
        uint32_t vertexShaderID;

        std::string vertexFunctionName;
        std::string fragmentFunctionName;
        
        //Asigna a cada propiedad su valor correspondiente, siempre y cuando sea de algún tipo permitido.
        std::unordered_map<std::string, MaterialProperty> properties;
    };

    class IlenMaterialHelper {
        public:
            static IlenMaterial generateIlenMaterialFromAsset(const ilen::core::MaterialAsset& asset);
    };
}