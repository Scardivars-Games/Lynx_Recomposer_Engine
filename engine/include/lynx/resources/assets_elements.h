#pragma once

#include <lynx/headers/enumerations.h>
#include <filesystem>

namespace lynx::resources {
    using namespace lynx::enumerations;
    template<LYNX_ASSET_TYPE T>
    struct AssetElement {
        LYNX_ASSET_TYPE type = T;
        uint32_t assetID;
        std::filesystem::path assetPath;

        AssetElement() = delete;
        AssetElement(uint32_t id, std::filesystem::path path) : assetID(id), assetPath(path) {} ;
    };

    using ModelAsset = AssetElement<LYNX_ASSET_TYPE_MODEL>;
    using TextureAsset = AssetElement<LYNX_ASSET_TYPE_TEXTURE>;
    using MaterialAsset = AssetElement<LYNX_ASSET_TYPE_MATERIAL>;
    using AudioAsset = AssetElement<LYNX_ASSET_TYPE_AUDIO>;
    using ShaderAsset = AssetElement<LYNX_ASSET_TYPE_SHADER>;
}