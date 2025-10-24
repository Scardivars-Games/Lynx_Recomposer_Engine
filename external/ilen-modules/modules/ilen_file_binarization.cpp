#include <iostream>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <external/json/json.hpp>
#include <ilen/headers/ilen-logs.h>
#include <ilen/resources/mesh-manager.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <ilen/job-system/job-system.h>
#include <future>
#include <mutex>


const char* DEBUG_PREFIX = "FILE_BINARIZATION";

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


using json = nlohmann::json;

void createAndOrganizeFiles(std::filesystem::path ilenEngineFolder);
void binarizeFiles(std::filesystem::path ilenEngineFolder);
std::string generateFriendlyName(const std::string& filename);
ilen::resources::mesh::MeshData loadModelData(std::filesystem::path modelPath);

ilen::job_system::JobSystem jobSystem;
std::mutex fileWrite;


namespace ilen::maths::vector {
    // Le enseña a nlohmann/json cómo convertir un array JSON a un ilen::maths::vector
    template<typename T, size_t N>
    void from_json(const nlohmann::json& j, Vector<T, N>& v) {
        if (j.is_array() && j.size() == N) {
            for (size_t i = 0; i < N; ++i) {
                j.at(i).get_to(v.data[i]);
            }
        }
    }
}

template<typename T>
void processScalarProperties(const nlohmann::json& jsonData, const std::string& key, std::unordered_map<std::string, ilen::graphics::MaterialProperty>& properties);

template<typename VecType>
void processVectorProperties(const nlohmann::json& jsonData, const std::string& key, std::unordered_map<std::string, ilen::graphics::MaterialProperty>& properties);


int main(int argc, char* argv[]){
    std::filesystem::path folderPath;

    jobSystem.Initializate();
    ilen::time::Time::Init();

    for(int i = 0; i < argc; i++){
        std::string arg = argv[i];
        if(arg == "--ilen-folder" || arg == "-f"){
            if(i + 1 < argc){
                folderPath = argv[i + 1];
            } else {
                std::cerr << "Error: El indicador --ilen-folder requiere una ruta a una carpeta." << std::endl;
                return 1;
            }
        }
    }

    //Pasos para poder binarizar
    createAndOrganizeFiles(folderPath);
    binarizeFiles(folderPath);

    jobSystem.StopJobSystem();

    return 0;
}

void createAndOrganizeFiles(std::filesystem::path ilenEngineFolder)
{
    std::string m_manifestFileName = "assets.ilenmanifest";

    std::filesystem::path engineSourceFolder = ilenEngineFolder;
    std::filesystem::path sourceAssetFolders = engineSourceFolder / "assets";

    //Erase empty folder in assets (cause this is called recursively after each step, will clear the empty folder after asset reorganization)
    try {
        // 1. Iterate over direct children only (not recursive)
        for (const auto& entry : std::filesystem::directory_iterator(sourceAssetFolders)) {

            // 2. Check if the entry is a directory
            if (entry.is_directory()) {

                // 3. Check if the directory is empty
                if (std::filesystem::is_empty(entry.path())) {
                    std::cout << "Removing empty directory: " << entry.path().string() << std::endl;
                    // 4. Remove it
                    std::filesystem::remove(entry.path());
                }
            }
        }
    } catch (std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    }

    auto manifestPath = sourceAssetFolders / "manifests" / m_manifestFileName;

    std::ifstream file(manifestPath);
    json data;

    if(file.is_open()){
        file >> data;
    } else {
        ILEN_LOG_P(DEBUG_PREFIX, "No se pudo abrir el archivo del manifiesto, o no existe en la ruta: %s", manifestPath.string().c_str());
        //Creamos una nueva lista
        auto predefinedPath = sourceAssetFolders / "manifests" / "predefined.ilenmanifest";
        std::ifstream predefined(predefinedPath);
        if(predefined.is_open())
        {
            try
            {
                data = json::parse(predefined);
            }catch (json::parse_error& e)
            {
                ILEN_WARNING_P(DEBUG_PREFIX, "Error at parsion predefined assets: %s", e.what());
                data = nullptr;
            }
        }else{

            data[ILEN_ASSET_MANIFEST_TOTAL_OF_ELEMENTS] = 0;
            data[ILEN_ASSET_MANIFEST_MODEL] = json::array(); //Inicializamos si estamos en Debug... No tiene sentido que se cree un nuevo indice en una build release
            data[ILEN_ASSET_MANIFEST_TEXTURES] = json::array();
            data[ILEN_ASSET_MANIFEST_MATERIALS] = json::array();
            data[ILEN_ASSET_MANIFEST_AUDIO] = json::array();
            data[ILEN_ASSET_MANIFEST_SHADERS] = json::array();
        }
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

    if (folderReorganized == true)
    {
        createAndOrganizeFiles(ilenEngineFolder);
        return;
    }

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

        //auto manifestPath = sourceAssetFolders / "manifests" / "assets.ilenmanifest";
        if(!std::filesystem::exists(manifestPath.parent_path()))
            std::filesystem::create_directories(manifestPath.parent_path());

        std::ofstream outFile(manifestPath);
        outFile << data.dump(2);
        outFile.close();

    }

    //assert(needSaveNewManifest == false && "Se escribió un nuevo manifiesto. Vuelve a iniciar la app para que ahora si funcione de forma correcta.");
    if (needSaveNewManifest == true)
    {
        createAndOrganizeFiles(ilenEngineFolder);
        return;
    }

    ILEN_WARNING_P(DEBUG_PREFIX, "Indexación de nuevos elementos completada. Generando binarios.");
}

std::string generateFriendlyName(const std::string& filename){

    std::string extension = std::filesystem::path(filename).extension().string();
    std::string name = std::filesystem::path(filename).stem().string();

    return name + "-" + extensionToAssetType.find(extension)->second;
}



void binarizeFiles(std::filesystem::path ilenEngineFolder) {
    using namespace ilen::resources;
    std::string m_manifestFileName = "assets.ilenmanifest";

    std::filesystem::path engineSourceFolder = ilenEngineFolder;
    std::filesystem::path sourceAssetFolders = engineSourceFolder / "assets";

    //Erase empty folder in assets (cause this is called recursively after each step, will clear the empty folder after asset reorganization)
    try {
        // 1. Iterate over direct children only (not recursive)
        for (const auto& entry : std::filesystem::directory_iterator(sourceAssetFolders)) {

            // 2. Check if the entry is a directory
            if (entry.is_directory()) {

                // 3. Check if the directory is empty
                if (std::filesystem::is_empty(entry.path())) {
                    std::cout << "Removing empty directory: " << entry.path().string() << std::endl;
                    // 4. Remove it
                    std::filesystem::remove(entry.path());
                }
            }
        }
    } catch (std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    }

    auto manifestPath = sourceAssetFolders / "manifests" / m_manifestFileName;

    std::ifstream file(manifestPath);
    json manifestData;

    if(file.is_open()){
        file >> manifestData;
    } else {
        ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "No se pudo abrir el archivo del manifiesto, o no existe en la ruta: %s", manifestPath.string().c_str());
    }

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

    //std::filesystem::path engineSourceFolder(ILEN_SOURCES_FOLDER); //Carpeta del engine con los assets en crudo.
    //std::filesystem::path sourceAssetFolders = engineSourceFolder / "assets";
    std::filesystem::path binaryDataFolder = engineSourceFolder / ".ilen-bin-data";
    //Creando el directotio de los binarios si no existe
    if(!std::filesystem::exists(binaryDataFolder)){
        std::filesystem::create_directories(binaryDataFolder);
    }

    std::unordered_map<std::string, uint32_t> friendlyNameToID;
    std::unordered_map<uint32_t, ModelAsset> idToModelAsset;
    std::unordered_map<uint32_t, TextureAsset> idToTextureAsset;
    std::unordered_map<uint32_t, AudioAsset> idToAudioAsset;
    std::unordered_map<uint32_t, ShaderAsset> idToShaderAsset;
    std::unordered_map<uint32_t, MaterialAsset> idToMaterialAsset;

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
                jobSystem.SubmitJob([=, &sourceAssetFolders, &engineSourceFolder, &binaryModelFolder, &idToModelAsset, &friendlyNameToID, &assetData]() {
                    auto modelPath = sourceAssetFolders / filePath;
                    ilen::resources::mesh::MeshData toBinarize = loadModelData(modelPath);
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

    //Shader loading
    if(manifestData.contains(ILEN_ASSET_MANIFEST_SHADERS)){
        std::string targetFolder = "shaders";
        std::filesystem::path shaderBinaryFolder = binaryDataFolder / targetFolder;
        if (!std::filesystem::exists(shaderBinaryFolder))
        {
            std::filesystem::create_directories(shaderBinaryFolder);
        }

        for (const auto& shaderData: manifestData[ILEN_ASSET_MANIFEST_SHADERS])
        {

            const std::string ILEN_SHADER_FILE_NAME = "shader-file-name";
            const std::string ILEN_SHADER_DIRECTORY = "shader-dir";
            const std::string ILEN_SHADER_HUMAN_REDEABLE_NAME = "shader-name";

            std::string originalFilePath = shaderData[ILEN_ASSET_MANIFEST_ASSET_PATH];

            auto sourcePath = sourceAssetFolders / originalFilePath;
            auto savePath = binaryDataFolder / std::filesystem::relative(sourcePath, engineSourceFolder / "assets");
            //zsavePath.replace_extension(".b_ilenshader");

            auto path = std::filesystem::relative(savePath, binaryDataFolder);

            uint32_t id = shaderData[ILEN_ASSET_MANIFEST_ASSET_ID];
            std::string name = shaderData[ILEN_ASSET_MANIFEST_ASSET_NAME];

            std::ifstream shaderFile(sourcePath);
            json shader;

            if(shaderFile.is_open()){
                try
                {
                    shader = json::parse(shaderFile);
                }catch (json::parse_error& e)
                {
                    ILEN_ERROR_P(DEBUG_PREFIX, "Problemas al leer shader en ruta %s. Error: %s", sourcePath.c_str(), e.what());
                }
            }else{
                ILEN_ERROR_P(DEBUG_PREFIX, "Tratando de leer shader en la ruta %s y no se encontró, o logro abrir", sourcePath.c_str());
                //return ErrorShader; //Enviando shader predefinido de error
            }

            ilen::graphics::IlenShader generatedShader(shader[ILEN_SHADER_FILE_NAME], shader[ILEN_SHADER_DIRECTORY], shader[ILEN_SHADER_HUMAN_REDEABLE_NAME]);

            //auto generatedShader = ilen::graphics::IlenShaderUtils::generateIlenShaderFromAsset(ShaderAsset(id, path));

            //Header del archivo binario
            ilen::binary::headers::IlenShaderHeader header{};
            header.version = 1;
            strncpy(header.magicNumber, "REDAHS_NELI", sizeof(header.magicNumber) - 1);
            header.fileNameSize = generatedShader.shaderFileName.size();
            header.fileRouteSize = generatedShader.shaderRoute.string().size(); //Esta ruta es solo carpeta,  el nombre del archivo final se compone usando el filename y la extensión correspondiente a la API gráfica.
            header.shaderNameSize = generatedShader.shaderName.size();

            if(!std::filesystem::exists(savePath.parent_path()))
            {
                std::filesystem::create_directories(savePath.parent_path());
            }

            savePath.replace_extension(".b_ilenshader");

            std::ofstream outFile(savePath,std::ios::binary);
            outFile.write(reinterpret_cast<const char*>(&header),sizeof(ilen::binary::headers::IlenShaderHeader));
            outFile.write(generatedShader.shaderFileName.c_str(), header.fileNameSize);
            outFile.write(generatedShader.shaderRoute.c_str(), header.fileRouteSize);
            outFile.write(generatedShader.shaderName.c_str(), header.shaderNameSize);
            //outFile.write(reinterpret_cast<const char*>(&generatedShader), sizeof(ilen::graphics::IlenShader));

            friendlyNameToID[name] = id;
            idToShaderAsset.emplace(id, ShaderAsset(id, std::filesystem::relative(savePath,shaderBinaryFolder)));
        }
    }

    //Material loading
    if(manifestData.contains(ILEN_ASSET_MANIFEST_SHADERS)){
        std::string targetFolder = "materials";
        std::filesystem::path materialBinaryFolder = binaryDataFolder / targetFolder;
        if (!std::filesystem::exists(materialBinaryFolder))
        {
            std::filesystem::create_directories(materialBinaryFolder);
        }

        for (const auto& materialData: manifestData[ILEN_ASSET_MANIFEST_MATERIALS])
        {
            const std::string ILEN_MATERIAL_NAME = "material-name";
            const std::string ILEN_MATERIAL_VERTEX_SHADER_ID = "vertex-shader-id";
            const std::string ILEN_MATERIAL_FRAGMENT_SHADER_ID = "fragment-shader-id";
            const std::string ILEN_MATERIAL_VERTEX_SHADER_FUNCTION = "fragment-shader-function";
            const std::string ILEN_MATERIAL_FRAGMENT_SHADER_FUNCTION = "vertex-shader-function";

            //Keys for properties, to be added in the future
            const std::string ILEN_MATERIAL_FLOAT_PROPERTY = "float-property";
            const std::string ILEN_MATERIAL_INTEGER_PROPERTY = "int-property";
            const std::string ILEN_MATERIAL_FLOAT2_PROPERTY = "float2-property";
            const std::string ILEN_MATERIAL_FLOAT3_PROPERTY = "float3-property";
            const std::string ILEN_MATERIAL_FLOAT4_PROPERTY = "float4-property";
            const std::string ILEN_MATERIAL_TEXTURE_PROPERTY = "texture-property"; // Para texturas

            std::string originalFilePath = materialData[ILEN_ASSET_MANIFEST_ASSET_PATH];

            auto sourcePath = sourceAssetFolders / originalFilePath;
            auto savePath = binaryDataFolder / std::filesystem::relative(sourcePath, engineSourceFolder / "assets");
            //zsavePath.replace_extension(".b_ilenshader");

            auto path = std::filesystem::relative(savePath, binaryDataFolder);

            uint32_t id = materialData[ILEN_ASSET_MANIFEST_ASSET_ID];
            std::string name = materialData[ILEN_ASSET_MANIFEST_ASSET_NAME];

            std::ifstream materialFile(sourcePath);
            json materialJson;

            if(materialFile.is_open()){
                try
                {
                    materialJson = json::parse(materialFile);
                }catch (json::parse_error& e)
                {
                    ILEN_ERROR_P(DEBUG_PREFIX, "Problemas al leer shader en ruta %s. Error: %s", sourcePath.c_str(), e.what());
                }
            }else
            {
                ILEN_ERROR_P(DEBUG_PREFIX, "Tratando de leer shader en la ruta %s y no se encontró, o logro abrir", sourcePath.c_str());
            }


            ilen::graphics::IlenMaterial material;
            material.materialName = materialJson.value(ILEN_MATERIAL_NAME, "Unnamed Material");
            material.vertexShaderID = materialJson.value(ILEN_MATERIAL_VERTEX_SHADER_ID, 0);
            material.fragmentShaderID = materialJson.value(ILEN_MATERIAL_FRAGMENT_SHADER_ID, 0);

            // Corregí el typo en las constantes para que coincidan con el archivo de cabecera
            material.vertexFunctionName = materialJson.value(ILEN_MATERIAL_VERTEX_SHADER_FUNCTION, "");
            material.fragmentFunctionName = materialJson.value(ILEN_MATERIAL_FRAGMENT_SHADER_FUNCTION, "");

            // Procesar todas las propiedades con la nueva lógica
            processScalarProperties<float>(materialJson, ILEN_MATERIAL_FLOAT_PROPERTY, material.properties);
            processScalarProperties<uint32_t>(materialJson, ILEN_MATERIAL_INTEGER_PROPERTY, material.properties);
            processScalarProperties<uint32_t>(materialJson, ILEN_MATERIAL_TEXTURE_PROPERTY, material.properties); // Las texturas se guardan como ID (uint32_t)

            processVectorProperties<ilen::maths::vector::vec2>(materialJson, ILEN_MATERIAL_FLOAT2_PROPERTY, material.properties);
            processVectorProperties<ilen::maths::vector::vec3>(materialJson, ILEN_MATERIAL_FLOAT3_PROPERTY, material.properties);
            processVectorProperties<ilen::maths::vector::vec4>(materialJson, ILEN_MATERIAL_FLOAT4_PROPERTY, material.properties);

            //Save and binarize material file
            ilen::binary::headers::IlenMaterialHeader header{};
            header.version = 1;
            strncpy(header.magicNumber, "LAIRETAM_NELI", sizeof(header.magicNumber) - 1);
            header.materialNameSize = material.materialName.size();
            header.vertexFunctionSize = material.vertexFunctionName.size();
            header.fragmentFunctionSize = material.fragmentFunctionName.size();

            if(!std::filesystem::exists(savePath.parent_path()))
            {
                std::filesystem::create_directories(savePath.parent_path());
            }

            savePath.replace_extension(".b_ilenmat");


            //Orden de escritura!!!
            //Header -> Ints del material -> strings del material -> Properties (TODO)
            std::ofstream outFile(savePath,std::ios::binary);
            outFile.write(reinterpret_cast<const char*>(&header),sizeof(ilen::binary::headers::IlenMaterialHeader));
            outFile.write(reinterpret_cast<const char*>(&material.vertexShaderID), sizeof(uint32_t));
            outFile.write(reinterpret_cast<const char*>(&material.fragmentShaderID), sizeof(uint32_t));
            outFile.write(material.materialName.c_str(), header.materialNameSize);
            outFile.write(material.vertexFunctionName.c_str(), header.vertexFunctionSize);
            outFile.write(material.fragmentFunctionName.c_str(), header.fragmentFunctionSize);

            friendlyNameToID[name] = id;
            idToMaterialAsset.emplace(id, MaterialAsset(id, std::filesystem::relative(savePath, materialBinaryFolder)));
        }
    }

    //3 - Generar un manifiesto para los binarios.
    //No necesito un manifiesto en JSON, mejor volcar los mapas de memoria en un archivo, así no tenemos que volver a cargar y configurar estos, sino que sea solo una lectura.

    //4 - Guardar ese manifiesto
    std::filesystem::path manifiestPath = binaryDataFolder / "manifests" / "assets.b_ilenmanifest";
    if(!std::filesystem::exists(manifiestPath.parent_path())){
        std::filesystem::create_directories(manifiestPath.parent_path());
    }

    ilen::binary::headers::IlenManifestHeader manifestHeader{};
    strncpy(manifestHeader.magicNumber, "TSEFINAM_NELI", sizeof(manifestHeader.magicNumber) - 1);
    manifestHeader.version = 1;
    manifestHeader.totalOfNamesToID = friendlyNameToID.size();
    manifestHeader.totalOfModels = idToModelAsset.size();
    manifestHeader.totalOfTextures = idToTextureAsset.size();
    manifestHeader.totalOfShaders = idToShaderAsset.size();
    manifestHeader.totalOfMaterials = idToMaterialAsset.size();

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
    //Array ID to Shader
    for (auto& pair: idToShaderAsset)
    {
        //Orden -> ID, SizeOfPath, Path
        uint32_t ID = pair.first;
        ShaderAsset shader = pair.second;
        std::string path = shader.assetPath.string();
        uint32_t pathSize = path.length();

        binaryFile.write(reinterpret_cast<const char*>(&ID), sizeof(ID));
        binaryFile.write(reinterpret_cast<const char*>(&pathSize), sizeof(pathSize));
        binaryFile.write(path.c_str(),pathSize);
    }
    //Array ID to Material
    for (auto& pair: idToMaterialAsset)
    {
        //Orden -> ID, SizeOfPath, Path
        uint32_t ID = pair.first;
        MaterialAsset material = pair.second;
        //auto shaderPath = shader.shaderRoute / shader.shaderName / ".b_ilenshader";
        std::string path = material.assetPath.string();
        uint32_t pathSize = path.length();

        binaryFile.write(reinterpret_cast<const char*>(&ID), sizeof(ID));
        binaryFile.write(reinterpret_cast<const char*>(&pathSize), sizeof(pathSize));
        binaryFile.write(path.c_str(),pathSize);
    }

    binaryFile.close();
    ILEN_WARNING_P(DEBUG_PREFIX, "End of binarization: %.4f", ilen::time::Time::getTotalTime());
}

ilen::resources::mesh::MeshData loadModelData(std::filesystem::path modelPath){
        //std::filesystem::path meshFilePath = core::FileManager::fileRoute(meshPath, ILEN_FILE_TYPE_NONE);
        Assimp::Importer importer;

        const aiScene* scene = importer.ReadFile(modelPath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
        if(!scene || !scene->HasMeshes()){
            ILEN_WARNING_P(DEBUG_PREFIX, "El objeto en la ruta %s no contenía ningún dato del tipo Mesh o ocurrió un error. Favor de revisar la información del archivo. Error: %s", modelPath.string().c_str(), importer.GetErrorString());
            return ilen::resources::mesh::MeshData{};
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

        ilen::resources::mesh::MeshData meshData{};
        meshData.indexBufferData = indexData;
        meshData.vertexBufferData = vertexData;

        return meshData;
    }


// Función auxiliar para procesar un objeto de propiedades escalares (float, int).
template<typename T>
void processScalarProperties(const nlohmann::json& jsonData, const std::string& key, std::unordered_map<std::string, ilen::graphics::MaterialProperty>& properties) {
    if (jsonData.contains(key) && jsonData[key].is_object()) {
        // Iteramos sobre cada par clave-valor del objeto JSON (ej. "u_Roughness": 0.5)
        for (auto const& [propName, propValueJson] : jsonData[key].items()) {
            if (propValueJson.is_number()) {
                properties[propName] = propValueJson.get<T>();
            }
        }
    }
}

// Función auxiliar para procesar un objeto de propiedades vectoriales (vec2, vec3, Color).
template<typename VecType>
void processVectorProperties(const nlohmann::json& jsonData, const std::string& key, std::unordered_map<std::string, ilen::graphics::MaterialProperty>& properties) {
    if (jsonData.contains(key) && jsonData[key].is_object()) {
        // Iteramos sobre cada par clave-valor del objeto JSON (ej. "_color": [1, 0, 0])
        for (auto const& [propName, propValueJson] : jsonData[key].items()) {
            if (propValueJson.is_array()) {
                VecType vecValue;
                // nlohmann::json puede deserializar un array JSON directamente
                // a una estructura como vec3 o Color si los miembros coinciden.
                propValueJson.get_to(vecValue);
                properties[propName] = vecValue;
            }
        }
    }
}