#include <ilen/resources/texture_manager.h>
#include <ilen/resources/assets_elements.h>

namespace ilen::resources::texture {
    TextureManager::TextureManager(EngineContext* engineContext) {
        jobSystem = engineContext->p_jobSystem.get();
        renderer = engineContext->p_gameRenderer.get();
        assetsManager = engineContext->p_assetsManager.get();
    }

    std::weak_ptr<TextureGPULink> TextureManager::getTextureHandler(TextureID id){
        auto it = m_texBufferData.find(id);
        if(it != m_texBufferData.end()){
            //Texture ya en la VRAM, mandar a la app
            return it->second;
        }

        auto isLoading = m_texLoadingData.find(id);
        if(isLoading != m_texLoadingData.end()){
            //Se esta cargando
            return {};
        }

        auto textureObject = assetsManager->getAssetElement<TextureAsset>(id);

        std::future<TextureGPULink> future = jobSystem->SubmitJobWithResult([this, textureObject, id]() -> TextureGPULink {
            auto loadPath = ilen::core::FileManager::fileRoute(textureObject.assetPath, ILEN_FILE_TYPE_TEXTURE);

            std::ifstream readTex(loadPath, std::ios::binary);
            if(!readTex.is_open()){
                ILEN_ERROR_P(DEBUG_PREFIX, "No se pudo abrir la textura ubicada en %s", loadPath.c_str());
            }

            ilen::binary::headers::IlenTextureHeader texHeader{};
            readTex.read(reinterpret_cast<char*>(&texHeader), sizeof(texHeader));            

            if(std::string(texHeader.magicNumber) != "ERUTXET_NELI"){
                ILEN_ERROR_P(DEBUG_PREFIX, "La cabezera del texture ubicado en %s no es compatible con este programa", loadPath.c_str());
                return {};
            }

            auto activeCompression = renderer->getActualSupportedTextureCompressionFormat();
            IlenTexture textureData(readTex, texHeader, activeCompression);
            readTex.close();

            if(textureData.format == ilen::enumerations::ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_INVALID || textureData.pixelData.empty()){
                ILEN_ERROR_P(DEBUG_PREFIX, "La textura ubicada en %s no tiene un payload con datos comprimidos válidos para la GPU actual.", loadPath.c_str());
                return TextureGPULink{};
            }

            TextureGPULink link{};
            ilen::render::IlenGPUTextureDescriptor texDesc{};
            texDesc.width = textureData.width;
            texDesc.height = textureData.height;
            texDesc.mipLevels = textureData.mipLevels;
            texDesc.pixelFormat = textureData.format;
            texDesc.usage = ILEN_GPU_TEXTURE_USAGE_SHADER_READ;
            texDesc.memoryProperty = ILEN_GPU_MEMORY_PROPERTY_TYPE::ILEN_GPU_MEMORY_PROPERTY_PRIVATE;
            texDesc.label = "Texture_Asset_" + std::to_string(id);

            link.texture = renderer->createTexture(texDesc, textureData.pixelData.data());
            link.width = textureData.width;
            link.height = textureData.height;
            link.channels = textureData.channels;

            //De momento, usamos un sampler por defecto. Los datos del sampler deben estar con los metadatos de la textura, lo cual incluye sus sprites
            // 2. Asignar el Sampler pidiéndoselo al renderizador
            ilen::render::IlenGPUSamplerDescriptor samplerDesc{};
            samplerDesc.minFilter = ilen::enumerations::ILEN_GPU_SAMPLER_FILTER_TYPE::ILEN_GPU_SAMPLER_FILTER_LINEAR;
            samplerDesc.magFilter = ilen::enumerations::ILEN_GPU_SAMPLER_FILTER_TYPE::ILEN_GPU_SAMPLER_FILTER_LINEAR;
            samplerDesc.addressModeU = ilen::enumerations::ILEN_GPU_SAMPLER_ADDRESS_MODE::ILEN_GPU_SAMPLER_ADDRESS_REPEAT;
            samplerDesc.addressModeV = ilen::enumerations::ILEN_GPU_SAMPLER_ADDRESS_MODE::ILEN_GPU_SAMPLER_ADDRESS_REPEAT;
            samplerDesc.label = "Default_Linear_Repeat";

            link.sampler = renderer->createSampler(samplerDesc);

            link.atlasSprites = textureData.loadedAtlasSprites;

            return link;
        });

        m_texLoadingData.emplace(id, std::move(future));
        return {};
    }

    void TextureManager::UpdateProcess(){
        for(auto it = m_texLoadingData.begin(); it != m_texLoadingData.end();){
            auto& [id, future] = *it;

            if(future.wait_for(std::chrono::seconds(0)) == std::future_status::ready){
                TextureGPULink link = future.get();
                m_texBufferData[id] = std::make_shared<TextureGPULink>(std::move(link));
                m_texRuntimeID.emplace(id, m_nextRuntimeID++);
                it = m_texLoadingData.erase(it); 
            } else {
                ++it;
            }
        }
    }

    uint16_t TextureManager::getTextureRuntimeID(TextureID id) const {
        std::lock_guard lock(m_mutex);
        auto it = m_texRuntimeID.find(id);
        if(it != m_texRuntimeID.end()) {
            return it->second;
        }
        return 0; 
    }

    float TextureManager::LoadingStatus() {
        if(m_texLoadingData.size() > 0) {
            //Hay datos cargando
            lastTimeAmountOfElementLoading = m_texLoadingData.size() > lastTimeAmountOfElementLoading ? static_cast<uint32_t>(m_texLoadingData.size()) : lastTimeAmountOfElementLoading;
            return 1.0f - (static_cast<float>(m_texLoadingData.size()) / static_cast<float>(lastTimeAmountOfElementLoading));
        }else {
            lastTimeAmountOfElementLoading = 0;
            return 1.0f;
        }
    }
}

