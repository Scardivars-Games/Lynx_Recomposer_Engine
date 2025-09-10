#include <ilen/core/assets-manager.h>
#include <ilen/core/logs.h>
#include <ilen/core/filemanager.h>



//CONSTANTE TIPOS TO AVOID TYPING ERRORS
const std::string ILEN_ASSET_MANIFEST_TOTAL_OF_ELEMENTS = "totalOfElements";
const std::string ILEN_ASSET_MANIFEST_ASSET_ID = "id";
const std::string ILEN_ASSET_MANIFEST_ASSET_NAME = "name";
const std::string ILEN_ASSET_MANIFEST_ASSET_PATH = "path";
const std::string ILEN_ASSET_MANIFEST_MODEL = "models";
const std::string ILEN_ASSET_MANIFEST_TEXTURES = "textures";
const std::string ILEN_ASSET_MANIFEST_MATERIALS = "materials";
const std::string ILEN_ASSET_MANIFEST_AUDIO = "audio";
const std::string ILEN_ASSET_MANIFEST_SHADERS = "shaders";


const std::vector<std::string> ILEN_ASSET_MANIFEST_ASSETS_TYPES = { ILEN_ASSET_MANIFEST_MODEL, ILEN_ASSET_MANIFEST_TEXTURES, ILEN_ASSET_MANIFEST_AUDIO, ILEN_ASSET_MANIFEST_MATERIALS, ILEN_ASSET_MANIFEST_SHADERS };

using namespace ilen::core;
using json = nlohmann::json;

std::string generateFriendlyName(const std::string& filename){
    std::string name = std::filesystem::path(filename).stem().string();
    return name;
}

template<typename AssetType>
void writeToMap(const json& data, std::unordered_map<std::string, uint32_t>& nameToIndexMap, std::unordered_map<uint32_t, AssetType>& idToAssetMap){
    for (const auto& assetData : data){
        uint32_t id = assetData[ILEN_ASSET_MANIFEST_ASSET_ID];
        std::string name = assetData[ILEN_ASSET_MANIFEST_ASSET_NAME];
        std::string path = assetData[ILEN_ASSET_MANIFEST_ASSET_PATH];
        nameToIndexMap[name] = id;
        idToAssetMap.emplace(id, AssetType(id, path));
    }
}

AssetsManager& AssetsManager::getInstance() {
    static AssetsManager instance;
    return instance;
}

void AssetsManager::loadManifiest(const std::string& manifestName) {
    m_manifestFileName = manifestName;

    auto manifestPath = FileManager::fileRoute(manifestName, ILEN_ASSETS_MANIFEST);

    std::ifstream file(manifestPath);
    json data;

    if(file.is_open()){
        file >> data;
    } else {
        Debug::Log("No se pudo abrir el archivo del manifiesto, o no existe en la ruta: " + manifestPath.string(), "ASSETS_MANAGER");
        #ifdef ILEN_DEBUG
        data[ILEN_ASSET_MANIFEST_TOTAL_OF_ELEMENTS] = 0;
        data[ILEN_ASSET_MANIFEST_MODEL] = json::array(); //Inicializamos si estamos en Debug... No tiene sentido que se cree un nuevo indice en una build release
        data[ILEN_ASSET_MANIFEST_TEXTURES] = json::array();
        data[ILEN_ASSET_MANIFEST_MATERIALS] = json::array();
        data[ILEN_ASSET_MANIFEST_AUDIO] = json::array();
        data[ILEN_ASSET_MANIFEST_SHADERS] = json::array();
        #endif
    }

    #if ILEN_DEBUG && ILEN_DESKTOP //En dispositivod móviles o consolas esto no tiene sentido. Realiza el trabajo usando los archivos fuente
    indexNewModels(data); //Actualizamos el manifiesto con los nuevos archivos no indexados
    #endif

    //Model loading
    if(data.contains(ILEN_ASSET_MANIFEST_MODEL)){
        writeToMap(data[ILEN_ASSET_MANIFEST_MODEL], m_friendlyNameToID, m_idToModelAsset);
    }

    //Texture loading
    if(data.contains(ILEN_ASSET_MANIFEST_TEXTURES)){
        writeToMap(data[ILEN_ASSET_MANIFEST_TEXTURES], m_friendlyNameToID, m_idToTextureAsset);
    }

    //Material loading
    if(data.contains(ILEN_ASSET_MANIFEST_MATERIALS)){
        writeToMap(data[ILEN_ASSET_MANIFEST_MATERIALS], m_friendlyNameToID, m_idToMaterialAsset);
    }
     
    //Audio loading
    if(data.contains(ILEN_ASSET_MANIFEST_AUDIO)){
        writeToMap(data[ILEN_ASSET_MANIFEST_AUDIO], m_friendlyNameToID, m_idToAudioAsset);
    }

    //Shader loading
    if(data.contains(ILEN_ASSET_MANIFEST_SHADERS)){
        writeToMap(data[ILEN_ASSET_MANIFEST_SHADERS], m_friendlyNameToID, m_idToShaderAsset);
    }

    isManifestLoaded = true;
    Debug::Log("MANIFIESTO CARGADO EXITOSAMENTE. Total de items cargados: " + std::to_string(m_friendlyNameToID.size()), "ASSETS_MANAGER");
}

#if ILEN_DESKTOP && ILEN_DEBUG //No se necesita este símbolo en un sistema que no sirva para programar
void AssetsManager::indexNewModels(json& manifestData){
    Debug::Warning("Indexando nuevos objetos en modo Debug", "ASSETS_MANAGER");

    uint32_t maxID = manifestData.value(ILEN_ASSET_MANIFEST_TOTAL_OF_ELEMENTS, 0);
    std::set<std::string> existingPaths;
    bool folderReorganized = false;

    //Necesito una base de datos de extensiones soportadas y que tipo de asset es
    std::unordered_map<std::string, std::string> extensionToAssetType = {
        //Models
        {".obj", "models"},
        {".glb", "models"},
        {".mtl", "models"}, //Materials de un .obj
        //Textures
        {".png", "textures"},
        {".jpg", "textures"},
        {".jpeg", "textures"},
        //Shaders
        {".metal", "shaders"},
        {".hlsl", "shaders"},
        {".glsl", "shaders"},
        {".spv", "shaders"},
        //Audio
        {".mp3", "audio"},
        {".wav", "audio"},
        {".flac", "audio"},
        //Materials
        {".ilenmat", "materials"}
    };
    std::unordered_map<std::string, std::string> folderToManifestAssetGroup = {
        {"models", ILEN_ASSET_MANIFEST_MODEL},
        {"textures", ILEN_ASSET_MANIFEST_TEXTURES},
        {"shaders", ILEN_ASSET_MANIFEST_SHADERS},
        {"audio", ILEN_ASSET_MANIFEST_AUDIO},
        {"materials", ILEN_ASSET_MANIFEST_MATERIALS},
    };
    std::string unsupportedFolder = "unsupported";

    //Obteniendo todas las rutas declaradas anteriormente en el manifiesto.
    for(const auto& assetType : ILEN_ASSET_MANIFEST_ASSETS_TYPES) {
        if(manifestData.contains(assetType)){
            for(const auto& assetData : manifestData[assetType]){
                existingPaths.insert(assetData.value(ILEN_ASSET_MANIFEST_ASSET_PATH, ""));
            }
        }
    }
    std::filesystem::path engineSourceFolder(ILEN_SOURCES_FOLDER);
    std::filesystem::path sourceAssetFolders = engineSourceFolder / "assets";

    //creamos un nuevo vector para almacenar todos las rutas de archivos encontrados en assets
    std::vector<std::filesystem::path> filesToIndex;
    for(const auto& entry: std::filesystem::recursive_directory_iterator(sourceAssetFolders)){
        if(entry.is_regular_file() && entry.path().filename() != m_manifestFileName){
            filesToIndex.push_back(entry.path());
        }
    }

    //Organizamos los archivos en su carpeta correcta según el formato de su extensión.
    for(const auto& currentPath : filesToIndex){
        std::string fileExtension = currentPath.extension().string();
        std::string targetFolder = unsupportedFolder;

        auto it = extensionToAssetType.find(fileExtension);
        if(it != extensionToAssetType.end()){
            targetFolder = it->second;
        }

        std::filesystem::path relativeToAssets = std::filesystem::relative(currentPath, sourceAssetFolders);
        std::string firstFolder = relativeToAssets.begin()->string(); //Primera carpeta de la ruta relativa (revisa que este correctamente ubicado)

        if(firstFolder != targetFolder){
            //Revisar si no es una carpeta de tipo del motor
            bool firstFolderIsType = false;

            for(const auto& extensions : extensionToAssetType){
                if(extensions.second == firstFolder){
                    firstFolderIsType = true;
                    break;
                }
            }         
            
            if(firstFolderIsType){
                relativeToAssets = std::filesystem::relative(relativeToAssets, firstFolder);
            }

            std::filesystem::path finalDestinationPath = sourceAssetFolders / targetFolder / relativeToAssets;

            if(!std::filesystem::exists(finalDestinationPath.parent_path())){
                //Creamos el directorio si no existe
                std::filesystem::create_directories(finalDestinationPath.parent_path());
            }

            //Copiamos el recurso a su nueva carpeta
            std::filesystem::rename(currentPath, finalDestinationPath);
            folderReorganized = true;
        }
    }

    //TODO: Checkear porque parece que no siempre al volver a compilar actualiza las carpetas.
    assert(folderReorganized == false && "Se movieron varios assets a su carpeta correcta. Reinicie nuevamente la aplicación para que use las rutas correctas de archivos y se escriba un nuevo manifiesto.");

    bool needSaveNewManifest = false;
    for(const auto& currentPath : filesToIndex){ //Si llego hasta aquí significa que la lista anterior ta joia.
        std::filesystem::path relativeToAssets = std::filesystem::relative(currentPath, sourceAssetFolders);
        
        if(existingPaths.count(relativeToAssets.generic_string())) continue; //Si existe el path en el manifiesto, saltar al próximo.
        
        std::string extension = relativeToAssets.extension().string();
        auto it = extensionToAssetType.find(extension);

        //Guardar si la extensión existe y si esta en la carpeta correcta. Técnicamente no es necesario, pero no esta demás la comprobación, además de que esto solo esta en DEBUG.
        if(it != extensionToAssetType.end() && relativeToAssets.begin()->string() == it->second){
            needSaveNewManifest = true;
            
            uint32_t newID = ++maxID;
            std::string name = generateFriendlyName(currentPath.filename().string());

            json newAsset;
            newAsset[ILEN_ASSET_MANIFEST_ASSET_ID] = newID;
            newAsset[ILEN_ASSET_MANIFEST_ASSET_NAME] = name;
            newAsset[ILEN_ASSET_MANIFEST_ASSET_PATH] = relativeToAssets.generic_string();
            manifestData[folderToManifestAssetGroup.find(it->second)->second].push_back(newAsset);
        }
    }

    //Escribir nuevo manifiesto con los nuevos datos si es necesario
    if(needSaveNewManifest){
        manifestData[ILEN_ASSET_MANIFEST_TOTAL_OF_ELEMENTS] = maxID;
        
        auto manifestPath = sourceAssetFolders / "manifests" / m_manifestFileName;
        if(!std::filesystem::exists(manifestPath.parent_path()))
            std::filesystem::create_directories(manifestPath.parent_path());

        std::ofstream outFile(manifestPath);
        outFile << manifestData.dump(2);
        outFile.close();

    }

    assert(needSaveNewManifest == false && "Se escribió un nuevo manifiesto. Vuelve a iniciar la app para que ahora si funcione de forma correcta.");


    Debug::Warning("Indexación de nuevos elementos completada", "ASSETS_MANAGER");
}
#endif

uint32_t AssetsManager::getAssetID(const std::string& friendlyName) const {
    auto element = m_friendlyNameToID.find(friendlyName);
    if(element == m_friendlyNameToID.end()){
        Debug::ExecuteError("El asset con nombre " + friendlyName + " no existe en los registros. Revisar el nombre.", "ASSETS_MANAGER");
    }
    return element->second; //Regresa el id :)
}
