#pragma once

#include <string>
#include <memory>
#include <external/json/json.hpp>
#include <ilen/core/logs.h>
#include <ilen/render/render-component.h>
#include <ilen/headers/enumerations.h>

namespace ilen::graphics {
    struct IlenShader;
    class IlenShaderUtils;
    struct IlenMaterial;
    class IlenMaterialHelper;
}

//Administra una base de datos, en formato JSON, con cada objeto y elemento que administre el motor. Su comportamiento será el de un Singleton.
namespace ilen::core {
    using namespace ilen::enumerations;
    template<ILEN_ASSET_TYPE T>
    struct AssetElement {
        ILEN_ASSET_TYPE type = T;
        uint32_t assetID;
        std::filesystem::path assetPath;
        
        AssetElement() = delete;
        AssetElement(uint32_t id, std::filesystem::path path) : assetID(id), assetPath(path) {} ;
    };

    using ModelAsset = AssetElement<ILEN_ASSET_TYPE_MODEL>;
    using TextureAsset = AssetElement<ILEN_ASSET_TYPE_TEXTURE>;
    using MaterialAsset = AssetElement<ILEN_ASSET_TYPE_MATERIAL>;
    using AudioAsset = AssetElement<ILEN_ASSET_TYPE_AUDIO>;
    using ShaderAsset = AssetElement<ILEN_ASSET_TYPE_SHADER>;

    class AssetsManager {
        public:
            static AssetsManager& getInstance();

            //Requisitos del arhcivo de manifiesto: 1 solo archivo por proyecto. Cada tipo de dato esta en estructuras diferentes. Los IDs no son repetitivos, aunque esten en diferentes estructuras, los ID son únicos como si de elemento global se tratase.
            //Estructura del manifiesto: - totalOfElements : uint32_t (total de datos disponibles en el proyecto). Estructuras : - ID, -StrigName -FilePath.
            uint32_t getAssetID(const std::string& friendlyName) const;

            template<typename T>
            const T& getAssetElement(const uint32_t& id) const {
                if constexpr (std::is_same_v<T, ModelAsset>) {
                    auto it = m_idToModelAsset.find(id);
                    if (it == m_idToModelAsset.end()){
                        Debug::ExecuteError("Asset con id: " + std::to_string(id) + " no existente en el subgrupo de modelos.");
                    }
                    return it->second;
                } else if constexpr (std::is_same_v<T, TextureAsset>) {
                    auto it = m_idToTextureAsset.find(id);
                    if (it == m_idToTextureAsset.end()){
                        Debug::ExecuteError("Asset con id: " + std::to_string(id) + " no existente en el subgrupo de texturas.");
                    }
                    return it->second;
                } else if constexpr (std::is_same_v<T, AudioAsset>) {
                    auto it = m_idToAudioAsset.find(id);
                    if (it == m_idToAudioAsset.end()){
                        Debug::ExecuteError("Asset con id: " + std::to_string(id) + " no existente en el subgrupo de audio.");
                    }
                    return it->second;
                } else if constexpr (std::is_same_v<T, ilen::graphics::IlenShader>) {
                    auto it = m_idToShaderAsset.find(id);
                    if (it == m_idToShaderAsset.end()){
                        Debug::ExecuteError("Asset con id: " + std::to_string(id) + " no existente en el subgrupo de shaders.");
                    }
                    return *it->second;
                } else if constexpr (std::is_same_v<T, ilen::graphics::IlenMaterial>) {
                    auto it = m_idToMaterialAsset.find(id);
                    if (it == m_idToMaterialAsset.end()){
                        Debug::ExecuteError("Asset con id: " + std::to_string(id) + " no existente en el subgrupo de materiales.");
                    }
                    return *it->second;
                } else {
                    // Esto previene errores de compilación si se llama con un tipo no soportado.
                    static_assert(!std::is_same_v<T, T>, "Tipo de asset no soportado en getAssetElement");
                }
            }

            template<typename T>
            const T& getAssetElement(const std::string& friendlyName) const {
                auto id = getAssetID(friendlyName);
                return getAssetElement<T>(id);
            };

            void loadManifiest();

            void manifestIsLoaded(){
                assert(isManifestLoaded == false && "Tratando de usar métodos del AssetsManager sin haber cargado el manifiesto.");
            };
        private:
            AssetsManager() = default;
            ~AssetsManager() = default;
            AssetsManager(const AssetsManager&) = delete;
            AssetsManager& operator=(const AssetsManager&) = delete;

            #if ILEN_DEBUG
            //Este método se ejecuta en modo Debug, e indexa los nuevos datos.
            void indexNewModels();
            void generateBinaryRepresentation(nlohmann::json& manifestData);
            ilen::ecs::components::MeshData loadModelData(std::filesystem::path modelPath);
            std::mutex fileWrite;
            #endif

            bool isManifestLoaded = false;
            std::unordered_map<std::string, uint32_t> m_friendlyNameToID;
            std::unordered_map<uint32_t, ModelAsset> m_idToModelAsset;
            std::unordered_map<uint32_t, TextureAsset> m_idToTextureAsset;
            std::unordered_map<uint32_t, AudioAsset> m_idToAudioAsset;
            std::unordered_map<uint32_t, std::shared_ptr<ilen::graphics::IlenShader>> m_idToShaderAsset;
            std::unordered_map<uint32_t, std::shared_ptr<ilen::graphics::IlenMaterial>> m_idToMaterialAsset;
            std::string m_manifestFileName;
    };
}

namespace ilen::binary::headers {
    struct IlenMeshHeader {
        std::string magicNumber; // ILEN_MESH -> HSEM_NELI
        uint32_t version;
        uint32_t vertexCount;
        uint32_t indexCount;
    };
}