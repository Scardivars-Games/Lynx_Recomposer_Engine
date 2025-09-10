#pragma once

#include <string>
#include <set>
#include <vector>
#include <external/json/json.hpp>
#include <ilen/core/logs.h>

//Administra una base de datos, en formato JSON, con cada objeto y elemento que administre el motor. Su comportamiento será el de un Singleton.
namespace ilen::core {
    enum AssetType {
        ILEN_ASSET_TYPE_MODEL,
        ILEN_ASSET_TYPE_TEXTURE,
        ILEN_ASSET_TYPE_MATERIAL,
        ILEN_ASSET_TYPE_AUDIO,
        ILEN_ASSET_TYPE_SHADER,
        //No se incluye shader entre los tipos de assets, ya que el motor no le asigna los shaders a las entidades, sino los materiales.
        //Aún así, este objeto si mantiene una lista con los id y direcciones de los shaders para que el material se pueda crear y asignar.
    };

    template<AssetType T>
    struct AssetElement {
        AssetType type = T;
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

            template<AssetType T>
            AssetElement<T> getAssetElement(const uint32_t& id) const {
                AssetElement<T> selectedAsset;
                switch (T)
                {
                    case ILEN_ASSET_TYPE_MODEL:
                        selectedAsset = m_idToModelAsset.find(id);
                        if (selectedAsset == m_idToModelAsset.end()){
                            Debug::ExecuteError("Asset con id: " + std::to_string(id) + " no existente en el subgrupo de modelos.");
                        }
                        break;
                    case ILEN_ASSET_TYPE_TEXTURE:
                        selectedAsset = m_idToTextureAsset.find(id);
                        if (selectedAsset == m_idToTextureAsset.end()){
                            Debug::ExecuteError("Asset con id: " + std::to_string(id) + " no existente en el subgrupo de texturas.");
                        }
                        break;
                    case ILEN_ASSET_TYPE_MATERIAL:
                        selectedAsset = m_idToMaterialAsset.find(id);
                        if (selectedAsset == m_idToMaterialAsset.end()){
                            Debug::ExecuteError("Asset con id: " + std::to_string(id) + " no existente en el subgrupo de materiales.");
                        }
                        break;
                    case ILEN_ASSET_TYPE_AUDIO:
                        selectedAsset = m_idToAudioAsset.find(id);
                        if (selectedAsset == m_idToAudioAsset.end()){
                            Debug::ExecuteError("Asset con id: " + std::to_string(id) + " no existente en el subgrupo de audio.");
                        }
                        break;
                    case ILEN_ASSET_TYPE_SHADER:
                        selectedAsset = m_idToShaderAsset.find(id);
                        if (selectedAsset == m_idToShaderAsset.end()){
                            Debug::ExecuteError("Asset con id: " + std::to_string(id) + " no existente en el subgrupo de shaders.");
                        }
                        break;
                    default:
                        Debug::ExecuteError("El tipo pasado no está implementado, o no existe en el contexto del motor.");
                }

                return selectedAsset;
            };

            template<AssetType T>
            AssetElement<T> getAssetElement(const std::string& friendlyName) const {
                auto id = getAssetID(friendlyName);
                return getAssetElement<T>(id);
            };

            void loadManifiest(const std::string& manifestName);

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
            void indexNewModels(nlohmann::json& manifestData);
            #endif

            bool isManifestLoaded = false;
            std::unordered_map<std::string, uint32_t> m_friendlyNameToID;
            std::unordered_map<uint32_t, ModelAsset> m_idToModelAsset;
            std::unordered_map<uint32_t, TextureAsset> m_idToTextureAsset;
            std::unordered_map<uint32_t, MaterialAsset> m_idToMaterialAsset;
            std::unordered_map<uint32_t, AudioAsset> m_idToAudioAsset;
            std::unordered_map<uint32_t, ShaderAsset> m_idToShaderAsset;
            std::string m_manifestFileName;
    };
}