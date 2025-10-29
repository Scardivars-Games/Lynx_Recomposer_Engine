//
// Created by Owen Daniel Fernández Chacón on 26/10/25.
//

#include <ilen/resources/materials-manager.h>

namespace ilen::resources::materials {
    MaterialManager::MaterialManager(EngineContext *context) {
        p_jobSystem = context->p_jobSystem.get();
        p_assetsManager = context->p_assetsManager.get();

        auto allMaterials = p_assetsManager->getAllElementsOfType<ILEN_ASSET_TYPE_MATERIAL>();

        std::vector<std::future<void>> futures;

        for (auto materialData: allMaterials) {
           std::future<void> future = p_jobSystem->SubmitJob([this, materialData]() {
                LoadMaterialData(materialData.second);
           });

            futures.push_back(std::move(future));
        }

        for (auto& future: futures) {
            future.wait();
        }
    }

    void MaterialManager::LoadMaterialData(const MaterialAsset &asset) {
        std::filesystem::path matBinarySource = ilen::core::FileManager::fileRoute(asset.assetPath, ILEN_FILE_TYPE_MATERIAL);

        std::ifstream file(matBinarySource);

        if (!file.is_open())
        {
            ILEN_WARNING_P(DEBUG_PREFIX, "No se pudo abrir el archivo en la ruta: %s.", matBinarySource.c_str());
            return;
        }

        ilen::binary::headers::IlenMaterialHeader header{};
        file.read(reinterpret_cast<char*>(&header), sizeof(header));

        if (std::string(header.magicNumber) != "LAIRETAM_NELI") {
            ILEN_ERROR_P(DEBUG_PREFIX, "Binary in route %s is not a valid material.", matBinarySource.c_str());
            return;
        }

        IlenMaterial material(file, header);

        std::lock_guard lock(m_mutex);
        m_materials.emplace(asset.assetID, material);
    }

    const IlenMaterial& MaterialManager::getMaterial(MaterialID id) {
        auto it  = m_materials.find(id);

        if (it != m_materials.end()) {
            return it->second;
        }

        //No se encontro el material, tratamos de cargarlo si esta presente en el manifiesto. Sino, el manifiesto regresa un material para errores.
        auto asset = p_assetsManager->getAssetElement<MaterialAsset>(id);
        if (asset.assetID == ErrorMaterialID) {
            auto errorMat = m_materials.find(ErrorMaterialID);
            return errorMat->second;
        }

        LoadMaterialData(asset);
        auto finalSearch = m_materials.find(id);
        return finalSearch->second;
    }

}