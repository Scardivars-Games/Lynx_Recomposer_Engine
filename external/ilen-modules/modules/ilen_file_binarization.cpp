#include <iostream>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <json/json.hpp>
#define STB_IMAGE_IMPLEMENTATION //Esto es para el STB
#include <images/stb_image.h>
#include <ilen/headers/ilen-logs.h>
#include <ilen/headers/enumerations.h>
#include <ilen/graphics/meshes.h>
#include <ilen/resources/binary_headers.h>
#include <ilen/resources/assets-elements.h>
#include <ilen/graphics/material.h>
#include <ilen/graphics/shader.h>
#include <ilen/graphics/color.h>
#include <ilen/core/time.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <ilen/job-system/job-system.h>
#include <set>
#include <future>
#include <mutex>
#include <astcenc.h>
#include <bc7enc.h>

namespace ilen::core::threading {
    std::thread::id g_MainThreadId;
    thread_local bool t_IsMainThread = false;
    thread_local size_t t_ThreadIndex = UNREGISTERED_THREAD;
}

using namespace ilen::enumerations;
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

//CONSTANTES PARA SHADERS
const std::string ILEN_SHADER_FILE_NAME = "shader-file-name";
const std::string ILEN_SHADER_DIRECTORY = "shader-dir";
const std::string ILEN_SHADER_HUMAN_REDEABLE_NAME = "shader-name";

//CONSTANTES PARA MATERIALES
const std::string ILEN_MATERIAL_NAME = "material-name";
const std::string ILEN_MATERIAL_VERTEX_SHADER_ID = "vertex-shader-id";
const std::string ILEN_MATERIAL_FRAGMENT_SHADER_ID = "fragment-shader-id";
const std::string ILEN_MATERIAL_VERTEX_SHADER_FUNCTION = "vertex-shader-function";
const std::string ILEN_MATERIAL_FRAGMENT_SHADER_FUNCTION = "fragment-shader-function";
const std::string ILEN_MATERIAL_RENDER_QUEUE = "render-queue";

//Keys for properties, to be added in the future
const std::string ILEN_MATERIAL_PROPERTIES_GROUP = "properties";
const std::string ILEN_MATERIAL_FLOAT_PROPERTY = "float-property";
const std::string ILEN_MATERIAL_INTEGER_PROPERTY = "int-property";
const std::string ILEN_MATERIAL_FLOAT2_PROPERTY = "float2-property";
const std::string ILEN_MATERIAL_FLOAT3_PROPERTY = "float3-property";
const std::string ILEN_MATERIAL_FLOAT4_PROPERTY = "float4-property";
const std::string ILEN_MATERIAL_COLOR_PROPERTY = "color-property"; //Necesario, porque color puede admintir propiedades HDR.
const std::string ILEN_MATERIAL_TEXTURE_PROPERTY = "texture-property"; // Para texturas

const std::string ILEN_MATERIAL_PROPERTY_NAME = "name";
const std::string ILEN_MATERIAL_PROPERTY_VALUE = "value";

//Necesito una base de datos de extensiones soportadas y que tipo de asset es
std::unordered_map<std::string, std::string> extensionToAssetType = {
    //Models
    {".obj", "assets/models"},
    {".glb", "assets/models"},
    {".mtl", "assets/models"}, //Materials de un .obj
    //Textures
    {".png", "assets/textures"},
    {".jpg", "assets/textures"},
    {".jpeg", "assets/textures"},
    {".meta_ilentex", "assets/textures"},
    //Shaders
    {".metal", "assets/shader-code"},
    {".hlsl", "assets/shader-code"},
    {".glsl", "assets/shader-code"},
    {".spv", "assets/shader-code"},
    {".slang", "assets/shader-code"},
    {".meta_ilenshader", "assets/shaders"},
    //Audio
    {".mp3", "assets/audio"},
    {".wav", "assets/audio"},
    {".flac", "assets/audio"},
    //Materials
    {".meta_ilenmat", "assets/materials"},
    //Manifiestos y otros datos del motor
    {".meta_ilenmanifest", "manifests"},
    {".meta_ilenschema", "manifests"},
    //Escenas
    {".meta_ilenscene", "scenes"}
};

std::unordered_map<std::string, std::string> folderToManifestAssetGroup = {
    {"models", ILEN_ASSET_MANIFEST_MODEL},
    {"textures", ILEN_ASSET_MANIFEST_TEXTURES},
    {"shaders", ILEN_ASSET_MANIFEST_SHADERS},
    {"shader-code", ILEN_ASSET_MANIFEST_SHADERS},
    {"audio", ILEN_ASSET_MANIFEST_AUDIO},
    {"materials", ILEN_ASSET_MANIFEST_MATERIALS},
};

static std::map<std::string, ILEN_GPU_RENDER_QUEUE_TYPE> stringToCorrectRenderQueue = {
    {"Opaque", ILEN_GPU_RENDER_QUEUE_TYPE::ILEN_GPU_RENDER_QUEUE_OPAQUE},
    {"AlphaTest", ILEN_GPU_RENDER_QUEUE_TYPE::ILEN_GPU_RENDER_QUEUE_ALPHA_TEST},
    {"Transparent",ILEN_GPU_RENDER_QUEUE_TYPE::ILEN_GPU_RENDER_QUEUE_TRANSPARENT},
    {"Overlay", ILEN_GPU_RENDER_QUEUE_TYPE::ILEN_GPU_RENDER_QUEUE_OVERLAY},
};

const std::vector<std::string> ILEN_ASSET_MANIFEST_ASSETS_TYPES = { ILEN_ASSET_MANIFEST_MODEL, ILEN_ASSET_MANIFEST_TEXTURES, ILEN_ASSET_MANIFEST_AUDIO, ILEN_ASSET_MANIFEST_MATERIALS, ILEN_ASSET_MANIFEST_SHADERS };

enum class CompileTargetOptions {
    Windows,
    Linux,
    macOS,
    macOS_arm,
    macOS_x86_64,
    Android,
    iOS,
    tvOS,
    Undefined,
};

CompileTargetOptions actualCompileOption = CompileTargetOptions::Undefined;

using json = nlohmann::json;

void createAndOrganizeFiles(std::filesystem::path ilenEngineFolder);
void binarizeFiles(std::filesystem::path ilenEngineFolder);
std::string generateFriendlyName(const std::string& filename);
ilen::graphics::MeshData loadModelData(std::filesystem::path modelPath);

ilen::job_system::JobSystem jobSystem;
//std::mutex fileWrite;

namespace nlohmann {
    template <>
    struct adl_serializer<ilen::graphics::Color> {
        static void from_json(const json& j, ilen::graphics::Color& c) {
            // Lógica para Array [r, g, b, a]
            if (j.is_array()) {
                if (j.size() >= 3) {
                    j.at(0).get_to(c.r);
                    j.at(1).get_to(c.g);
                    j.at(2).get_to(c.b);
                }
                if (j.size() >= 4) {
                    j.at(3).get_to(c.a);
                } else {
                    c.a = 1.0f;
                }
            }
            // Lógica para Objeto {"r": ...}
            else if (j.is_object()) {
                j.at("r").get_to(c.r);
                j.at("g").get_to(c.g);
                j.at("b").get_to(c.b);
                if (j.contains("a")) j.at("a").get_to(c.a);
                else c.a = 1.0f;
            }
        }
    };
}

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

std::vector<uint8_t> CompressToBC7(const uint8_t* pixels, int width, int height);
std::vector<uint8_t> CompressToASTC(const uint8_t* pixels, int width, int height);
void ApplyEdgeBleeding(uint8_t* pixels, int width, int height);

int main(int argc, char* argv[]){
    std::filesystem::path folderPath;

    jobSystem.Initializate();
    ilen::time::Time::Init();

    bc7enc_compress_block_init();

    for(int i = 0; i < argc; i++){
        std::string arg = argv[i];
        if(arg == "--ilen-folder" || arg == "-f"){
            if(i + 1 < argc){
                folderPath = argv[i + 1];
            } else {
                std::cerr << "Error: El indicador --ilen-folder requiere una ruta a una carpeta." << std::endl;
                return 1;
            }
        } else if(arg == "--compile-target" || arg == "-t"){
            if(i + 1 < argc){
                std::string target = std::string(argv[i + 1]);
                ILEN_LOG_P("Binarization", "Data: %s", target.c_str());
                if(target == "macOS"){
                    actualCompileOption = CompileTargetOptions::macOS;
                }else if(target == "iOS"){
                    actualCompileOption = CompileTargetOptions::iOS;
                }else if (target == "tvOS") {
                    actualCompileOption = CompileTargetOptions::tvOS;
                }else if(target == "windows") {
                    actualCompileOption = CompileTargetOptions::Windows;
                }else if(target == "linux") {
                    actualCompileOption = CompileTargetOptions::Linux;
                }else if(target == "android"){
                    actualCompileOption = CompileTargetOptions::Android;
                }else {
                    actualCompileOption = CompileTargetOptions::Undefined;
                }
            } else {
                std::cerr << "Error: El indicador --compile-target requiere un valor de target." << std::endl;
                return 1;
            }
        }
    }

    if(actualCompileOption == CompileTargetOptions::Undefined){
        std::cerr << "Se requiere un target para usar esta libreria. Añádela con el comando --compile-target <target>." << std::endl;
        return -1;
    }

    if(folderPath.empty()){
        return -1;
    }

    //Pasos para poder binarizar
    createAndOrganizeFiles(folderPath);
    binarizeFiles(folderPath);

    jobSystem.StopJobSystem();

    return 0;
}

void createAndOrganizeFiles(std::filesystem::path ilenEngineFolder)
{
    std::string m_manifestFileName = "assets.meta_ilenmanifest";

    std::filesystem::path engineSourceFolder = ilenEngineFolder;
    std::filesystem::path sourceAssetFolders = engineSourceFolder / "project_data" / "assets";
    std::filesystem::path manifestFolder = engineSourceFolder / "project_data" / "manifests";

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

    auto manifestPath = manifestFolder / m_manifestFileName;

    std::ifstream file(manifestPath);
    json data;

    if(file.is_open()){
        file >> data;
    } else {
        ILEN_LOG_P(DEBUG_PREFIX, "No se pudo abrir el archivo del manifiesto, o no existe en la ruta: %s", manifestPath.string().c_str());
        //Creamos una nueva lista
        auto predefinedPath = manifestFolder / "assets_defaults.meta_ilenmanifest";
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
        ".metal", ".hlsl", ".glsl", ".spv", ".meta_ilenmanifest", ".mtl", ".msl", ".slang", ".meta_ilentex", ".meta_ilenschema", ".meta_ilenscene"
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
    for(const auto& entry: std::filesystem::recursive_directory_iterator(sourceAssetFolders.parent_path())){
        if(entry.is_regular_file() && entry.path().filename() != m_manifestFileName && entry.path().filename() != ".DS_Store"){
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

        std::filesystem::path relativeToAssets = std::filesystem::relative(currentPath, sourceAssetFolders.parent_path());
        std::string firstFolder = relativeToAssets.begin()->string(); //Primera carpeta de la ruta relativa (revisa que este correctamente ubicado)
        if(firstFolder == "assets") {
            relativeToAssets = std::filesystem::relative(currentPath, sourceAssetFolders);
            firstFolder = relativeToAssets.begin()->string();
        }
        //ILEN_LOG_P(DEBUG_PREFIX, "First folder, %s. Relative TO assets: %s. Comparacion %s", firstFolder.c_str(), relativeToAssets.c_str(), std::filesystem::relative(targetFolder, "assets").c_str());

        std::string filteredTargetFolder = targetFolder.starts_with("assets") ? std::filesystem::relative(targetFolder, "assets").string() : targetFolder;

        if(firstFolder != filteredTargetFolder){
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

            std::filesystem::path finalDestinationPath = sourceAssetFolders.parent_path() / targetFolder / relativeToAssets;

            if(!std::filesystem::exists(finalDestinationPath.parent_path())){
                //Creamos el directorio si no existe
                std::filesystem::create_directories(finalDestinationPath.parent_path());
            }

            //Copiamos el recurso a su nueva carpeta
            std::filesystem::rename(currentPath, finalDestinationPath);
            ILEN_LOG_P(DEBUG_PREFIX, "Se copio el archivo %s", currentPath.c_str());
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
        //ILEN_LOG_P(DEBUG_PREFIX, "Revisando nueva lista de archivos");

        std::filesystem::path relativeToAssets = std::filesystem::relative(currentPath, sourceAssetFolders);


        auto result = std::find(existingPaths.begin(), existingPaths.end(), relativeToAssets.generic_string());

        if(result != existingPaths.end()) continue; //Si existe el path en el manifiesto, saltar al próximo.

        std::string extension = relativeToAssets.extension().string();
        auto it = extensionToAssetType.find(extension);

        std::string filteredTargetFolder = it->second.starts_with("assets") ? std::filesystem::relative(it->second, "assets").string() : it->second;

        //std::find(extensionCantRegistrate.begin(), extensionCantRegistrate.end(), it) == extensionCantRegistrate.end()

        //Guardar si la extensión existe y si esta en la carpeta correcta. Técnicamente no es necesario, pero no esta demás la comprobación, además de que esto solo esta en DEBUG. No añado las extensiones prohibidas al manifiesto.
        if(it != extensionToAssetType.end() && relativeToAssets.begin()->string() == filteredTargetFolder && std::find(extensionCantRegistrate.begin(), extensionCantRegistrate.end(), it->first) == extensionCantRegistrate.end()){
            needSaveNewManifest = true;
            ILEN_LOG_P(DEBUG_PREFIX, "Se necesita reescribir el manifiesto");


            uint32_t newID = ++maxID;
            std::string name = generateFriendlyName(currentPath.filename().string());

            json newAsset;
            newAsset[ILEN_ASSET_MANIFEST_ASSET_ID] = newID;
            newAsset[ILEN_ASSET_MANIFEST_ASSET_NAME] = name;
            newAsset[ILEN_ASSET_MANIFEST_ASSET_PATH] = relativeToAssets.generic_string();
            data[folderToManifestAssetGroup.find(filteredTargetFolder)->second].push_back(newAsset);
        }
    }

    //Escribir nuevo manifiesto con los nuevos datos si es necesario
    if(needSaveNewManifest){
        data[ILEN_ASSET_MANIFEST_TOTAL_OF_ELEMENTS] = maxID;

        //auto manifestPath = sourceAssetFolders / "manifests" / "assets.ilenmanifest";
        if(!std::filesystem::exists(manifestPath.parent_path()))
            std::filesystem::create_directories(manifestPath.parent_path());

        ILEN_LOG_P(DEBUG_PREFIX, "Path del manifiest %s", manifestPath.c_str());

        std::ofstream outFile(manifestPath);
        outFile << data.dump(2);
        outFile.close();

    }

    //assert(needSaveNewManifest == false && "Se escribió un nuevo manifiesto. Vuelve a iniciar la app para que ahora si funcione de forma correcta.");
    if (needSaveNewManifest)
    {
        createAndOrganizeFiles(ilenEngineFolder);
        return;
    }

    ILEN_WARNING_P(DEBUG_PREFIX, "Indexación de nuevos elementos completada. Generando binarios.");
}

//New version of generation and binarization ()
//(IA GENERATED CODE, not refactorized nor supervised creation) En este caso es un refuerzo del codigo mio, para soportar el nuevo sistema de carpetas más complejo. No debería dar problemas.
/*void createAndOrganizeFiles(std::filesystem::path ilenEngineFolder)
{
    static int recursionGuard = 0;
    recursionGuard++;
    if (recursionGuard > 5)
    {
        ILEN_ERROR_P(DEBUG_PREFIX, "Posible bucle infinito detectado en createAndOrganizeFiles");
        recursionGuard--;
        return;
    }

    std::string m_manifestFileName = "assets.meta_ilenmanifest";

    std::filesystem::path projectDataFolder = ilenEngineFolder / "project_data";
    std::filesystem::path assetsFolder      = projectDataFolder / "assets";
    std::filesystem::path manifestFolder    = projectDataFolder / "manifests";

    std::filesystem::create_directories(assetsFolder);
    std::filesystem::create_directories(manifestFolder);

    auto manifestPath = manifestFolder / m_manifestFileName;

    // Cargar manifiesto
    json data;
    std::ifstream file(manifestPath);
    if (file.is_open())
    {
        file >> data;
    }
    else
    {
        ILEN_LOG_P(DEBUG_PREFIX, "Manifiesto no encontrado, creando uno nuevo.");
        auto predefinedPath = manifestFolder / "assets_defaults.meta_ilenmanifest";
        std::ifstream predefined(predefinedPath);
        if (predefined.is_open())
            data = json::parse(predefined);
        else
        {
            data[ILEN_ASSET_MANIFEST_TOTAL_OF_ELEMENTS] = 0;
            data[ILEN_ASSET_MANIFEST_MODEL]     = json::array();
            data[ILEN_ASSET_MANIFEST_TEXTURES]  = json::array();
            data[ILEN_ASSET_MANIFEST_MATERIALS] = json::array();
            data[ILEN_ASSET_MANIFEST_AUDIO]     = json::array();
            data[ILEN_ASSET_MANIFEST_SHADERS]   = json::array();
        }
    }

    uint32_t maxID = data.value(ILEN_ASSET_MANIFEST_TOTAL_OF_ELEMENTS, 0);

    std::vector<std::string> existingAssetPaths;
    for (const auto& type : ILEN_ASSET_MANIFEST_ASSETS_TYPES)
    {
        if (data.contains(type))
            for (const auto& asset : data[type])
                existingAssetPaths.push_back(asset.value(ILEN_ASSET_MANIFEST_ASSET_PATH, ""));
    }

    const std::vector<std::string> extensionCantRegistrate = {
        ".metal", ".hlsl", ".glsl", ".spv", ".mtl", ".msl", ".slang",
        ".meta_ilentex", ".meta_ilenshader", ".meta_ilenmat",
        ".meta_ilenmanifest", ".meta_ilenschema", ".meta_ilenscene"
    };

    const std::vector<std::string> ignoredFiles = { ".DS_Store", "Thumbs.db", "desktop.ini" };

    // Escanear archivos
    std::vector<std::filesystem::path> filesToIndex;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(projectDataFolder))
    {
        if (!entry.is_regular_file()) continue;

        std::string filename = entry.path().filename().string();
        if (filename == m_manifestFileName) continue;
        if (std::find(ignoredFiles.begin(), ignoredFiles.end(), filename) != ignoredFiles.end()) continue;

        filesToIndex.push_back(entry.path());
    }

    bool folderReorganized = false;

    for (const auto& currentPath : filesToIndex)
    {
        std::string extension = currentPath.extension().string();
        auto it = extensionToAssetType.find(extension);

        std::string targetPrefix = (it != extensionToAssetType.end()) ? it->second : "unsupported";

        std::filesystem::path currentRelative = std::filesystem::relative(currentPath, projectDataFolder);

        // === NUEVA LÓGICA: calcular ruta deseada correctamente ===
        std::filesystem::path desiredRelative;

        if (targetPrefix.find("assets/") == 0)
        {
            // Archivos que deben estar dentro de assets/
            desiredRelative = std::filesystem::path("assets") / targetPrefix.substr(7);
        }
        else
        {
            // Otros (manifests, scenes, etc.)
            desiredRelative = std::filesystem::path(targetPrefix);
        }

        // Preservar jerarquía de subcarpetas
        if (currentRelative.begin() != currentRelative.end())
        {
            auto pathIt = currentRelative.begin();
            ++pathIt; // saltar la primera carpeta original
            for (; pathIt != currentRelative.end(); ++pathIt)
                desiredRelative /= *pathIt;
        }

        // Si ya está en el lugar correcto, no mover
        if (currentRelative == desiredRelative)
            continue;

        std::filesystem::path finalDestination = projectDataFolder / desiredRelative;

        if (!std::filesystem::exists(finalDestination.parent_path()))
            std::filesystem::create_directories(finalDestination.parent_path());

        try
        {
            std::filesystem::rename(currentPath, finalDestination);
            folderReorganized = true;
            ILEN_LOG_P(DEBUG_PREFIX, "Movido: %s → %s", 
                       currentPath.string().c_str(), 
                       finalDestination.string().c_str());
        }
        catch (const std::exception& e)
        {
            ILEN_ERROR_P(DEBUG_PREFIX, "Error al mover %s: %s", 
                         currentPath.string().c_str(), e.what());
        }
    }

    if (folderReorganized)
    {
        ILEN_WARNING_P(DEBUG_PREFIX, "Estructura reorganizada. Re-ejecutando indexación...");
        recursionGuard--;
        createAndOrganizeFiles(ilenEngineFolder);
        return;
    }

    // ==================== ACTUALIZAR MANIFIESTO ====================
    bool needSaveNewManifest = false;

    for (const auto& currentPath : filesToIndex)
    {
        std::filesystem::path relativeFromProjectData = std::filesystem::relative(currentPath, projectDataFolder);

        if (relativeFromProjectData.empty() || *relativeFromProjectData.begin() != "assets")
            continue;

        // Ruta que se guarda en el manifiesto (relativa a assets/)
        std::filesystem::path manifestRelative = std::filesystem::relative(currentPath, assetsFolder);
        std::string relStr = manifestRelative.generic_string();

        if (std::find(existingAssetPaths.begin(), existingAssetPaths.end(), relStr) != existingAssetPaths.end())
            continue;

        std::string extension = currentPath.extension().string();
        auto it = extensionToAssetType.find(extension);

        if (it != extensionToAssetType.end() &&
            std::find(extensionCantRegistrate.begin(), extensionCantRegistrate.end(), extension) == extensionCantRegistrate.end())
        {
            needSaveNewManifest = true;
            uint32_t newID = ++maxID;
            std::string name = generateFriendlyName(currentPath.filename().string());

            json newAsset;
            newAsset[ILEN_ASSET_MANIFEST_ASSET_ID]   = newID;
            newAsset[ILEN_ASSET_MANIFEST_ASSET_NAME] = name;
            newAsset[ILEN_ASSET_MANIFEST_ASSET_PATH] = relStr;

            std::string groupKey = it->second;
            if (groupKey.find("assets/") == 0)
                groupKey = groupKey.substr(7);

            auto groupIt = folderToManifestAssetGroup.find(groupKey);
            if (groupIt != folderToManifestAssetGroup.end())
                data[groupIt->second].push_back(newAsset);
        }
    }

    if (needSaveNewManifest)
    {
        data[ILEN_ASSET_MANIFEST_TOTAL_OF_ELEMENTS] = maxID;

        std::ofstream outFile(manifestPath);
        outFile << data.dump(2);
        outFile.close();

        ILEN_WARNING_P(DEBUG_PREFIX, "Manifiesto actualizado con nuevos assets.");
        recursionGuard--;
        createAndOrganizeFiles(ilenEngineFolder);
        return;
    }

    recursionGuard--;
    ILEN_WARNING_P(DEBUG_PREFIX, "Indexación y organización de project_data completada correctamente.");
}*/

std::string generateFriendlyName(const std::string& filename){

    std::string extension = std::filesystem::path(filename).extension().string();
    std::string name = std::filesystem::path(filename).stem().string();

    return name + "-" + extensionToAssetType.find(extension)->second;
}



void binarizeFiles(std::filesystem::path ilenEngineFolder) {
    using namespace ilen::resources;
    std::string m_manifestFileName = "assets.meta_ilenmanifest";

    std::filesystem::path engineSourceFolder = ilenEngineFolder;
    std::filesystem::path sourceAssetFolders = engineSourceFolder / "project_data" / "assets";
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

    auto manifestPath = sourceAssetFolders.parent_path() / "manifests" / m_manifestFileName;

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

        auto data = manifestData[ILEN_ASSET_MANIFEST_MODEL];

        //std::vector<std::future<void>> futures;
        size_t total = data.size();
        size_t threads = jobSystem.AvailableThreads();
        size_t chunkSize = (total + threads - 1)/ threads;
        size_t numChunks = (total + chunkSize - 1) / chunkSize;

        ILEN_LOG_P(DEBUG_PREFIX, "Datos Utiles: total %i, hilos %i, tamaño del chunk: %i, numero de chunks: %i", total, threads, chunkSize, numChunks);

        ilen::job_system::JobGroup group;

        std::vector<std::unordered_map<std::string, uint32_t>> localFriendlyNameToID(numChunks);
        std::vector<std::unordered_map<uint32_t, ModelAsset>> localIDToModelAsset(numChunks);

        for(size_t i = 0; i < numChunks; i++){
            size_t start = i * chunkSize;
            size_t end = std::min(start + chunkSize, total);

            ILEN_LOG_P(DEBUG_PREFIX, "Trabajando chunk %i, que va del indice %i al %i", i, start, end);


            jobSystem.SubmitJob([=, &sourceAssetFolders, &engineSourceFolder, &binaryModelFolder, &idToModelAsset, &friendlyNameToID, &data, &localFriendlyNameToID, &localIDToModelAsset]() {
                for(size_t j = start; j < end; j++){
                    std::string filePath = data[j][ILEN_ASSET_MANIFEST_ASSET_PATH];

                    auto modelPath = sourceAssetFolders / filePath;
                    ilen::graphics::MeshData toBinarize = loadModelData(modelPath);
                    auto savePath = binaryModelFolder / std::filesystem::relative(engineSourceFolder / filePath, engineSourceFolder / targetFolder);
                    savePath.replace_extension(".ilenmesh");
                    //Header del binario
                    ilen::binary::headers::IlenMeshHeader header{};
                    strncpy(header.magicNumber, "HSEM_NELI", sizeof(header.magicNumber) - 1);
                    header.version = 1;
                    header.vertexCount = toBinarize.vertexBufferData.size();
                    header.indexCount = toBinarize.indexBufferData.size();

                    
                    if(!std::filesystem::exists(savePath.parent_path())){
                        std::error_code ec; //Estp es para evitar que multiples hilos creen la misma carpeta, y de un error catastrofico
                        std::filesystem::create_directories(savePath.parent_path(), ec);
                    }

                    std::ofstream outFile(savePath, std::ios::binary);
                    outFile.write(reinterpret_cast<const char*>(&header), sizeof(ilen::binary::headers::IlenMeshHeader));
                    outFile.write(reinterpret_cast<const char*>(toBinarize.vertexBufferData.data()), sizeof(ilen::render::buffers::VertexBufferData) * toBinarize.vertexBufferData.size());
                    outFile.write(reinterpret_cast<const char*>(toBinarize.indexBufferData.data()), sizeof(uint32_t) * toBinarize.indexBufferData.size());

                    outFile.close();

                    //std::lock_guard<std::mutex> lock(fileWrite);

                    uint32_t id = data[j][ILEN_ASSET_MANIFEST_ASSET_ID];
                    std::string name = data[j][ILEN_ASSET_MANIFEST_ASSET_NAME];
                    std::string path = std::filesystem::relative(savePath, binaryModelFolder);

                    localFriendlyNameToID[i][name] = id;
                    localIDToModelAsset[i].emplace(id, ModelAsset(id, path));
                    //friendlyNameToID[name] = id;
                    //idToModelAsset.emplace(id, ModelAsset(id, path));
                }
            }, &group);
        }

        group.Wait();

        //Asignado las IDs
        for(size_t i = 0; i < numChunks; i++){
            for(auto [name, id]: localFriendlyNameToID[i]){
                friendlyNameToID[name] = id;
            }
            for(auto [id, asset]: localIDToModelAsset[i]){
                idToModelAsset.emplace(id, asset);
            }
        }

        /*for(const json& assetData : manifestData[ILEN_ASSET_MANIFEST_MODEL]){
            std::string filePath = assetData[ILEN_ASSET_MANIFEST_ASSET_PATH];
            futures.push_back(
                jobSystem.SubmitJob([=, &sourceAssetFolders, &engineSourceFolder, &binaryModelFolder, &idToModelAsset, &friendlyNameToID, &assetData]() {
                    auto modelPath = sourceAssetFolders / filePath;
                    ilen::graphics::MeshData toBinarize = loadModelData(modelPath);
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
        }*/
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

            std::string originalFilePath = shaderData[ILEN_ASSET_MANIFEST_ASSET_PATH];

            auto sourcePath = sourceAssetFolders / originalFilePath;
            auto savePath = binaryDataFolder / std::filesystem::relative(sourcePath, engineSourceFolder / "project_data/assets");
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

            savePath.replace_extension(".ilenshader");

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
    if(manifestData.contains(ILEN_ASSET_MANIFEST_MATERIALS)){
        std::string targetFolder = "materials";
        std::filesystem::path materialBinaryFolder = binaryDataFolder / targetFolder;
        if (!std::filesystem::exists(materialBinaryFolder))
        {
            std::filesystem::create_directories(materialBinaryFolder);
        }

        for (const auto& materialData: manifestData[ILEN_ASSET_MANIFEST_MATERIALS])
        {

            std::string originalFilePath = materialData[ILEN_ASSET_MANIFEST_ASSET_PATH];

            auto sourcePath = sourceAssetFolders / originalFilePath;
            auto savePath = binaryDataFolder / std::filesystem::relative(sourcePath, engineSourceFolder / "project_data/assets");
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

            std::string renderQueueString = materialJson.value(ILEN_MATERIAL_RENDER_QUEUE, "Opaque");
            auto it = stringToCorrectRenderQueue.find(renderQueueString);
            if (it != stringToCorrectRenderQueue.end()) {
                material.renderQueue = it->second;
            }else {
                material.renderQueue = ilen::enumerations::ILEN_GPU_RENDER_QUEUE_TYPE::ILEN_GPU_RENDER_QUEUE_OPAQUE;
            }

            // Procesar todas las propiedades con la nueva lógica
            auto propData = materialJson.value(ILEN_MATERIAL_PROPERTIES_GROUP, json::array());
            processScalarProperties<float>(propData, ILEN_MATERIAL_FLOAT_PROPERTY, material.properties);
            processScalarProperties<uint32_t>(propData, ILEN_MATERIAL_INTEGER_PROPERTY, material.properties);
            processScalarProperties<uint32_t>(propData, ILEN_MATERIAL_TEXTURE_PROPERTY, material.properties); // Las texturas se guardan como ID (uint32_t)

            processVectorProperties<ilen::maths::vector::vec2>(propData, ILEN_MATERIAL_FLOAT2_PROPERTY, material.properties);
            processVectorProperties<ilen::maths::vector::vec3>(propData, ILEN_MATERIAL_FLOAT3_PROPERTY, material.properties);
            processVectorProperties<ilen::maths::vector::vec4>(propData, ILEN_MATERIAL_FLOAT4_PROPERTY, material.properties);
            processVectorProperties<ilen::graphics::Color>(propData, ILEN_MATERIAL_COLOR_PROPERTY, material.properties);

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

            savePath.replace_extension(".ilenmat");


            //Orden de escritura!!!
            //Header -> Ints del material (incluye el RenderQueue) -> strings del material ->  Properties (TODO)
            std::ofstream outFile(savePath,std::ios::binary);
            outFile.write(reinterpret_cast<const char*>(&header),sizeof(ilen::binary::headers::IlenMaterialHeader));
            outFile.write(reinterpret_cast<const char*>(&material.vertexShaderID), sizeof(uint32_t));
            outFile.write(reinterpret_cast<const char*>(&material.fragmentShaderID), sizeof(uint32_t));
            outFile.write(reinterpret_cast<const char *>(&material.renderQueue), sizeof(ILEN_GPU_RENDER_QUEUE_TYPE));
            outFile.write(material.materialName.c_str(), header.materialNameSize);
            outFile.write(material.vertexFunctionName.c_str(), header.vertexFunctionSize);
            outFile.write(material.fragmentFunctionName.c_str(), header.fragmentFunctionSize);

            friendlyNameToID[name] = id;
            idToMaterialAsset.emplace(id, MaterialAsset(id, std::filesystem::relative(savePath, materialBinaryFolder)));
        }
    }

    //Guardando las texturas
    if(manifestData.contains(ILEN_ASSET_MANIFEST_TEXTURES)) {
        std::string targetFolder = "textures";
        std::filesystem::path binaryTextureFolder = binaryDataFolder / targetFolder;

        if(!std::filesystem::exists(binaryTextureFolder)) {
            std::filesystem::create_directories(binaryTextureFolder);
        }

        auto data = manifestData[ILEN_ASSET_MANIFEST_TEXTURES];
        size_t total = data.size();
        size_t threads = jobSystem.AvailableThreads();
        size_t chunkSize = (total + threads - 1) / threads;
        size_t numChunks = (total + chunkSize - 1) / chunkSize;

        ILEN_LOG_P(DEBUG_PREFIX, "Texturas - Total: %i, Chunks: %i", total, numChunks);

        ilen::job_system::JobGroup group;
        std::vector<std::unordered_map<std::string, uint32_t>> localFriendlyNameToID(numChunks);
        std::vector<std::unordered_map<uint32_t, TextureAsset>> localIDToTextureAsset(numChunks);

        for(size_t i = 0; i < numChunks; i++) {
            size_t start = i * chunkSize;
            size_t end = std::min(start + chunkSize, total);

            jobSystem.SubmitJob([=, &sourceAssetFolders, &engineSourceFolder, &binaryTextureFolder, &data, &localFriendlyNameToID, &localIDToTextureAsset]() {
                for(size_t j = start; j < end; j++){
                    std::string filePath = data[j][ILEN_ASSET_MANIFEST_ASSET_PATH];
                    auto texturePath = sourceAssetFolders / filePath;

                    // Calculamos la ruta de destino ANTES de hacer nada
                    auto metaPath = texturePath;
                    metaPath.replace_extension(".meta_ilentex");

                    auto savePath = binaryTextureFolder / std::filesystem::relative(texturePath, engineSourceFolder / "project_data/assets/textures");
                    savePath.replace_extension(".ilentex");

                    ILEN_LOG_P(DEBUG_PREFIX, "Path de la textura es: %s", texturePath.c_str());
                    ILEN_LOG_P(DEBUG_PREFIX, "Engine sources folder es: %s", engineSourceFolder.c_str());
  


                    if(!std::filesystem::exists(metaPath)){
                        //No existe metadatos, los creamos.
                        nlohmann::json defaultMetadata = {
                            {"version", 1},
                            {"settings", {
                                {"compress", true},
                                {"sRGB", true},
                                {"filter", "linear"}
                            }},
                            {
                                "atlas", {
                                    {"enabled", false},
                                    {"sprites", nlohmann::json::array()}
                                }
                            }
                        };

                        std::ofstream newMetaFile(metaPath);
                        newMetaFile << defaultMetadata.dump(4);
                        newMetaFile.close();

                        ILEN_LOG_P(DEBUG_PREFIX, "Auto-generado .meta_ilentex por defecto para: %s", filePath.c_str());
                    }

                    // --- 1. LÓGICA DE CACHÉ ---
                    bool needsBinarization = true;

                    if (std::filesystem::exists(savePath)) {
                        // Obtener la fecha de la última modificación de ambos archivos
                        auto sourceTime = std::filesystem::last_write_time(texturePath);
                        auto targetTime = std::filesystem::last_write_time(savePath);
                        auto metaTime = std::filesystem::last_write_time(metaPath);

                        bool pngChanged = sourceTime > targetTime;
                        bool metaChanged = metaTime > targetTime;

                        // Si el binario es más nuevo o igual que el PNG original, no hay cambios.
                        if (!pngChanged && !metaChanged) {
                            needsBinarization = false;
                        }
                    }

                    // Si no necesita binarizarse, nos saltamos el costo de CPU
                    if (!needsBinarization) {
                        // ¡IMPORTANTE! Aunque no comprimamos, TENEMOS que registrar la textura en el mapa 
                        // para que el assets.b_ilenmanifest final sepa que existe.
                        uint32_t id = data[j][ILEN_ASSET_MANIFEST_ASSET_ID];
                        std::string name = data[j][ILEN_ASSET_MANIFEST_ASSET_NAME];
                        std::string path = std::filesystem::relative(savePath, binaryTextureFolder).string();

                        localFriendlyNameToID[i][name] = id;
                        localIDToTextureAsset[i].emplace(id, TextureAsset(id, path));
                        
                        ILEN_LOG_P(DEBUG_PREFIX, "Saltando textura ya cacheada: %s", name.c_str());
                        continue; // Pasar a la siguiente textura del bucle
                    }

                    int width, height, channels;
                    //Dato importante, stbi_load fuerza la carga en 4 canales, algo importante para la compresión de las texturas.
                    uint8_t* rawPixels = stbi_load(texturePath.string().c_str(), &width, &height, &channels, 4);

                    if(!rawPixels) {
                        ILEN_WARNING_P(DEBUG_PREFIX, "Fallo la carga de la textura cruda: %s", texturePath.string().c_str());
                        continue;
                    }

                    ApplyEdgeBleeding(rawPixels, width, height); //Filtrado de la textura :)
                    
                    //Parseo de los metadatos y calculo del atlas de la textura
                    std::vector<ilen::binary::headers::IlenAtlasSpriteInfo> atlasSprite;
                    bool compressTexture = true;

                    if(std::filesystem::exists(metaPath)) {
                        std::ifstream metaFile(metaPath);
                        json metaJson;
                        metaFile >> metaJson;

                        if(metaJson.contains("settings") && metaJson["settings"].contains("compress")){
                            compressTexture = metaJson["settings"]["compress"];
                        }

                        //Calculo de las UV (0 - 1) para cada sprite en la textura
                        if(metaJson.contains("atlas") && metaJson["atlas"]["enabled"] == true){
                            for(const auto& sprite : metaJson["atlas"]["sprites"]) {
                                ilen::binary::headers::IlenAtlasSpriteInfo spriteInfo{};
                                spriteInfo.id = sprite["id"];

                                float x = sprite["x"];
                                float y = sprite["y"];
                                float w = sprite["w"];
                                float h = sprite["h"];

                                //Conversion a espacio UV
                                spriteInfo.uvRect[0] = x / static_cast<float>(width);
                                spriteInfo.uvRect[1] = y / static_cast<float>(height);
                                spriteInfo.uvRect[2] = w / static_cast<float>(width);
                                spriteInfo.uvRect[3] = h / static_cast<float>(height);

                                atlasSprite.push_back(spriteInfo);
                            }
                        }
                    } 


                    std::vector<ilen::binary::headers::TexturePayloadInfo> toc; //1 payload por cada textura usada
                    std::vector<std::vector<uint8_t>> compressedBlobs;

                    bool needsASTC = compressTexture && (actualCompileOption == CompileTargetOptions::macOS || actualCompileOption == CompileTargetOptions::macOS_arm || actualCompileOption == CompileTargetOptions::iOS || actualCompileOption == CompileTargetOptions::tvOS || actualCompileOption == CompileTargetOptions::Android);
                    bool needsBC7 = compressTexture && (actualCompileOption == CompileTargetOptions::Windows || actualCompileOption == CompileTargetOptions::Linux || actualCompileOption == CompileTargetOptions::macOS || actualCompileOption == CompileTargetOptions::macOS_x86_64);
                
                    //Comprimir a ASTC
                    if(needsASTC){
                        auto blob = CompressToASTC(rawPixels, width, height);
                        compressedBlobs.push_back(blob);

                        ilen::binary::headers::TexturePayloadInfo info{};
                        info.format = ilen::enumerations::ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_ASTC_4X4_UNORM;
                        info.dataSize = blob.size();
                        toc.push_back(info);
                    }

                    //Comprimir usando BC7
                    if(needsBC7) {
                        auto blob = CompressToBC7(rawPixels, width, height);
                        compressedBlobs.push_back(blob);

                        ilen::binary::headers::TexturePayloadInfo info{};
                        info.format = ilen::enumerations::ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_BC7_UNORM;
                        info.dataSize = blob.size();
                        toc.push_back(info);
                    }

                    //Fallback por si es desconocida la plataforma
                    if(toc.empty()){
                        size_t byteSize = width * height * 4;
                        std::vector<uint8_t> blob(rawPixels, rawPixels + byteSize);
                        compressedBlobs.push_back(blob);

                        ilen::binary::headers::TexturePayloadInfo info{};
                        info.format = ilen::enumerations::ILEN_GPU_TEXTURE_PIXEL_FORMAT_TYPE::ILEN_GPU_TEXTURE_PIXEL_FORMAT_RGBA8_UNORM;
                        info.dataSize = blob.size();
                        toc.push_back(info);
                    }

                    stbi_image_free(rawPixels);

                    //Calculo de los offsets
                    uint64_t baseDataSize = sizeof(ilen::binary::headers::IlenTextureHeader) +
                        (sizeof(ilen::binary::headers::TexturePayloadInfo) * toc.size()) +
                        (sizeof(ilen::binary::headers::IlenAtlasSpriteInfo) * atlasSprite.size());
                    
                    uint64_t currentOffset = baseDataSize;

                    for(size_t k = 0; k < toc.size(); k++){
                        toc[k].offset = currentOffset;
                        currentOffset += toc[k].dataSize;
                    }

                    //Generar el archivo binario
                    ilen::binary::headers::IlenTextureHeader header{};
                    strncpy(header.magicNumber, "ERUTXET_NELI", sizeof(header.magicNumber) - 1);
                    header.version = 1;
                    header.width = width;
                    header.height = height;
                    header.channels = 4;
                    header.mipLevels = 1;
                    header.payloadCount = toc.size();
                    header.atlasSpriteCount = atlasSprite.size();

                    //auto savePath = binaryTextureFolder / std::filesystem::relative(engineSourceFolder / filePath, engineSourceFolder / targetFolder);
                    //savePath.replace_extension(".ilentex");

                    if(!std::filesystem::exists(savePath.parent_path())){
                        std::error_code ec;
                        std::filesystem::create_directories(savePath.parent_path(), ec);
                    }

                    std::ofstream outFile(savePath, std::ios::binary);
                    //Escribiendo los datos
                    outFile.write(reinterpret_cast<const char*>(&header), sizeof(ilen::binary::headers::IlenTextureHeader));
                    outFile.write(reinterpret_cast<const char*>(toc.data()), sizeof(ilen::binary::headers::TexturePayloadInfo) * toc.size());
                    if(!atlasSprite.empty()) {
                        outFile.write(reinterpret_cast<const char*>(atlasSprite.data()), sizeof(ilen::binary::headers::IlenAtlasSpriteInfo) * atlasSprite.size());
                    }
                    for(const auto& blob: compressedBlobs){
                        outFile.write(reinterpret_cast<const char*>(blob.data()), blob.size());
                    }
                    outFile.close();

                    //Registrar en el manifiesto
                    uint32_t id = data[j][ILEN_ASSET_MANIFEST_ASSET_ID];
                    std::string name = data[j][ILEN_ASSET_MANIFEST_ASSET_NAME];

                    ILEN_LOG_P(DEBUG_PREFIX, "Save Path de la textura: %s", savePath.c_str());
                    ILEN_LOG_P(DEBUG_PREFIX, "Binary Data Folder: %s", binaryDataFolder.c_str());

                    std::string path = std::filesystem::relative(savePath, binaryDataFolder / "textures").string();
                    ILEN_LOG_P(DEBUG_PREFIX, "Path guardado es : %s", path.c_str());

                    localFriendlyNameToID[i][name] = id;
                    localIDToTextureAsset[i].emplace(id, TextureAsset(id, path));
                }   
            }, &group);
        }

        group.Wait();

        //Integrar los resultados en el hilo principal
        for(size_t i = 0; i < numChunks; i++){
            for(auto [name, id]: localFriendlyNameToID[i]){
                friendlyNameToID[name] = id;
            }
            for(auto [id, asset]: localIDToTextureAsset[i]){
                idToTextureAsset.emplace(id, asset);
            }
        }

        ILEN_LOG_P(DEBUG_PREFIX, "Texturas binarizadas de forma correcta");
    }

    //3 - Generar un manifiesto para los binarios.
    //No necesito un manifiesto en JSON, mejor volcar los mapas de memoria en un archivo, así no tenemos que volver a cargar y configurar estos, sino que sea solo una lectura.

    //4 - Guardar ese manifiesto
    std::filesystem::path manifiestPath = binaryDataFolder / "manifests" / "assets.ilenmanifest";
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

ilen::graphics::MeshData loadModelData(std::filesystem::path modelPath){
        //std::filesystem::path meshFilePath = core::FileManager::fileRoute(meshPath, ILEN_FILE_TYPE_NONE);
        Assimp::Importer importer;

        const aiScene* scene = importer.ReadFile(modelPath, aiProcess_Triangulate | aiProcess_ConvertToLeftHanded | aiProcess_JoinIdenticalVertices);        
        if(!scene || !scene->HasMeshes()){
            ILEN_WARNING_P(DEBUG_PREFIX, "El objeto en la ruta %s no contenía ningún dato del tipo Mesh o ocurrió un error. Favor de revisar la información del archivo. Error: %s", modelPath.string().c_str(), importer.GetErrorString());
            return ilen::graphics::MeshData{};
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

        ilen::graphics::MeshData meshData{};
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
    if (!jsonData.is_array()) {
        ILEN_ERROR_P(DEBUG_PREFIX, "Recibimos un objeto en vez de una lista de propiedades. Revisar el .ilenmat usado");
        return;
    }

    for (const auto& item: jsonData) {
        ILEN_LOG_P(DEBUG_PREFIX, "Objeto válido");
        if (item.contains(key)) {
            ILEN_LOG_P(DEBUG_PREFIX, "Contiene llave");
            const auto& property = item[key];

            if (property.contains(ILEN_MATERIAL_PROPERTY_NAME) && property.contains(ILEN_MATERIAL_PROPERTY_VALUE)) {
                // DEBUG TEMPORAL
                if (property.is_array()) {
                    ILEN_LOG_P(DEBUG_PREFIX, "ERROR INMINENTE: 'property' YA es un array: %s", property.dump().c_str());
                } else if (property.is_object()) {
                    ILEN_LOG_P(DEBUG_PREFIX, "INFO: 'property' es un objeto (correcto): %s", property.dump().c_str());
                } else {
                    ILEN_LOG_P(DEBUG_PREFIX, "INFO: 'property' es otro tipo: %s", property.dump().c_str());
                }
                ILEN_LOG_P(DEBUG_PREFIX, "Contiene elementos deseados");
                std::string propertyName = property[ILEN_MATERIAL_PROPERTY_NAME].get<std::string>();
                ILEN_LOG_P(DEBUG_PREFIX, "Leido el nombre %s", propertyName.c_str());
                VecType value;

                property[ILEN_MATERIAL_PROPERTY_VALUE].get_to(value);
                properties[propertyName] = value;
            }
        }
    }
}

//Funciones para la ayuda de compresión de las texturas (IA GENERATED CODE, not refactorized nor supervised creation) <-- Esto significa que es un posible vector de probelmas, ya que no se como funciona su algoritmo, porque no lo supervise, solo fue Ctrl+C, Ctrl+V
// --- COMPRESIÓN BC7 (PC) ---
std::vector<uint8_t> CompressToBC7(const uint8_t* pixels, int width, int height) {
    // BC7 usa bloques de 4x4 pixeles. Cada bloque se comprime a 16 bytes.
    int blocksX = (width + 3) / 4;
    int blocksY = (height + 3) / 4;
    
    std::vector<uint8_t> compressedData(blocksX * blocksY * 16);
    uint8_t* destPtr = compressedData.data();

    bc7enc_compress_block_params params;
    bc7enc_compress_block_params_init(&params); // Calidad por defecto
    // params.m_max_partitions_mode = ... (Puedes ajustar la calidad aquí)

    for (int y = 0; y < height; y += 4) {
        for (int x = 0; x < width; x += 4) {
            uint32_t block4x4[16] = {0}; // 16 pixeles RGBA (4x4)

            // Extraer el bloque de 4x4 de la imagen original (con padding si estamos en los bordes)
            for (int by = 0; by < 4; ++by) {
                for (int bx = 0; bx < 4; ++bx) {
                    int px = std::min(x + bx, width - 1);
                    int py = std::min(y + by, height - 1);
                    // stb_image devuelve pixeles en formato RGBA (4 canales, 8 bits cada uno)
                    const uint8_t* pSrc = pixels + ((py * width) + px) * 4;
                    // Empaquetar RGBA en un uint32_t (Little Endian)
                    block4x4[by * 4 + bx] = pSrc[0] | (pSrc[1] << 8) | (pSrc[2] << 16) | (pSrc[3] << 24);
                }
            }

            // Comprimir el bloque y avanzar el puntero de destino
            bc7enc_compress_block(destPtr, block4x4, &params);
            destPtr += 16;
        }
    }
    return compressedData;
}

// --- COMPRESIÓN ASTC (APPLE / MÓVIL) ---
std::vector<uint8_t> CompressToASTC(const uint8_t* pixels, int width, int height) {
    // Configuración para ASTC 4x4 (Excelente calidad, 8 bits por pixel)
    astcenc_config config;
    astcenc_config_init(ASTCENC_PRF_LDR, 4, 4, 1, ASTCENC_PRE_MEDIUM, 0, &config);

    astcenc_context* context;
    astcenc_context_alloc(&config, 1, &context, nullptr); // 1 hilo por contexto interno

    astcenc_image image;
    image.dim_x = width;
    image.dim_y = height;
    image.dim_z = 1;
    image.data_type = ASTCENC_TYPE_U8;
    
    // ASTC pide un array de punteros a los "slices" (solo tenemos 1 porque es 2D)
    void* data_slices[1] = { (void*)pixels };
    image.data = data_slices;

    // Calcular tamaño de salida (ASTC 4x4 da 16 bytes por bloque)
    int blocksX = (width + 3) / 4;
    int blocksY = (height + 3) / 4;
    std::vector<uint8_t> compressedData(blocksX * blocksY * 16);

    astcenc_swizzle swizzle = {
        ASTCENC_SWZ_R,
        ASTCENC_SWZ_G,
        ASTCENC_SWZ_B,
        ASTCENC_SWZ_A,
    };

    // Comprimir
    astcenc_compress_image(context, &image, &swizzle, compressedData.data(), compressedData.size(), 0);

    astcenc_context_free(context);
    return compressedData;
}

void ApplyEdgeBleeding(uint8_t* pixels, int width, int height) {
    // Un pase simple de dilatación para píxeles con Alpha 0
    // Si un píxel es invisible, busca un vecino válido y copia su RGB
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            size_t idx = ((y * width) + x) * 4;
            
            if (pixels[idx + 3] == 0) { // Si el alpha es 0
                // Buscar un vecino opaco (arriba, abajo, izquierda, derecha)
                int neighborsX[] = {0, 0, -1, 1};
                int neighborsY[] = {-1, 1, 0, 0};
                
                for (int i = 0; i < 4; ++i) {
                    int nx = x + neighborsX[i];
                    int ny = y + neighborsY[i];
                    
                    if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                        size_t nIdx = ((ny * width) + nx) * 4;
                        if (pixels[nIdx + 3] > 0) { // Si el vecino es opaco
                            pixels[idx]     = pixels[nIdx];     // R
                            pixels[idx + 1] = pixels[nIdx + 1]; // G
                            pixels[idx + 2] = pixels[nIdx + 2]; // B
                            break; // Ya encontramos color, pasamos al siguiente
                        }
                    }
                }
            }
        }
    }
}