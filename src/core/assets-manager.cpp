#include <fstream>
#include <ilen/core/assets-manager.h>
#include <ilen/graphics/material.h>
#include <ilen/graphics/shader.h>
#include <ilen/core/filemanager.h>
#include <set>
#include <vector>
#if ILEN_DEBUG && ILEN_DESKTOP
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <ilen/core/time.h>
#endif

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
    {".metal", "shader-code"},
    {".hlsl", "shader-code"},
    {".glsl", "shader-code"},
    {".spv", "shader-code"},
    {".ilenshader", "shaders"},
    //Audio
    {".mp3", "audio"},
    {".wav", "audio"},
    {".flac", "audio"},
    //Materials
    {".ilenmat", "materials"},
    {".ilenmanifest", "manifests"}
};

std::unordered_map<std::string, std::string> folderToManifestAssetGroup = {
    {"models", ILEN_ASSET_MANIFEST_MODEL},
    {"textures", ILEN_ASSET_MANIFEST_TEXTURES},
    {"shaders", ILEN_ASSET_MANIFEST_SHADERS},
    {"shader-code", ILEN_ASSET_MANIFEST_SHADERS},
    {"audio", ILEN_ASSET_MANIFEST_AUDIO},
    {"materials", ILEN_ASSET_MANIFEST_MATERIALS},
};

const std::vector<std::string> ILEN_ASSET_MANIFEST_ASSETS_TYPES = { ILEN_ASSET_MANIFEST_MODEL, ILEN_ASSET_MANIFEST_TEXTURES, ILEN_ASSET_MANIFEST_AUDIO, ILEN_ASSET_MANIFEST_MATERIALS, ILEN_ASSET_MANIFEST_SHADERS };

using namespace ilen::core;

#if ILEN_DEBUG && ILEN_DESKTOP
using json = nlohmann::json;

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
#endif

std::string generateFriendlyName(const std::string& filename){

    std::string extension = std::filesystem::path(filename).extension().string();
    std::string name = std::filesystem::path(filename).stem().string();

    return name + "-" + extensionToAssetType.find(extension)->second; 
}

void AssetsManager::loadManifiest(EngineContext* engineContext) {
    #if ILEN_DEBUG && ILEN_DESKTOP //En dispositivod móviles o consolas esto no tiene sentido. Realiza el trabajo usando los archivos fuente
    jobSystem = engineContext->p_jobSystem.get();
    indexNewModels(); //Actualizamos el manifiesto con nuevos archivos y generamos sus respectivas representaciones binarias.
    #endif

    m_manifestFileName = "assets.bin.ilenmanifest";

    auto manifestPath = FileManager::fileRoute(m_manifestFileName, ILEN_FILE_TYPE_ASSETS_MANIFEST);

    std::ifstream file(manifestPath, std::ios::binary);

    if(!file.is_open()){
        ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "No se pudo abrir el manifiesto principal de assets del juego");
    }

    ilen::binary::headers::IlenManifestHeader header{};

    file.read(reinterpret_cast<char*>(&header),sizeof(header));

    if(std::string(header.magicNumber) != "TSEFINAM_NELI"){
        ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "Header of Manifest file is not compatible by magic number");
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
    for(int i = 0; i < header.totalOfModels; i++){
        uint32_t ID;
        uint32_t pathNameLenght;

        file.read(reinterpret_cast<char*>(&ID), sizeof(ID));
        file.read(reinterpret_cast<char*>(&pathNameLenght), sizeof(pathNameLenght));

        std::string path(pathNameLenght, '\0');
        file.read(&path[0], pathNameLenght);

        m_idToTextureAsset.emplace(ID, TextureAsset(ID, std::filesystem::path(path)));
    }


    isManifestLoaded = true;
    ILEN_LOG_P(DEBUG_PREFIX, "MANIFIESTO CARGADO EXITOSAMENTE. Total de items cargados: %zu", m_friendlyNameToID.size());
    /*json data;

    if(file.is_open()){
        file >> data;
    } else {
        Debug::ExecuteError("No se pudo abrir el archivo del manifiesto, o no existe en la ruta: " + manifestPath.string(), "ASSETS_MANAGER");
    }

    //Model loading
    if(data.contains(ILEN_ASSET_MANIFEST_MODEL)){
        writeToMap(data[ILEN_ASSET_MANIFEST_MODEL], m_friendlyNameToID, m_idToModelAsset);
    }

    //Texture loading
    if(data.contains(ILEN_ASSET_MANIFEST_TEXTURES)){
        writeToMap(data[ILEN_ASSET_MANIFEST_TEXTURES], m_friendlyNameToID, m_idToTextureAsset);
    }


     
    //Audio loading
    if(data.contains(ILEN_ASSET_MANIFEST_AUDIO)){
        writeToMap(data[ILEN_ASSET_MANIFEST_AUDIO], m_friendlyNameToID, m_idToAudioAsset);
    }

    //Shader loading
    if(data.contains(ILEN_ASSET_MANIFEST_SHADERS)){
        std::unordered_map<uint32_t, ShaderAsset> temporalShaderMap;
        writeToMap(data[ILEN_ASSET_MANIFEST_SHADERS], m_friendlyNameToID, temporalShaderMap);

        for(const auto& shaderMap: temporalShaderMap){
            auto toStoreShader = ilen::graphics::IlenShaderUtils::generateIlenShaderFromAsset(shaderMap.second);
            m_idToShaderAsset.emplace(shaderMap.first, std::make_shared<ilen::graphics::IlenShader>(std::move(toStoreShader))); //Hacer el segundo de ShaderAsset a IlenShader
        }
    }

        //Material loading
    if(data.contains(ILEN_ASSET_MANIFEST_MATERIALS)){
        std::unordered_map<uint32_t, MaterialAsset> temporalMaterialMap;
        writeToMap(data[ILEN_ASSET_MANIFEST_MATERIALS], m_friendlyNameToID, temporalMaterialMap);

        for(const auto& materialMap: temporalMaterialMap){
            auto toStoreMaterial = ilen::graphics::IlenMaterialHelper::generateIlenMaterialFromAsset(materialMap.second);
            m_idToMaterialAsset.emplace(materialMap.first, std::make_shared<ilen::graphics::IlenMaterial>(std::move(toStoreMaterial))); //Hacer el segundo de ShaderAsset a IlenShader
        }    
    }*/
}

#if ILEN_DESKTOP && ILEN_DEBUG //No se necesita este símbolo en un sistema que no sirva para programar
void AssetsManager::indexNewModels(){
    m_manifestFileName = "assets.ilenmanifest";

    std::filesystem::path engineSourceFolder(ILEN_SOURCES_FOLDER);
    std::filesystem::path sourceAssetFolders = engineSourceFolder / "assets";

    auto manifestPath = sourceAssetFolders / "manifests" / "assets.ilenmanifest";

    std::ifstream file(manifestPath);
    json data;

    if(file.is_open()){
        file >> data;
    } else {
        ILEN_LOG_P(DEBUG_PREFIX, "No se pudo abrir el archivo del manifiesto, o no existe en la ruta: %s", manifestPath.string().c_str());
        #ifdef ILEN_DEBUG
        //Creamos una nueva lista
        auto predefinedPath = sourceAssetFolders / "manifests" / "predefined.ilenmanifest";
        std::ifstream predefined(predefinedPath);
        if(predefined.is_open()){
            predefined >> data;
        }else{
            data[ILEN_ASSET_MANIFEST_TOTAL_OF_ELEMENTS] = 0;
            data[ILEN_ASSET_MANIFEST_MODEL] = json::array(); //Inicializamos si estamos en Debug... No tiene sentido que se cree un nuevo indice en una build release
            data[ILEN_ASSET_MANIFEST_TEXTURES] = json::array();
            data[ILEN_ASSET_MANIFEST_MATERIALS] = json::array();
            data[ILEN_ASSET_MANIFEST_AUDIO] = json::array();
            data[ILEN_ASSET_MANIFEST_SHADERS] = json::array();
        }
        #endif
    }

    ILEN_WARNING_P(DEBUG_PREFIX, "Indexando nuevos objetos en modo Debug");

    //Debug::Warning(nlohmann::to_string(manifestData));

    uint32_t maxID = data.value(ILEN_ASSET_MANIFEST_TOTAL_OF_ELEMENTS, 0);
    std::vector<std::string> existingPaths;
    bool folderReorganized = false;

    //Este vector son las extensiones que no deben de registrarse en el assets.json, ya que otras clases gestionaran su uso.
    std::vector<std::string> extensionCantRegistrate = {
        ".metal", ".hlsl", ".glsl", ".spv", ".ilenmanifest", ".mtl"
    };
    std::string unsupportedFolder = "unsupported";

    //Obteniendo todas las rutas declaradas anteriormente en el manifiesto.
    for(const auto& assetType : ILEN_ASSET_MANIFEST_ASSETS_TYPES) {
        if(data.contains(assetType)){
            for(const auto& assetData : data[assetType]){
                existingPaths.push_back(assetData.value(ILEN_ASSET_MANIFEST_ASSET_PATH, ""));
            }
        }
    }

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
        
        auto result = std::find(existingPaths.begin(), existingPaths.end(), relativeToAssets.generic_string());

        if(result != existingPaths.end()) continue; //Si existe el path en el manifiesto, saltar al próximo.
        
        std::string extension = relativeToAssets.extension().string();
        auto it = extensionToAssetType.find(extension);

        //std::find(extensionCantRegistrate.begin(), extensionCantRegistrate.end(), it) == extensionCantRegistrate.end()

        //Guardar si la extensión existe y si esta en la carpeta correcta. Técnicamente no es necesario, pero no esta demás la comprobación, además de que esto solo esta en DEBUG. No añado las extensiones prohibidas al manifiesto.
        if(it != extensionToAssetType.end() && relativeToAssets.begin()->string() == it->second && std::find(extensionCantRegistrate.begin(), extensionCantRegistrate.end(), it->first) == extensionCantRegistrate.end()){
            needSaveNewManifest = true;
            
            uint32_t newID = ++maxID;
            std::string name = generateFriendlyName(currentPath.filename().string());

            json newAsset;
            newAsset[ILEN_ASSET_MANIFEST_ASSET_ID] = newID;
            newAsset[ILEN_ASSET_MANIFEST_ASSET_NAME] = name;
            newAsset[ILEN_ASSET_MANIFEST_ASSET_PATH] = relativeToAssets.generic_string();
            data[folderToManifestAssetGroup.find(it->second)->second].push_back(newAsset);
        }
    }

    //Escribir nuevo manifiesto con los nuevos datos si es necesario
    if(needSaveNewManifest){
        data[ILEN_ASSET_MANIFEST_TOTAL_OF_ELEMENTS] = maxID;
        
        auto manifestPath = sourceAssetFolders / "manifests" / "assets.ilenmanifest";
        if(!std::filesystem::exists(manifestPath.parent_path()))
            std::filesystem::create_directories(manifestPath.parent_path());

        std::ofstream outFile(manifestPath);
        outFile << data.dump(2);
        outFile.close();

    }

    assert(needSaveNewManifest == false && "Se escribió un nuevo manifiesto. Vuelve a iniciar la app para que ahora si funcione de forma correcta.");


    ILEN_WARNING_P(DEBUG_PREFIX, "Indexación de nuevos elementos completada. Generando binarios.");
    generateBinaryRepresentation(data);
}

    void AssetsManager::generateBinaryRepresentation(json& manifestData) {
        ILEN_WARNING_P(DEBUG_PREFIX, "Start of binarization: %.4f", ilen::time::Time::getTotalTime());
        //Los archivos binarios que usará el motor van a estar en una carpeta llamada /.ilen-gen-assets
        
        //1- Leer datos del manifiesto para saber que archivos hay que hacer binario.
        uint32_t maxID = manifestData.value(ILEN_ASSET_MANIFEST_TOTAL_OF_ELEMENTS, 0);
        std::set<std::string> existingPaths;

        //Obteniendo todas las rutas declaradas anteriormente en el manifiesto.
        for(const auto& assetType : ILEN_ASSET_MANIFEST_ASSETS_TYPES) {
            if(manifestData.contains(assetType)){
                for(const auto& assetData : manifestData[assetType]){
                    existingPaths.insert(assetData.value(ILEN_ASSET_MANIFEST_ASSET_PATH, ""));
                }
            }
        }
        std::filesystem::path engineSourceFolder(ILEN_SOURCES_FOLDER); //Carpeta del engine con los assets en crudo.
        std::filesystem::path sourceAssetFolders = engineSourceFolder / "assets";
        std::filesystem::path binaryDataFolder = engineSourceFolder / ".ilen-bin-data";
        //Creando el directotio de los binarios si no existe
        if(!std::filesystem::exists(binaryDataFolder)){
            std::filesystem::create_directories(binaryDataFolder);
        }

        std::unordered_map<std::string, uint32_t> friendlyNameToID;
        std::unordered_map<uint32_t, ModelAsset> idToModelAsset;
        std::unordered_map<uint32_t, TextureAsset> idToTextureAsset;
        std::unordered_map<uint32_t, AudioAsset> idToAudioAsset;
        std::unordered_map<uint32_t, std::shared_ptr<ilen::graphics::IlenShader>> idToShaderAsset;
        std::unordered_map<uint32_t, std::shared_ptr<ilen::graphics::IlenMaterial>> idToMaterialAsset;

        //2 - Procesar cada archivo y guardarlo.
        //Model loading
        if(manifestData.contains(ILEN_ASSET_MANIFEST_MODEL)){
            std::string targetFolder = "models";
            std::filesystem::path binaryModelFolder = binaryDataFolder / targetFolder;
            if(!std::filesystem::exists(binaryModelFolder)){
                std::filesystem::create_directories(binaryModelFolder);
            }

            std::vector<std::future<void>> futures;

            for(const json& assetData : manifestData[ILEN_ASSET_MANIFEST_MODEL]){
                std::string filePath = assetData[ILEN_ASSET_MANIFEST_ASSET_PATH];
                futures.push_back(
                    jobSystem->SubmitJob([=, this, &sourceAssetFolders, &engineSourceFolder, &binaryModelFolder, &idToModelAsset, &friendlyNameToID, &assetData]() {
                        auto modelPath = sourceAssetFolders / filePath;
                        ilen::ecs::components::MeshData toBinarize = loadModelData(modelPath);
                        auto savePath = binaryModelFolder / std::filesystem::relative(engineSourceFolder / filePath, engineSourceFolder / targetFolder);
                        savePath.replace_extension(".ilenmesh");
                        //Header del binario
                        ilen::binary::headers::IlenMeshHeader header{};
                        strncpy(header.magicNumber, "HSEM_NELI", sizeof(header.magicNumber) - 1);
                        header.version = 1;
                        header.vertexCount = toBinarize.vertexBufferData.size();
                        header.indexCount = toBinarize.indexBufferData.size();

                        if(!std::filesystem::exists(savePath.parent_path())){
                            std::filesystem::create_directories(savePath.parent_path());
                        }

                        std::ofstream outFile(savePath, std::ios::binary);
                        outFile.write(reinterpret_cast<const char*>(&header), sizeof(ilen::binary::headers::IlenMeshHeader));
                        outFile.write(reinterpret_cast<const char*>(toBinarize.vertexBufferData.data()), sizeof(ilen::render::buffers::VertexBufferData) * toBinarize.vertexBufferData.size());
                        outFile.write(reinterpret_cast<const char*>(toBinarize.indexBufferData.data()), sizeof(uint32_t) * toBinarize.indexBufferData.size());

                        outFile.close();

                        std::lock_guard<std::mutex> lock(fileWrite);

                        uint32_t id = assetData[ILEN_ASSET_MANIFEST_ASSET_ID];
                        std::string name = assetData[ILEN_ASSET_MANIFEST_ASSET_NAME];
                        std::string path = std::filesystem::relative(savePath, binaryModelFolder);
                        friendlyNameToID[name] = id;
                        idToModelAsset.emplace(id, ModelAsset(id, path));
                    })
                );
            }

            for (auto& f: futures){
                f.get();
            }
        }

        //Texture loading
        /*if(data.contains(ILEN_ASSET_MANIFEST_TEXTURES)){
            writeToMap(data[ILEN_ASSET_MANIFEST_TEXTURES], m_friendlyNameToID, m_idToTextureAsset);
        }*/


        
        //Audio loading
        /*if(data.contains(ILEN_ASSET_MANIFEST_AUDIO)){
            writeToMap(data[ILEN_ASSET_MANIFEST_AUDIO], m_friendlyNameToID, m_idToAudioAsset);
        }*/

        //Shader loading
        /*if(data.contains(ILEN_ASSET_MANIFEST_SHADERS)){
            std::unordered_map<uint32_t, ShaderAsset> temporalShaderMap;
            writeToMap(data[ILEN_ASSET_MANIFEST_SHADERS], m_friendlyNameToID, temporalShaderMap);

            for(const auto& shaderMap: temporalShaderMap){
                auto toStoreShader = ilen::graphics::IlenShaderUtils::generateIlenShaderFromAsset(shaderMap.second);
                m_idToShaderAsset.emplace(shaderMap.first, std::make_shared<ilen::graphics::IlenShader>(std::move(toStoreShader))); //Hacer el segundo de ShaderAsset a IlenShader
            }
        }*/

            //Material loading
        /*if(data.contains(ILEN_ASSET_MANIFEST_MATERIALS)){
            std::unordered_map<uint32_t, MaterialAsset> temporalMaterialMap;
            writeToMap(data[ILEN_ASSET_MANIFEST_MATERIALS], m_friendlyNameToID, temporalMaterialMap);

            for(const auto& materialMap: temporalMaterialMap){
                auto toStoreMaterial = ilen::graphics::IlenMaterialHelper::generateIlenMaterialFromAsset(materialMap.second);
                m_idToMaterialAsset.emplace(materialMap.first, std::make_shared<ilen::graphics::IlenMaterial>(std::move(toStoreMaterial))); //Hacer el segundo de ShaderAsset a IlenShader
            }    
        }*/
        
        //3 - Generar un manifiesto para los binarios.
        //No necesito un manifiesto en JSON, mejor volcar los mapas de memoria en un archivo, así no tenemos que volver a cargar y configurar estos, sino que sea solo una lectura.
        
        //4 - Guardar ese manifiesto
        std::filesystem::path manifiestPath = binaryDataFolder / "manifests" / "assets.bin.ilenmanifest";
        if(!std::filesystem::exists(manifiestPath.parent_path())){
            std::filesystem::create_directories(manifiestPath.parent_path());
        }

        ilen::binary::headers::IlenManifestHeader manifestHeader{};
        strncpy(manifestHeader.magicNumber, "TSEFINAM_NELI", sizeof(manifestHeader.magicNumber) - 1);
        manifestHeader.version = 1;
        manifestHeader.totalOfNamesToID = friendlyNameToID.size();
        manifestHeader.totalOfModels = idToModelAsset.size();
        manifestHeader.totalOfTextures = idToTextureAsset.size();

        std::ofstream binaryFile(manifiestPath, std::ios::binary);
        binaryFile.write(reinterpret_cast<const char*>(&manifestHeader), sizeof(ilen::binary::headers::IlenManifestHeader));
        //Escribimos el Array de friend Name to ID
        for(auto& pair: friendlyNameToID){
            //Orden -> ID, SizeOfName, Name
            uint32_t ID = pair.second;
            std::string friendName = pair.first;
            uint32_t friendNameSize = friendName.length();

            binaryFile.write(reinterpret_cast<const char*>(&ID), sizeof(ID));
            binaryFile.write(reinterpret_cast<const char*>(&friendNameSize), sizeof(friendNameSize));
            binaryFile.write(friendName.c_str(), friendNameSize);
        }
        
        //Array ID to Model
        for(auto& pair: idToModelAsset){
            //Orden -> ID, SizeOfPath, Path
            uint32_t ID = pair.first;
            std::string path = pair.second.assetPath.string();
            uint32_t pathSize = path.length();

            binaryFile.write(reinterpret_cast<const char*>(&ID), sizeof(ID));
            binaryFile.write(reinterpret_cast<const char*>(&pathSize), sizeof(pathSize));
            binaryFile.write(path.c_str(),pathSize);
        }
        //Array ID to Texture
        for(auto& pair: idToTextureAsset){
            //Orden -> ID, SizeOfPath, Path
            uint32_t ID = pair.first;
            std::string path = pair.second.assetPath.string();
            uint32_t pathSize = path.length();

            binaryFile.write(reinterpret_cast<const char*>(&ID), sizeof(ID));
            binaryFile.write(reinterpret_cast<const char*>(&pathSize), sizeof(pathSize));
            binaryFile.write(path.c_str(),pathSize);
        }


        binaryFile.close();
        ILEN_WARNING_P(DEBUG_PREFIX, "End of binarization: %.4f", ilen::time::Time::getTotalTime());
    }

    ilen::ecs::components::MeshData AssetsManager::loadModelData(std::filesystem::path modelPath){
        //std::filesystem::path meshFilePath = core::FileManager::fileRoute(meshPath, ILEN_FILE_TYPE_NONE);
        Assimp::Importer importer;
        
        const aiScene* scene = importer.ReadFile(modelPath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
        if(!scene || !scene->HasMeshes()){
            ILEN_WARNING_P(DEBUG_PREFIX, "El objeto en la ruta %s no contenía ningún dato del tipo Mesh o ocurrió un error. Favor de revisar la información del archivo. Error: %s", modelPath.string().c_str(), importer.GetErrorString());
            return ilen::ecs::components::MeshData{};
        }

        //De momento soporte a un mesh por archivo. A futuro se debe de cambiar.
        aiMesh* mesh = scene->mMeshes[0];

        std::vector<ilen::render::buffers::VertexBufferData> vertexData;
        std::vector<uint32_t> indexData;
        
        //Obteniendo los vertices
        for(uint32_t i = 0; i < mesh->mNumVertices; i++){
            ilen::render::buffers::VertexBufferData vertex{};
            vertex.vertexPosition.x = mesh->mVertices[i].x;
            vertex.vertexPosition.y = mesh->mVertices[i].y;
            vertex.vertexPosition.z = mesh->mVertices[i].z;
            vertex.vertexPosition.w = 1.0;

            if(mesh->HasTextureCoords(0)){
                vertex.UV.x = mesh->mTextureCoords[0][i].x;
                vertex.UV.y = mesh->mTextureCoords[0][i].y;
            }else{
                vertex.UV.x = vertex.UV.y = 0.0; //UV texture por defecto
            }

            if(mesh->HasVertexColors(0)){
                vertex.vertexColor.r = mesh->mColors[0][i].r;
                vertex.vertexColor.g = mesh->mColors[0][i].g;
                vertex.vertexColor.b = mesh->mColors[0][i].b;
                vertex.vertexColor.a = mesh->mColors[0][i].a;
            }else{
                vertex.vertexColor = ilen::graphics::color::White;
            }

            vertexData.push_back(vertex);
        }

        //Obteniendo los indices para dibujar los datos.
        for(uint32_t i = 0; i < mesh->mNumFaces; i++){
            aiFace face = mesh->mFaces[i];
            for(uint32_t j = 0; j < face.mNumIndices; j++){
                indexData.push_back(face.mIndices[j]);
            }
        }

        ilen::ecs::components::MeshData meshData{};
        meshData.indexBufferData = indexData;
        meshData.vertexBufferData = vertexData;

        return meshData;
    }
#endif

uint32_t AssetsManager::getAssetID(const std::string& friendlyName) const {
    auto element = m_friendlyNameToID.find(friendlyName);
    if(element == m_friendlyNameToID.end()){
        ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "El asset con nombre %s no existe en los registros. Revisar el nombre.", friendlyName.c_str());
    }
    return element->second; //Regresa el id :)
}
