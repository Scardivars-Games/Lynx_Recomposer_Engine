#include <lynx/graphics/material.h>
#include <lynx/core/filemanager.h>
#include <lynx/core/logs.h>
#include <lynx/core/logs.h>
#include <lynx/maths/vector.h>
#include <filesystem>


using namespace lynx::core;

const std::string LYNX_MATERIAL_NAME = "material-name";
const std::string LYNX_MATERIAL_VERTEX_SHADER_ID = "vertex-shader-id";
const std::string LYNX_MATERIAL_FRAGMENT_SHADER_ID = "fragment-shader-id";
const std::string LYNX_MATERIAL_VERTEX_SHADER_FUNCTION = "fragment-shader-function";
const std::string LYNX_MATERIAL_FRAGMENT_SHADER_FUNCTION = "vertex-shader-function";
const std::string LYNX_MATERIAL_FLOAT_PROPERTY = "float-property";
const std::string LYNX_MATERIAL_INTEGER_PROPERTY = "int-property";
const std::string LYNX_MATERIAL_FLOAT2_PROPERTY = "float2-property";
const std::string LYNX_MATERIAL_FLOAT3_PROPERTY = "float3-property";
const std::string LYNX_MATERIAL_FLOAT4_PROPERTY = "float4-property";
const std::string LYNX_MATERIAL_TEXTURE_PROPERTY = "texture-property"; // Para texturas

/*
namespace lynx::maths::vector {
    // Le enseña a nlohmann/json cómo convertir un array JSON a un lynx::maths::vector
    template<typename T, size_t N>
    void from_json(const nlohmann::json& j, Vector<T, N>& v) {
        if (j.is_array() && j.size() == N) {
            for (size_t i = 0; i < N; ++i) {
                j.at(i).get_to(v.data[i]);
            }
        }
    }
}

// Función auxiliar para procesar un objeto de propiedades escalares (float, int).
template<typename T>
void processScalarProperties(const nlohmann::json& jsonData, const std::string& key, std::unordered_map<std::string, lynx::graphics::MaterialProperty>& properties) {
    if (jsonData.contains(key) && jsonData[key].is_object()) {
        // Iteramos sobre cada par clave-valor del objeto JSON (ej. "u_Roughness": 0.5)
        for (auto const& [propName, propValueJson] : jsonData[key].items()) {
            if (propValueJson.is_number()) {
                properties[propName] = propValueJson.get<T>();
            }
        }
    }
}

// Función auxiliar para procesar un objeto de propiedades vectoriales (vec2, vec3, Color).
template<typename VecType>
void processVectorProperties(const nlohmann::json& jsonData, const std::string& key, std::unordered_map<std::string, lynx::graphics::MaterialProperty>& properties) {
    if (jsonData.contains(key) && jsonData[key].is_object()) {
        // Iteramos sobre cada par clave-valor del objeto JSON (ej. "_color": [1, 0, 0])
        for (auto const& [propName, propValueJson] : jsonData[key].items()) {
            if (propValueJson.is_array()) {
                VecType vecValue;
                // nlohmann::json puede deserializar un array JSON directamente
                // a una estructura como vec3 o Color si los miembros coinciden.
                propValueJson.get_to(vecValue);
                properties[propName] = vecValue;
            }
        }
    }
}*/
/*
namespace lynx::graphics {
    IlenMaterial IlenMaterialHelper::generateIlenMaterialFromAsset(const lynx::resources::MaterialAsset& asset){
        using json = nlohmann::json;
        auto materialPath = FileManager::fileRoute(asset.assetPath, LYNX_FILE_TYPE_NONE);

        std::ifstream file(materialPath);
        json data;

        if(file.is_open()){
            file >> data;
        } else {
            LYNX_ERROR_P(DEBUG_PREFIX, "No se pudo abrir el archivo de material en la ruta: %s", materialPath.c_str());
            return IlenMaterial{};
        }

        IlenMaterial material;
        material.materialName = data.value(LYNX_MATERIAL_NAME, "Unnamed Material");
        material.vertexShaderID = data.value(LYNX_MATERIAL_VERTEX_SHADER_ID, 0);
        material.fragmentShaderID = data.value(LYNX_MATERIAL_FRAGMENT_SHADER_ID, 0);
        
        // Corregí el typo en las constantes para que coincidan con el archivo de cabecera
        material.vertexFunctionName = data.value(LYNX_MATERIAL_VERTEX_SHADER_FUNCTION, "");
        material.fragmentFunctionName = data.value(LYNX_MATERIAL_FRAGMENT_SHADER_FUNCTION, "");

        // Procesar todas las propiedades con la nueva lógica
        processScalarProperties<float>(data, LYNX_MATERIAL_FLOAT_PROPERTY, material.properties);
        processScalarProperties<uint32_t>(data, LYNX_MATERIAL_INTEGER_PROPERTY, material.properties);
        processScalarProperties<uint32_t>(data, LYNX_MATERIAL_TEXTURE_PROPERTY, material.properties); // Las texturas se guardan como ID (uint32_t)

        processVectorProperties<lynx::maths::vector::vec2>(data, LYNX_MATERIAL_FLOAT2_PROPERTY, material.properties);
        processVectorProperties<lynx::maths::vector::vec3>(data, LYNX_MATERIAL_FLOAT3_PROPERTY, material.properties);
        processVectorProperties<lynx::maths::vector::vec4>(data, LYNX_MATERIAL_FLOAT4_PROPERTY, material.properties);

        return material;
    }
}*/