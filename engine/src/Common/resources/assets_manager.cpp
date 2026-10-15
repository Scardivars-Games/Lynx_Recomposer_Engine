#include <fstream>
#include <lynx/resources/assets_manager.h>
#include <lynx/graphics/material.h>
#include <lynx/graphics/shader.h>
#include <lynx/core/filemanager.h>
#include <lynx/resources/texture_manager.h>
#include <set>
#include <vector>

using namespace lynx::resources;
using namespace lynx::core;

void AssetsManager::loadManifiest(EngineContext* engineContext) {
    context = engineContext;

    m_manifestFileName = "assets.lynxmanifest";

    auto manifestPath = FileManager::fileRoute(m_manifestFileName, LYNX_FILE_TYPE_ASSETS_MANIFEST);

    std::ifstream file(manifestPath, std::ios::binary);

    if(!file.is_open()){
        LYNX_EXECUTE_ERROR_P(DEBUG_PREFIX, "No se pudo abrir el manifiesto principal de assets del juego");
    }

    lynx::binary::headers::LynxManifestHeader header{};

    file.read(reinterpret_cast<char*>(&header),sizeof(lynx::binary::headers::LynxManifestHeader));

    if(std::string(header.magicNumber) != "TSEFINAM_NELI"){
        LYNX_EXECUTE_ERROR_P(DEBUG_PREFIX, "Header of Manifest file is not compatible by magic number");
    }

    m_friendlyNameToID.reserve(header.totalOfNamesToID);
    for(int i = 0; i < header.totalOfNamesToID; i++){
        uint32_t ID;
        uint32_t friendNameLength;

        file.read(reinterpret_cast<char*>(&ID), sizeof(ID));
        file.read(reinterpret_cast<char*>(&friendNameLength), sizeof(friendNameLength));

        std::string friendName(friendNameLength, '\0');
        file.read(&friendName[0], friendNameLength);
        
        m_friendlyNameToID.emplace(friendName, ID);
    }

    //Models
    m_idToModelAsset.reserve(header.totalOfModels);
    for(int i = 0; i < header.totalOfModels; i++){
        uint32_t ID;
        uint32_t pathNameLenght;

        file.read(reinterpret_cast<char*>(&ID), sizeof(ID));
        file.read(reinterpret_cast<char*>(&pathNameLenght), sizeof(pathNameLenght));

        std::string path(pathNameLenght, '\0');
        file.read(&path[0], pathNameLenght);

        m_idToModelAsset.emplace(ID, ModelAsset(ID, std::filesystem::path(path)));
    }

    //Textures
    m_idToTextureAsset.reserve(header.totalOfTextures);
    for(int i = 0; i < header.totalOfTextures; i++){
        uint32_t ID;
        uint32_t pathNameLenght;

        file.read(reinterpret_cast<char*>(&ID), sizeof(ID));
        file.read(reinterpret_cast<char*>(&pathNameLenght), sizeof(pathNameLenght));

        std::string path(pathNameLenght, '\0');
        file.read(&path[0], pathNameLenght);

        m_idToTextureAsset.emplace(ID, TextureAsset(ID, std::filesystem::path(path)));
    }

    //Shaders
    m_idToShaderAsset.reserve(header.totalOfShaders);
    for(int i = 0; i < header.totalOfShaders; i++)
    {
        uint32_t ID;
        uint32_t pathNameLenght;

        file.read(reinterpret_cast<char*>(&ID), sizeof(ID));
        file.read(reinterpret_cast<char*>(&pathNameLenght), sizeof(pathNameLenght));

        std::string path(pathNameLenght, '\0');
        file.read(&path[0], pathNameLenght);

        m_idToShaderAsset.emplace(ID, ShaderAsset(ID, std::filesystem::path(path)));
    }

    //Materials
    m_idToMaterialAsset.reserve(header.totalOfMaterials);
    for(int i = 0; i < header.totalOfMaterials; i++)
    {
        uint32_t ID;
        uint32_t pathNameLenght;

        file.read(reinterpret_cast<char*>(&ID), sizeof(ID));
        file.read(reinterpret_cast<char*>(&pathNameLenght), sizeof(pathNameLenght));

        std::string path(pathNameLenght, '\0');
        file.read(&path[0], pathNameLenght);

        m_idToMaterialAsset.emplace(ID, MaterialAsset(ID, std::filesystem::path(path)));
    }

    isManifestLoaded = true;
    LYNX_LOG_P(DEBUG_PREFIX, "MANIFIESTO CARGADO EXITOSAMENTE. Total de items cargados: %zu", m_friendlyNameToID.size());
}

uint32_t AssetsManager::getAssetID(const std::string& friendlyName) const {
    auto element = m_friendlyNameToID.find(friendlyName);
    if(element == m_friendlyNameToID.end()){
        LYNX_EXECUTE_ERROR_P(DEBUG_PREFIX, "El asset con nombre %s no existe en los registros. Revisar el nombre.", friendlyName.c_str());
    }
    return element->second; //Regresa el id :)
}

float AssetsManager::getAssetLoadStatus() {
    float totalResult = context->p_meshManager->LoadingStatus();
    totalResult += context->p_renderPipelineManager->LoadingStatus();
    totalResult += context->p_textureManager->LoadingStatus();

    return totalResult / 3.0f;
}