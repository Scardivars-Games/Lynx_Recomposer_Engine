#pragma once

#include <ilen/headers/enumerations.h>
#include <filesystem>

namespace ilen::resources {
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
}