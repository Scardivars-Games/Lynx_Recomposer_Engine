#pragma once

#include <string>
#include <memory>
#include <ilen/headers/ilen-logs.h>
#include <ilen/render/render-component.h>
#include <ilen/headers/enumerations.h>
#include <ilen/core/lifetime.h>

#if ILEN_DEBUG && ILEN_DESKTOP
#include <ilen/job-system/job-system.h>
#include <external/json/json.hpp>
#endif

namespace ilen {
    struct EngineContext;
}

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
            //Requisitos del arhcivo de manifiesto: 1 solo archivo por proyecto. Cada tipo de dato esta en estructuras diferentes. Los IDs no son repetitivos, aunque esten en diferentes estructuras, los ID son únicos como si de elemento global se tratase.
            //Estructura del manifiesto: - totalOfElements : uint32_t (total de datos disponibles en el proyecto). Estructuras : - ID, -StrigName -FilePath.
            uint32_t getAssetID(const std::string& friendlyName) const;

            template<typename T>
            const T& getAssetElement(const uint32_t& id) const {
                if constexpr (std::is_same_v<T, ModelAsset>) {
                    auto it = m_idToModelAsset.find(id);
                    if (it == m_idToModelAsset.end()){
                        ILEN_EXECUTE_ERROR_P("ASSET_MANAGER", "Asset con id: %d no existente en el subgrupo de modelos.", id);
                    }
                    return it->second;
                } else if constexpr (std::is_same_v<T, TextureAsset>) {
                    auto it = m_idToTextureAsset.find(id);
                    if (it == m_idToTextureAsset.end()){
                        ILEN_EXECUTE_ERROR_P("ASSET_MANAGER", "Asset con id: %d no existente en el subgrupo de texturas.", id);
                    }
                    return it->second;
                } else if constexpr (std::is_same_v<T, AudioAsset>) {
                    auto it = m_idToAudioAsset.find(id);
                    if (it == m_idToAudioAsset.end()){
                        ILEN_EXECUTE_ERROR_P("ASSET_MANAGER", "Asset con id: %d no existente en el subgrupo de audio.", id);
                    }
                    return it->second;
                } else if constexpr (std::is_same_v<T, ilen::graphics::IlenShader>) {
                    auto it = m_idToShaderAsset.find(id);
                    if (it == m_idToShaderAsset.end()){
                        ILEN_EXECUTE_ERROR_P("ASSET_MANAGER", "Asset con id: %d no existente en el subgrupo de shaders.", id);
                    }
                    return *it->second;
                } else if constexpr (std::is_same_v<T, ilen::graphics::IlenMaterial>) {
                    auto it = m_idToMaterialAsset.find(id);
                    if (it == m_idToMaterialAsset.end()){
                        ILEN_EXECUTE_ERROR_P("ASSET_MANAGER", "Asset con id: %d no existente en el subgrupo de materiales.", id);
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

            void loadManifiest(EngineContext* engineContext);

            void manifestIsLoaded(){
                assert(isManifestLoaded == false && "Tratando de usar métodos del AssetsManager sin haber cargado el manifiesto.");
            };
        private:

            #if ILEN_DEBUG
            //Este método se ejecuta en modo Debug, e indexa los nuevos datos.
            void indexNewModels();
            void generateBinaryRepresentation(nlohmann::json& manifestData);
            ilen::ecs::components::MeshData loadModelData(std::filesystem::path modelPath);
            std::mutex fileWrite;
            ilen::job_system::JobSystem* jobSystem;
            #endif

            bool isManifestLoaded = false;
            std::unordered_map<std::string, uint32_t> m_friendlyNameToID;
            std::unordered_map<uint32_t, ModelAsset> m_idToModelAsset;
            std::unordered_map<uint32_t, TextureAsset> m_idToTextureAsset;
            std::unordered_map<uint32_t, AudioAsset> m_idToAudioAsset;
            std::unordered_map<uint32_t, std::shared_ptr<ilen::graphics::IlenShader>> m_idToShaderAsset;
            std::unordered_map<uint32_t, std::shared_ptr<ilen::graphics::IlenMaterial>> m_idToMaterialAsset;
            std::string m_manifestFileName;

            const char* DEBUG_PREFIX = "ASSET_MANAGER";
    };
}

namespace ilen::binary::headers {
    struct IlenMeshHeader {
        char magicNumber[20]; // ILEN_MESH -> HSEM_NELI
        uint32_t version;
        uint32_t vertexCount;
        uint32_t indexCount;
    };

    struct IlenManifestHeader {
        char magicNumber[20]; //ILEN_MANIFEST -> TSEFINAM_NELI
        uint32_t version;
        uint32_t totalOfNamesToID;
        uint32_t totalOfModels;
        uint32_t totalOfTextures;
    };
}