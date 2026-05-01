#include <ilen/resources/shaders-manager.h>
#include <ilen/headers/ilen-logs.h>
#include <ilen/core/engine-context.h>

std::unordered_map<uint32_t, ilen::graphics::IlenShader> shaderTest;

namespace ilen::resources::shaders {
    ShaderManager::ShaderManager(EngineContext* context){
        p_jobSystem = context->p_jobSystem.get();
        p_renderer = context->p_gameRenderer.get();
        p_assetsManager = context->p_assetsManager.get();

        //Precargar todos los shaders del motor para que esten disponibles al momento de crear este objeto.
        auto allShaders = p_assetsManager->getAllElementsOfType<ILEN_ASSET_TYPE_SHADER>();

        size_t totalShaders = allShaders.size();
        size_t numThreads = p_jobSystem->AvailableThreads();
        size_t chunkSize = (totalShaders + numThreads - 1) / numThreads;
        size_t numChunks = (totalShaders + chunkSize - 1) / chunkSize;

        std::vector shadersIterator(allShaders.begin(), allShaders.end());

        ilen::job_system::JobGroup group;

        for(size_t i = 0; i < numChunks; i++){
            size_t start = i * chunkSize;
            size_t end = std::min(start + chunkSize, totalShaders);
            
            p_jobSystem->SubmitJob([this, &shadersIterator, start, end]() {
                for(size_t j = start; j < end; j++){
                    const auto& shaderData = shadersIterator[j];
                    LoadShaderData(shaderData.second);
                }
            }, &group);
        }

        group.Wait();

        /*std::vector<std::future<void>> futures;

        for (auto const& shaderMap : allShaders)
        {
            std::future<void> future = p_jobSystem->SubmitJob([this, shaderMap](){
                auto shader = shaderMap.second;
                LoadShaderData(shader);
            });
            futures.push_back(std::move(future));
        }

        for (auto& future: futures) {
            future.wait();
        }*/
    }

    std::weak_ptr<IShader> ShaderManager::getShaderData(ShaderID id)
    {
        auto it = m_shaderData.find(id);

        if (it != m_shaderData.end())
        {
            return it->second;
        }

        //Miramos si ese ID esta asignado a un shader. Sino, enviamos shader de error.
        if (id == ilen::resources::ErrorShaderID)
        {
            ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "Shader por error no puede ser solicitado por esta via por seguridad del programa.");
            //Fin de ejecución
        }

        auto result = p_assetsManager->getAssetElement<ShaderAsset>(id);
        if (result.assetID == ilen::resources::ErrorShaderID)
        {
            return getShaderData(result.assetID); // ESTE SHADER DEBE DE ESTAR PRECARGADO A ESTA ALTURA. SINO SE CICLARA LA APP ETERNAMENTE. Necesito un seguro
        }

        //Cargar shader
        LoadShaderData(result);

        return getShaderData(id); //Ahora si debería estar OK
    }

    void ShaderManager::LoadShaderData(const ShaderAsset& asset)
    {
        std::filesystem::path shaderBinRoute = ilen::core::FileManager::fileRoute(asset.assetPath, ILEN_FILE_TYPE_ENGINE_SHADER);

        std::ifstream file(shaderBinRoute, std::ios::binary);

        if (!file.is_open())
        {
            ILEN_WARNING_P(DEBUG_PREFIX, "No se pudo abrir el archivo en la ruta: %s.", shaderBinRoute.c_str());
            return;
        }

        ilen::binary::headers::IlenShaderHeader header{};

        file.read(reinterpret_cast<char*>(&header), sizeof(ilen::binary::headers::IlenShaderHeader));

        if (std::string(header.magicNumber) != "REDAHS_NELI")
        {
            ILEN_ERROR_P(DEBUG_PREFIX, "Se solicito carga de un archivo que no tiene header válido de Shader. Ruta: %s", shaderBinRoute.c_str());
            return;
        }

        graphics::IlenShader engineDataShader(file, header);

        auto platformShader = p_renderer->createShader(engineDataShader);

        std::lock_guard lock(m_mutex);
        m_shaderData.emplace(asset.assetID, std::move(platformShader));
        //shaderTest.emplace(asset.assetID, engineDataShader);
    }
}

