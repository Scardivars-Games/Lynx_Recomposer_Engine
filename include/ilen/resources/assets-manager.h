#pragma once

#include <string>
#include <memory>
#include <ilen/headers/ilen-logs.h>
#include <ilen/render/render-component.h>
#include <ilen/headers/enumerations.h>
#include <ilen/core/lifetime.h>
#include <ilen/graphics/meshes.h>
#include <ilen/resources/assets-elements.h>

/*
#if ILEN_DEBUG && ILEN_DESKTOP
#include <ilen/job-system/job-system.h>
#include <external/json/json.hpp>
#include <ilen/resources/mesh-manager.h>
#endif
*/

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
namespace ilen::resources {
    //ID de datos predefinidos
    static constexpr uint32_t ErrorShaderID = 6;
    static constexpr uint32_t ErrorMaterialID = 4;
    static constexpr uint32_t QuadMeshID = 2;
    static constexpr uint32_t PostProcessPresentShaderID = 8;
    static constexpr uint32_t PostProcessCalculateShaderID = 9;
    static constexpr uint32_t ErrorTextureID = 11;


    class AssetsManager {
        public:
            //Requisitos del arhcivo de manifiesto: 1 solo archivo por proyecto. Cada tipo de dato esta en estructuras diferentes. Los IDs no son repetitivos, aunque esten en diferentes estructuras, los ID son únicos como si de elemento global se tratase.
            //Estructura del manifiesto: - totalOfElements : uint32_t (total de datos disponibles en el proyecto). Estructuras : - ID, -StrigName -FilePath.
            [[nodiscard]] uint32_t getAssetID(const std::string& friendlyName) const;

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
                } else if constexpr (std::is_same_v<T, ShaderAsset>) {
                    auto it = m_idToShaderAsset.find(id);
                    if (it == m_idToShaderAsset.end()){
                        ILEN_ERROR_P("ASSET_MANAGER", "Asset con id: %d no existente en el subgrupo de shaders.", id);
                        auto errorIT = m_idToShaderAsset.find(ErrorShaderID);
                        if (errorIT == m_idToShaderAsset.end())
                        {
                            ILEN_EXECUTE_ERROR_P("ASSET_MANAGER", "Error grave al cargar el shader de error por defecto, como fallback por shader inválido. Reinstale el programa, y si el error persiste, póngase en contacto con soporte para buscar una solución");
                        }
                        return errorIT->second;
                    }
                    return it->second;
                } else if constexpr (std::is_same_v<T, MaterialAsset>) {
                    auto it = m_idToMaterialAsset.find(id);
                    if (it == m_idToMaterialAsset.end()){
                        ILEN_ERROR_P("ASSET_MANAGER", "Asset con id: %d no existente en el subgrupo de materials.", id);
                        auto errorIT = m_idToMaterialAsset.find(ErrorMaterialID);
                        if (errorIT == m_idToMaterialAsset.end())
                        {
                            ILEN_EXECUTE_ERROR_P("ASSET_MANAGER", "Error grave al cargar el shader de error por defecto, como fallback por material inválido. Reinstale el programa, y si el error persiste, póngase en contacto con soporte para buscar una solución");
                        }
                        return errorIT->second;
                    }
                    return it->second;
                } else {
                    // Esto previene errores de compilación si se llama con un tipo no soportado.
                    static_assert(!std::is_same_v<T, T>, "Tipo de asset no soportado en getAssetElement");
                    return nullptr;
                }
            }

            template<typename T>
            const T& getAssetElement(const std::string& friendlyName) const {
                auto id = getAssetID(friendlyName);
                return getAssetElement<T>(id);
            }

            template<ILEN_ASSET_TYPE T>
            std::unordered_map<uint32_t, AssetElement<T>> getAllElementsOfType() const {
                if constexpr (T == ILEN_ASSET_TYPE_MODEL)
                {
                    return m_idToModelAsset;
                }else if constexpr (T == ILEN_ASSET_TYPE_TEXTURE)
                {
                    return m_idToTextureAsset;
                }else if constexpr (T == ILEN_ASSET_TYPE_AUDIO)
                {
                    return m_idToAudioAsset;
                }else if constexpr (T == ILEN_ASSET_TYPE_SHADER)
                {
                    return m_idToShaderAsset;
                }else if constexpr (T == ILEN_ASSET_TYPE_MATERIAL)
                {
                    return m_idToMaterialAsset;
                }
            }


            void loadManifiest(EngineContext* engineContext);

            void manifestIsLoaded() const
            {
                assert(isManifestLoaded == false && "Tratando de usar métodos del AssetsManager sin haber cargado el manifiesto.");
            };
        private:
            bool isManifestLoaded = false;
            std::unordered_map<std::string, uint32_t> m_friendlyNameToID;
            std::unordered_map<uint32_t, ModelAsset> m_idToModelAsset;
            std::unordered_map<uint32_t, TextureAsset> m_idToTextureAsset;
            std::unordered_map<uint32_t, AudioAsset> m_idToAudioAsset;
            std::unordered_map<uint32_t, ShaderAsset> m_idToShaderAsset;
            std::unordered_map<uint32_t, MaterialAsset> m_idToMaterialAsset;
            std::string m_manifestFileName;

            const char* DEBUG_PREFIX = "ASSET_MANAGER";
    };
}
