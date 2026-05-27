//
// Created by Owen Daniel Fernández Chacón on 26/10/25.
//

#include <ilen/resources/materials-manager.h>
#include <ilen/core/engine-context.h>


namespace ilen::resources::materials {
    MaterialManager::MaterialManager(EngineContext *context) {
        p_jobSystem = context->p_jobSystem.get();
        p_assetsManager = context->p_assetsManager.get();

        auto allMaterials = p_assetsManager->getAllElementsOfType<ILEN_ASSET_TYPE_MATERIAL>();
    
        //std::vector<std::future<void>> futures;
        size_t totalMats = allMaterials.size();
        if(totalMats == 0) return;

        size_t numThreads = p_jobSystem->AvailableThreads();
        size_t chunckSize = (totalMats + numThreads - 1) / numThreads;
        size_t numChunks = (totalMats + chunckSize - 1) / chunckSize;

        ilen::job_system::JobGroup group;

        //Guardamos una lista con los iteradores del mapa de allMaterials, para acceder más rapido a los datos y de forma segura
        std::vector materialIterators(allMaterials.begin(), allMaterials.end());

        for(size_t i = 0; i < numChunks; i++){
            size_t start = i * chunckSize;
            size_t end = std::min(start + chunckSize, totalMats);

            p_jobSystem->SubmitJob([this, &materialIterators, start, end] () {
                for(size_t j = start; j < end; j++) {
                    const auto& matData = materialIterators[j];
                    LoadMaterialData(matData.second);
                }
            }, &group);
        }

        group.Wait();

        //REWORKING
       /*for (auto materialData: allMaterials) {
           std::future<void> future = p_jobSystem->SubmitJob([this, materialData]() {
                LoadMaterialData(materialData.second);
           });

            futures.push_back(std::move(future));
        }

        for (auto& future: futures) {
            future.wait();
        }*/
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
        m_materialsRuntime.emplace(asset.assetID, m_nextRuntimeID);
        m_nextRuntimeID++;
    }

    const IlenMaterial& MaterialManager::getMaterial(MaterialID id) {
        std::lock_guard lock(m_mutex);
        auto it  = m_materials.find(id);

        if (it != m_materials.end()) {
            return it->second;
        }

        //No se encontro el material, tratamos de cargarlo si esta presente en el manifiesto. Sino, el manifiesto regresa un material para errores.
        auto asset = p_assetsManager->getAssetElement<MaterialAsset>(id);
        if (asset.assetID == PredefinedAssetsID::ErrorMaterialID) {
            auto errorMat = m_materials.find(PredefinedAssetsID::ErrorMaterialID);
            if (errorMat == m_materials.end()) {
                ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "FATAL ERROR! Error during a material loading, and the predefined error material was not available. Please, reinstall the app.");
                //End of APP execution
            }

            return errorMat->second;
        }

        LoadMaterialData(asset);
        auto finalSearch = m_materials.find(id);
        return finalSearch->second;
    }

    uint16_t MaterialManager::getRuntimeMaterial(MaterialID id) const {
        std::lock_guard lock(m_mutex);
        auto it = m_materialsRuntime.find(id);
        if (it != m_materialsRuntime.end()) {
            return it->second;
        }

        return 0; //Fallback value, no impide el renderizado, pero hace que ese material pierda eficiencia
    }
}