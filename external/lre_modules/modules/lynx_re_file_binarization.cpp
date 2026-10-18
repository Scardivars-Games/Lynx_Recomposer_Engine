#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
//#include <sstream>
#include <filesystem>
#include <fstream>
#include <json/json.hpp>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_map>
#define STB_IMAGE_IMPLEMENTATION //Esto es para el STB
#include <images/stb_image.h>
#include <lynx/headers/lynx_logs.h>
#include <lynx/headers/enumerations.h>
#include <lynx/graphics/meshes.h>
#include <lynx/resources/binary_headers.h>
#include <lynx/resources/assets_elements.h>
#include <lynx/graphics/material.h>
#include <lynx/graphics/shader.h>
#include <lynx/graphics/color.h>
#include <lynx/core/time.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <lynx/job_system/job_system.h>
#include <astcenc.h>
#include <bc7enc.h>
#include <set>

namespace lynx::core::threading {
    std::thread::id g_MainThreadId;
    thread_local bool t_IsMainThread = false;
    thread_local size_t t_ThreadIndex = UNREGISTERED_THREAD;
}

using namespace lynx::enumerations;
constexpr const char* DEBUG_PREFIX = "FILE_BINARIZATION";

//CONSTANTE TIPOS TO AVOID TYPING ERRORS
const std::string LYNX_ASSET_MANIFEST_TOTAL_OF_ELEMENTS = "totalOfElements";
const std::string LYNX_ASSET_MANIFEST_ASSET_ID = "id";
const std::string LYNX_ASSET_MANIFEST_ASSET_NAME = "name";
const std::string LYNX_ASSET_MANIFEST_ASSET_PATH = "path";
const std::string LYNX_ASSET_MANIFEST_MODEL = "models";
const std::string LYNX_ASSET_MANIFEST_TEXTURES = "textures";
const std::string LYNX_ASSET_MANIFEST_MATERIALS = "materials";
const std::string LYNX_ASSET_MANIFEST_AUDIO = "audio";
const std::string LYNX_ASSET_MANIFEST_SHADERS = "shaders";

//CONSTANTES PARA SHADERS
const std::string LYNX_SHADER_FILE_NAME = "shader-file-name";
const std::string LYNX_SHADER_DIRECTORY = "shader-dir";
const std::string LYNX_SHADER_HUMAN_REDEABLE_NAME = "shader-name";

//CONSTANTES PARA MATERIALES
const std::string LYNX_MATERIAL_NAME = "material-name";
const std::string LYNX_MATERIAL_VERTEX_SHADER_ID = "vertex-shader-id";
const std::string LYNX_MATERIAL_FRAGMENT_SHADER_ID = "fragment-shader-id";
const std::string LYNX_MATERIAL_VERTEX_SHADER_FUNCTION = "vertex-shader-function";
const std::string LYNX_MATERIAL_FRAGMENT_SHADER_FUNCTION = "fragment-shader-function";
const std::string LYNX_MATERIAL_RENDER_QUEUE = "render-queue";

//Keys for properties, to be added in the future
const std::string LYNX_MATERIAL_PROPERTIES_GROUP = "properties";
const std::string LYNX_MATERIAL_FLOAT_PROPERTY = "float-property";
const std::string LYNX_MATERIAL_INTEGER_PROPERTY = "int-property";
const std::string LYNX_MATERIAL_FLOAT2_PROPERTY = "float2-property";
const std::string LYNX_MATERIAL_FLOAT3_PROPERTY = "float3-property";
const std::string LYNX_MATERIAL_FLOAT4_PROPERTY = "float4-property";
const std::string LYNX_MATERIAL_COLOR_PROPERTY = "color-property"; //Necesario, porque color puede admintir propiedades HDR.
const std::string LYNX_MATERIAL_TEXTURE_PROPERTY = "texture-property"; // Para texturas

const std::string LYNX_MATERIAL_PROPERTY_NAME = "name";
const std::string LYNX_MATERIAL_PROPERTY_VALUE = "value";

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
    {".meta_lynxtex", "assets/textures"},
    //Shaders
    {".metal", "assets/shader_code"},
    {".hlsl", "assets/shader_code"},
    {".glsl", "assets/shader_code"},
    {".spv", "assets/shader_code"},
    {".slang", "assets/shader_code"},
    {".meta_lynxshader", "assets/shaders"},
    //Audio
    {".mp3", "assets/audio"},
    {".wav", "assets/audio"},
    {".flac", "assets/audio"},
    //Prefabs
    {".meta_lynxprefab", "assets/prefabs"},
    //Materials
    {".meta_lynxmat", "assets/materials"},
    //Manifiestos y otros datos del motor
    {".meta_lynxmanifest", "manifests"},
    {".meta_lynxschema", "manifests"},
    {".inl", "manifests"},
    //Escenas
    {".meta_lynxscene", "scenes"}
};

std::unordered_map<std::string, std::string> folderToManifestAssetGroup = {
    {"models", LYNX_ASSET_MANIFEST_MODEL},
    {"textures", LYNX_ASSET_MANIFEST_TEXTURES},
    {"shaders", LYNX_ASSET_MANIFEST_SHADERS},
    {"shader_code", LYNX_ASSET_MANIFEST_SHADERS},
    {"audio", LYNX_ASSET_MANIFEST_AUDIO},
    {"materials", LYNX_ASSET_MANIFEST_MATERIALS},
};

static std::map<std::string, LYNX_GPU_RENDER_QUEUE_TYPE> stringToCorrectRenderQueue = {
    {"Opaque", LYNX_GPU_RENDER_QUEUE_TYPE::LYNX_GPU_RENDER_QUEUE_OPAQUE},
    {"AlphaTest", LYNX_GPU_RENDER_QUEUE_TYPE::LYNX_GPU_RENDER_QUEUE_ALPHA_TEST},
    {"Transparent",LYNX_GPU_RENDER_QUEUE_TYPE::LYNX_GPU_RENDER_QUEUE_TRANSPARENT},
    {"Overlay", LYNX_GPU_RENDER_QUEUE_TYPE::LYNX_GPU_RENDER_QUEUE_OVERLAY},
};

const std::vector<std::string> LYNX_ASSET_MANIFEST_ASSETS_TYPES = { LYNX_ASSET_MANIFEST_MODEL, LYNX_ASSET_MANIFEST_TEXTURES, LYNX_ASSET_MANIFEST_AUDIO, LYNX_ASSET_MANIFEST_MATERIALS, LYNX_ASSET_MANIFEST_SHADERS };

enum class CompileTargetOptions {
    Windows,
    Linux,
    macOS,
    macOS_arm,
    macOS_x86_64,
    Android,
    iOS,
    iOS_simulator,
    tvOS,
    tvOS_simulator,
    Undefined,
};

enum class AssetBuildMode {
    Engine,
    Project
};

struct AssetBuildOptions {
    AssetBuildMode mode = AssetBuildMode::Project;
    std::filesystem::path sourceAssets;
    std::filesystem::path sourceManifest;
    std::filesystem::path output;

    //Para cuando se compile el proyecto, sepa donde esta los assets predefinidos
    std::filesystem::path predefinedAssets;
    std::filesystem::path predefinedManifest;
};

AssetBuildOptions parseBuildOptions(int argc, char* argv[]);

void buildEngineAssets(const AssetBuildOptions& options);
void buildProjectAssets(const AssetBuildOptions& options);

using json = nlohmann::json;
using file_route = std::filesystem::path;

CompileTargetOptions actualCompileOption = CompileTargetOptions::Undefined;

void createAndOrganizeFiles(std::filesystem::path lynxEngineFolder);
void binarizeFiles(std::filesystem::path lynxEngineFolder);
std::string generateFriendlyName(const std::string& filename);
lynx::graphics::MeshData loadModelData(std::filesystem::path modelPath);

json readJsonFile(const file_route& path);
void writeJsonFile(const file_route& path, const json& data);
void copyDirectoryContents(const file_route& source, const file_route& destination);
json mergeProjectManifest(const json& predefinedManifest, const json& currentProjectManifest);
json createProjectOnlyManifest(const json& completeManifest, const json& predefinedManifest);
void writeRuntimeAssetManifest(const json& completeManifest, const file_route& binaryAssetsFolder);

lynx::job_system::JobSystem jobSystem;
//std::mutex fileWrite;

namespace nlohmann {
    template <>
    struct adl_serializer<lynx::graphics::Color> {
        static void from_json(const json& j, lynx::graphics::Color& c) {
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

namespace lynx::maths::vector {
    // Le enseña a nlohmann/json cómo convertir un array JSON a un lynx::maths::vector
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
void processScalarProperties(const nlohmann::json& jsonData, const std::string& key, std::unordered_map<std::string, lynx::graphics::MaterialProperty>& properties);

template<typename VecType>
void processVectorProperties(const nlohmann::json& jsonData, const std::string& key, std::unordered_map<std::string, lynx::graphics::MaterialProperty>& properties);

std::vector<uint8_t> CompressToBC7(const uint8_t* pixels, int width, int height);
std::vector<uint8_t> CompressToASTC(const uint8_t* pixels, int width, int height);
void ApplyEdgeBleeding(uint8_t* pixels, int width, int height);

int main(int argc, char* argv[]){
    try {
        const AssetBuildOptions options = parseBuildOptions(argc, argv);

        jobSystem.Initializate();
        lynx::time::Time::Init();
        bc7enc_compress_block_init();

        if(options.mode == AssetBuildMode::Engine) {
            buildEngineAssets(options);
        } else {
            buildProjectAssets(options);
        }

        jobSystem.StopJobSystem();
        return 0;
    }catch(const std::exception& exception) {
        jobSystem.StopJobSystem();

        std::cerr << "[LYNX_FILE_BINARIZATION] " << exception.what() << '\n';
        return 1;
    }
}

namespace {
    constexpr const char* ASSETS_MANIFEST_NAME = "assets.meta_lynxmanifest";
    constexpr const char* BINARY_MANIFEST_NAME = "assets.lynxmanifest";

    const std::array<std::string, 5> MANIFEST_GROUPS = {
        LYNX_ASSET_MANIFEST_MODEL,
        LYNX_ASSET_MANIFEST_TEXTURES,
        LYNX_ASSET_MANIFEST_AUDIO,
        LYNX_ASSET_MANIFEST_MATERIALS,
        LYNX_ASSET_MANIFEST_SHADERS
    };

    file_route requirePathArgument(int& index, int argc, char* argv[], const std::string& argument) {
        if(index + 1 >= argc) {
            throw std::runtime_error(argument + "requiere una ruta");
        }

        ++index;
        return file_route(argv[index]);
    }

    CompileTargetOptions parseCompileTarget(const std::string& target) {
        if(target == "macOS")
            return CompileTargetOptions::macOS;
        if(target == "iOS")
            return CompileTargetOptions::iOS;
        if(target == "iOS-simulator")
            return CompileTargetOptions::iOS_simulator;
        if(target == "tvOS")
            return  CompileTargetOptions::tvOS;
        if(target == "tvOS-simulator")
            return CompileTargetOptions::tvOS_simulator;
        if(target == "windows")
            return CompileTargetOptions::Windows;
        if(target == "linux")
            return CompileTargetOptions::Linux;
        if(target == "android")
            return CompileTargetOptions::Android;

        throw std::runtime_error("Target de compilación desconocido: "+ target);
    }

    void validateDirectory(const file_route& path, const std::string& argument) {
        if(path.empty()) {
            throw std::runtime_error("Falta el argumento " + argument);
        }

        if(!std::filesystem::is_directory(path)) {
            throw std::runtime_error("La ruta recibida por " + argument + " no es un directorio válido: " + path.string());
        }
    }

    void validateRegularFile(const file_route& path, const std::string& argument){
        if(path.empty()) {
            throw std::runtime_error("Falta el argumento " + argument);
        }

        if(!std::filesystem::is_regular_file(path)) {
            throw std::runtime_error("La ruta recibida por " + argument + " no es una archivo válido: " + path.string());
        }
    }

    void initializaManifest(json& manifest) {
        for(const std::string& group : MANIFEST_GROUPS) {
            if(!manifest.contains(group) || !manifest[group].is_array()){
                manifest[group] = json::array();
            }
        }

        if(!manifest.contains(LYNX_ASSET_MANIFEST_TOTAL_OF_ELEMENTS)) {
            manifest[LYNX_ASSET_MANIFEST_TOTAL_OF_ELEMENTS] = 0;
        }
    }

    std::string makeAssetKey(const std::string& group, const json& asset) {
        return group + ":" + asset.value(LYNX_ASSET_MANIFEST_ASSET_PATH, std::string{});
    }

    std::unordered_set<std::string> collectAssetKeys(const json& manifest) {
        std::unordered_set<std::string> result;
        for(const std::string& group : MANIFEST_GROUPS) {
            if(!manifest.contains(group)) continue;

            for(const json& asset : manifest[group]) {
                result.insert(makeAssetKey(group, asset));
            }
        }

        return result;
    }
    
    file_route removeCategoryPrefix(const file_route& sourcePath, const std::string& category) {
        const file_route prefix(category);
        const file_route relative = sourcePath.lexically_relative(prefix);

        if(relative.empty()) return sourcePath.filename();

        const std::string relativeString = relative.generic_string();
        if(relativeString.starts_with("..")) return sourcePath;

        return relative;
    }

    file_route runtimePathForAsset(const std::string& group, const file_route& sourcePath) {
        file_route result = removeCategoryPrefix(sourcePath, group);

        if(group == LYNX_ASSET_MANIFEST_MODEL) {
            result.replace_extension(".lynxmesh");
        }else if(group == LYNX_ASSET_MANIFEST_TEXTURES) {
            result.replace_extension(".lynxtex");
        }else if(group == LYNX_ASSET_MANIFEST_SHADERS) {
            result.replace_extension(".lynxshader");
        }else if(group == LYNX_ASSET_MANIFEST_MATERIALS) {
            result.replace_extension(".lynxmat");
        }

        return result;
    }

    template<typename AssetMap>
    void writeAssetMap(std::ofstream& output, const AssetMap& assets) {
        for(const auto& [id, asset]: assets) {
            const std::string path = asset.assetPath.generic_string();
            const uint32_t pathSize = static_cast<uint32_t>(path.size());
            output.write(reinterpret_cast<const char*>(&id), sizeof(id));
            output.write(reinterpret_cast<const char*>(&pathSize), sizeof(pathSize));
            output.write(path.data(), pathSize);
        }
    }
}

AssetBuildOptions parseBuildOptions(int argc, char* argv[]) {
    AssetBuildOptions options;
    bool modeWasProvided = false;

    for(int index = 1; index < argc; index++) {
        const std::string argument = argv[index];

        if(argument == "--mode" || argument == "-m"){
            if(index + 1 >= argc) {
                throw std::runtime_error("--mode requiere engine o project");
            }

            const std::string mode = argv[++index];

            if(mode == "engine"){
                options.mode = AssetBuildMode::Engine;
            }else if(mode == "project"){
                options.mode = AssetBuildMode::Project;
            }else{
                throw std::runtime_error("Modo desconocido: " + mode);
            }

            modeWasProvided = true;
        }else if(argument == "--source-manifest"){
            options.sourceManifest = requirePathArgument(index, argc, argv, argument);
        }else if(argument == "--source-assets") {
            options.sourceAssets = requirePathArgument(index, argc, argv, argument);
        }else if(argument == "--predefined-assets") {
            options.predefinedAssets = requirePathArgument(index, argc, argv, argument);
        }else if(argument == "--predefined-manifest") {
            options.predefinedManifest = requirePathArgument(index, argc, argv, argument);
        }else if(argument == "--output") {
            options.output = requirePathArgument(index, argc, argv, argument);
        }else if(argument == "--compile-target" || argument == "-t") {
            if(index + 1 >= argc) {
                throw std::runtime_error("--compile-target requiere un valor");
            }
            actualCompileOption = parseCompileTarget(argv[++index]);
        }else {
            throw std::runtime_error("Argumento desconocido: " + argument);
        }
    }

    if(!modeWasProvided) {
        throw std::runtime_error("Debes indicar --mode engine o --mode project");
    }

    validateDirectory(options.sourceAssets, "--source-assets");
    //validateRegularFile(options.sourceManifest, "--source-manifest");
    if(options.sourceManifest.empty())
    {
        throw std::runtime_error(
            "Falta el argumento --source-manifest."
        );
    }

    if(options.output.empty())
    {
        throw std::runtime_error(
            "Falta el argumento --output."
        );
    }

    if(actualCompileOption ==
    CompileTargetOptions::Undefined)
    {
        throw std::runtime_error(
            "Falta --compile-target."
        );
    }

    if(options.mode == AssetBuildMode::Engine)
    {
        // Al compilar el motor, su manifiesto predefinido
        // sí tiene que existir.
        validateRegularFile(
            options.sourceManifest,
            "--source-manifest"
        );
    }
    else
    {
        // En un proyecto nuevo, assets.meta_lynxmanifest
        // todavía no existe. Solo comprobamos que pueda
        // crearse dentro de project_data/manifests.
        if(options.sourceManifest.has_filename() == false)
        {
            throw std::runtime_error(
                "--source-manifest debe indicar la ruta completa "
                "del manifiesto que se creará."
            );
        }

        validateDirectory(
            options.predefinedAssets,
            "--predefined-assets"
        );

        validateRegularFile(
            options.predefinedManifest,
            "--predefined-manifest"
        );
    }

    /*if(options.mode == AssetBuildMode::Project) {
        validateDirectory(options.predefinedAssets, "--predefined-assets");
        validateRegularFile(options.predefinedManifest, "--predefined-manifest");
    }*/

    return options;
}

json readJsonFile(const file_route& path) {
    std::ifstream input(path);

    if(!input.is_open()) {
        throw std::runtime_error("No se pudo abrir el JSON: " + path.string());
    }

    json result;
    try {
        input >> result;
    }catch(const json::parse_error& exception) {
        throw std::runtime_error("JSON inválido en " + path.string() + ": " + exception.what());
    }

    return result;
}

void writeJsonFile(const file_route& path, const json& data) {
    std::filesystem::create_directories(path.parent_path());

    std::ofstream output(path);
    if(!output.is_open()) {
        throw std::runtime_error("No se pudo escribir: " + path.string());
    }

    output << data.dump(4);
}

void copyDirectoryContents(const file_route& source, const file_route& destination) {
    if(!std::filesystem::exists(source)) {
        throw std::runtime_error("No existe la carpeta que debe copiarse: " + source.string());
    }

    std::filesystem::create_directories(destination);
    for(const auto& entry : std::filesystem::recursive_directory_iterator(source)) {
        const file_route relative = std::filesystem::relative(entry.path(), source);
        const file_route finalPath = destination / relative;

        if(entry.is_directory()) {
            std::filesystem::create_directories(finalPath);
            continue;
        }

        if(!entry.is_regular_file()) continue;

        std::filesystem::create_directories(finalPath.parent_path());

        std::filesystem::copy_file(entry.path(), finalPath, std::filesystem::copy_options::overwrite_existing);
    }
}

json mergeProjectManifest(const json& predefinedManifest, const json& currentProjectManifest) {
    json result = predefinedManifest;
    initializaManifest(result);

    json current = currentProjectManifest;
    initializaManifest(current);

    std::unordered_set<std::string> existingKeys = collectAssetKeys(result);
    std::unordered_set<uint32_t> existingIDs;

    uint32_t maximumID = result.value(LYNX_ASSET_MANIFEST_TOTAL_OF_ELEMENTS, 0u);

    for(const std::string& group: MANIFEST_GROUPS) {
        for(const json& asset: result[group]) {
            const uint32_t id = asset.value(LYNX_ASSET_MANIFEST_ASSET_ID, 0u);
            existingIDs.insert(id);
            maximumID = std::max(maximumID, id);
        }
    }

    for(const std::string& group : MANIFEST_GROUPS) {
        for(json asset : current[group]) {
            const std::string key = makeAssetKey(group, asset);

            if(existingKeys.contains(key)) continue;

            uint32_t id = asset.value(LYNX_ASSET_MANIFEST_ASSET_ID, 0u);
            if(id == 0 || existingIDs.contains(id)) {
                id = maximumID++;
                asset[LYNX_ASSET_MANIFEST_ASSET_ID] = id;
            }else {
                maximumID = std::max(maximumID, id);
            }

            existingIDs.insert(id);
            existingKeys.insert(key);
            result[group].push_back(std::move(asset));
        }
    }

    result[LYNX_ASSET_MANIFEST_TOTAL_OF_ELEMENTS] = maximumID;

    return result;
}

json createProjectOnlyManifest(const json& completeManifest, const json& predefinedManifest) {
    json result = completeManifest;
    initializaManifest(result);

    const std::unordered_set<std::string> predefinedKeys = collectAssetKeys(predefinedManifest);

    for(const std::string& group : MANIFEST_GROUPS) {
        json filtered = json::array();
        for(const json& asset: result[group]) {
            if(!predefinedKeys.contains(makeAssetKey(group, asset))) {
                filtered.push_back(asset);
            }
        }

        if(filtered.empty()) {
            result.erase(group);
        }else {
            result[group] = std::move(filtered);
        }
    }

    return result;
}

void writeRuntimeAssetManifest(const json& completeManifest, const file_route& binaryAssetsFolder) {
    using namespace lynx::resources;

    std::unordered_map<std::string, uint32_t> friendlyNameToID;
    std::unordered_map<uint32_t, ModelAsset> models;
    std::unordered_map<uint32_t, TextureAsset> textures;
    std::unordered_map<uint32_t, ShaderAsset> shaders;
    std::unordered_map<uint32_t, MaterialAsset> materials;

    const auto registerGroup = [&](const std::string& group) {
        if(!completeManifest.contains(group)) return;

        for(const json& asset : completeManifest[group]) {
            const uint32_t id = asset.at(LYNX_ASSET_MANIFEST_ASSET_ID);
            const std::string name = asset.at(LYNX_ASSET_MANIFEST_ASSET_NAME);
            const file_route sourcePath = asset.at(LYNX_ASSET_MANIFEST_ASSET_PATH).get<std::string>();
            const file_route runtimePath = runtimePathForAsset(group, sourcePath);
            const file_route physicalPath = binaryAssetsFolder / group / runtimePath;

            if(!std::filesystem::is_regular_file(physicalPath)) {
                throw std::runtime_error("El manifiesto declara un asset cuyo binario no existe: " + physicalPath.string());
            }

            friendlyNameToID[name] = id;

            if(group == LYNX_ASSET_MANIFEST_MODEL) {
                models.emplace(id, ModelAsset(id, runtimePath));
            }else if(group == LYNX_ASSET_MANIFEST_TEXTURES) {
                textures.emplace(id, TextureAsset(id, runtimePath));
            }else if(group == LYNX_ASSET_MANIFEST_SHADERS) {
                shaders.emplace(id, ShaderAsset(id, runtimePath));
            }else if(group == LYNX_ASSET_MANIFEST_MATERIALS) {
                materials.emplace(id, MaterialAsset(id, runtimePath));
            }
        }
    };

    registerGroup(LYNX_ASSET_MANIFEST_MODEL);
    registerGroup(LYNX_ASSET_MANIFEST_TEXTURES);
    registerGroup(LYNX_ASSET_MANIFEST_MATERIALS);
    registerGroup(LYNX_ASSET_MANIFEST_SHADERS);

    const file_route manifestPath = binaryAssetsFolder / "manifests" / BINARY_MANIFEST_NAME;
    std::filesystem::create_directories(manifestPath.parent_path());

    lynx::binary::headers::LynxManifestHeader header{};

    strncpy(
        header.magicNumber,
        "TSEFINAM_NELI",
        sizeof(header.magicNumber) - 1
    );

    header.version = 1;
    header.totalOfNamesToID = friendlyNameToID.size();
    header.totalOfModels = models.size();
    header.totalOfTextures = textures.size();
    header.totalOfShaders = shaders.size();
    header.totalOfMaterials = materials.size();

    std::ofstream output(manifestPath, std::ios::binary | std::ios::trunc);
    if(!output.is_open()) {
        throw std::runtime_error("No se pudo crear el manifiesto runtime: " + manifestPath.string());
    }

    output.write(reinterpret_cast<const char*>(&header), sizeof(lynx::binary::headers::LynxManifestHeader));

    for(const auto& [name, id]: friendlyNameToID) {
        const uint32_t nameSize = static_cast<uint32_t>(name.size());
        output.write(reinterpret_cast<const char*>(&id), sizeof(id));
        output.write(reinterpret_cast<const char*>(&nameSize), sizeof(nameSize));
        output.write(name.data(), nameSize);
    }

    writeAssetMap(output, models);
    writeAssetMap(output, textures);
    writeAssetMap(output, shaders);
    writeAssetMap(output, materials);
}

void buildEngineAssets(const AssetBuildOptions& options) {
    const file_route workRoot = options.output.parent_path() / (".LYNX_engine_assets_work_" + options.output.filename().string());
    std::filesystem::remove_all(workRoot);

    const file_route workAssets = workRoot / "project_data" / "assets";
    const file_route workManifest = workRoot / "project_data" / "manifests" / ASSETS_MANIFEST_NAME;

    copyDirectoryContents(options.sourceAssets, workAssets);
    writeJsonFile(workManifest, readJsonFile(options.sourceManifest));

    binarizeFiles(workRoot);

    std::filesystem::remove_all(options.output);
    copyDirectoryContents(workRoot / ".lynx_bin_data", options.output);
    
    std::filesystem::remove(options.output / "manifests" / BINARY_MANIFEST_NAME); //Los binarios del motor no incluyen manifiesto, solo el de proyecto debe generarlo
    std::error_code error;
    std::filesystem::remove(options.output / "manifests", error);
    
    std::filesystem::remove_all(workRoot);
}

void buildProjectAssets(const AssetBuildOptions& options) {
    const file_route workRoot = options.output.parent_path() / (".LYNX_project_assets_work_" + options.output.filename().string());
    std::filesystem::remove_all(workRoot);

    const file_route workAssets = workRoot / "project_data" / "assets";
    const file_route workManifet = workRoot / "project_data" / "manifests" / ASSETS_MANIFEST_NAME;

    copyDirectoryContents(options.sourceAssets, workAssets);

    json predefined = readJsonFile(options.predefinedManifest);
    initializaManifest(predefined);

    json currentProjectManifest = predefined;

    if(std::filesystem::is_regular_file(options.sourceManifest)) {
        currentProjectManifest = readJsonFile(options.sourceManifest);
    }

    json completeManifest = mergeProjectManifest(predefined, currentProjectManifest);

    writeJsonFile(workManifet, completeManifest);
    createAndOrganizeFiles(workRoot);

    completeManifest = readJsonFile(workManifet);

    writeJsonFile(options.sourceManifest, completeManifest);

    const json projectOnlyManifest = createProjectOnlyManifest(completeManifest, predefined);
    writeJsonFile(workManifet, projectOnlyManifest);

    binarizeFiles(workRoot);

    std::filesystem::remove_all(options.output);
    std::filesystem::create_directories(options.output);

    copyDirectoryContents(options.predefinedAssets, options.output);
    copyDirectoryContents(workRoot / ".lynx_bin_data", options.output);

    writeRuntimeAssetManifest(completeManifest, options.output);

    std::filesystem::remove_all(workRoot);
}

void createAndOrganizeFiles(std::filesystem::path lynxEngineFolder)
{
    std::string m_manifestFilename = "assets.meta_lynxmanifest";

    std::filesystem::path engineSourceFolder = lynxEngineFolder;
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

    auto manifestPath = manifestFolder / m_manifestFilename;

    std::ifstream file(manifestPath);
    json data;

    if(file.is_open()){
        file >> data;
    } else {
        LYNX_LOG_P(DEBUG_PREFIX, "No se pudo abrir el archivo del manifiesto, o no existe en la ruta: %s", manifestPath.string().c_str());
        //Creamos una nueva lista
        auto predefinedPath = manifestFolder / "assets_defaults.meta_lynxmanifest";
        std::ifstream predefined(predefinedPath);
        if(predefined.is_open())
        {
            try
            {
                data = json::parse(predefined);
            }catch (json::parse_error& e)
            {
                LYNX_WARNING_P(DEBUG_PREFIX, "Error at parsion predefined assets: %s", e.what());
                data = nullptr;
            }
        }else{

            data[LYNX_ASSET_MANIFEST_TOTAL_OF_ELEMENTS] = 0;
            data[LYNX_ASSET_MANIFEST_MODEL] = json::array(); //Inicializamos si estamos en Debug... No tiene sentido que se cree un nuevo indice en una build release
            data[LYNX_ASSET_MANIFEST_TEXTURES] = json::array();
            data[LYNX_ASSET_MANIFEST_MATERIALS] = json::array();
            data[LYNX_ASSET_MANIFEST_AUDIO] = json::array();
            data[LYNX_ASSET_MANIFEST_SHADERS] = json::array();
        }
    }

    LYNX_WARNING_P(DEBUG_PREFIX, "Indexando nuevos objetos en modo Debug");

    //Debug::Warning(nlohmann::to_string(manifestData));

    uint32_t maxID = data.value(LYNX_ASSET_MANIFEST_TOTAL_OF_ELEMENTS, 0);
    std::vector<std::string> existingPaths;
    bool folderReorganized = false;

    //Este vector son las extensiones que no deben de registrarse en el assets.json, ya que otras clases gestionaran su uso.
    std::vector<std::string> extensionCantRegistrate = {
        ".inl", ".metal", ".hlsl", ".glsl", ".spv", ".meta_lynxmanifest", ".mtl", ".msl", ".slang", ".meta_lynxtex", ".meta_lynxschema", ".meta_lynxscene", ".meta_lynxprefab"
    };
    std::string unsupportedFolder = "unsupported";

    //Obteniendo todas las rutas declaradas anteriormente en el manifiesto.
    for(const auto& assetType : LYNX_ASSET_MANIFEST_ASSETS_TYPES) {
        if(data.contains(assetType)){
            for(const auto& assetData : data[assetType]){
                existingPaths.push_back(assetData.value(LYNX_ASSET_MANIFEST_ASSET_PATH, ""));
            }
        }
    }

    //creamos un nuevo vector para almacenar todos las rutas de archivos encontrados en assets
    std::vector<std::filesystem::path> filesToIndex;
    for(const auto& entry: std::filesystem::recursive_directory_iterator(sourceAssetFolders.parent_path())){
        if(entry.is_regular_file() && entry.path().filename() != m_manifestFilename && entry.path().filename() != ".DS_Store"){
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
        //LYNX_LOG_P(DEBUG_PREFIX, "First folder, %s. Relative TO assets: %s. Comparacion %s", firstFolder.c_str(), relativeToAssets.c_str(), std::filesystem::relative(targetFolder, "assets").c_str());

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
            LYNX_LOG_P(DEBUG_PREFIX, "Se copio el archivo %s", currentPath.c_str());
            folderReorganized = true;
        }
    }

    if (folderReorganized == true)
    {
        createAndOrganizeFiles(lynxEngineFolder);
        return;
    }

    bool needSaveNewManifest = false;
    for(const auto& currentPath : filesToIndex){ //Si llego hasta aquí significa que la lista anterior ta joia.
        //LYNX_LOG_P(DEBUG_PREFIX, "Revisando nueva lista de archivos");

        std::filesystem::path relativeToAssets = std::filesystem::relative(currentPath, sourceAssetFolders);


        auto result = std::find(existingPaths.begin(), existingPaths.end(), relativeToAssets.generic_string());

        if(result != existingPaths.end()) continue; //Si existe el path en el manifiesto, saltar al próximo.

        std::string extension = relativeToAssets.extension().string();
        auto it = extensionToAssetType.find(extension);

        if(it == extensionToAssetType.end()) {
            continue;
        }

        std::string filteredTargetFolder = it->second.starts_with("assets") ? std::filesystem::relative(it->second, "assets").string() : it->second;

        //std::find(extensionCantRegistrate.begin(), extensionCantRegistrate.end(), it) == extensionCantRegistrate.end()

        //Guardar si la extensión existe y si esta en la carpeta correcta. Técnicamente no es necesario, pero no esta demás la comprobación, además de que esto solo esta en DEBUG. No añado las extensiones prohibidas al manifiesto.
        if(it != extensionToAssetType.end() && relativeToAssets.begin()->string() == filteredTargetFolder && std::find(extensionCantRegistrate.begin(), extensionCantRegistrate.end(), it->first) == extensionCantRegistrate.end()){
            needSaveNewManifest = true;
            LYNX_LOG_P(DEBUG_PREFIX, "Se necesita reescribir el manifiesto");


            uint32_t newID = ++maxID;
            std::string name = generateFriendlyName(currentPath.filename().string());

            json newAsset;
            newAsset[LYNX_ASSET_MANIFEST_ASSET_ID] = newID;
            newAsset[LYNX_ASSET_MANIFEST_ASSET_NAME] = name;
            newAsset[LYNX_ASSET_MANIFEST_ASSET_PATH] = relativeToAssets.generic_string();
            data[folderToManifestAssetGroup.find(filteredTargetFolder)->second].push_back(newAsset);
        }
    }

    //Escribir nuevo manifiesto con los nuevos datos si es necesario
    if(needSaveNewManifest){
        data[LYNX_ASSET_MANIFEST_TOTAL_OF_ELEMENTS] = maxID;

        //auto manifestPath = sourceAssetFolders / "manifests" / "assets.lynxmanifest";
        if(!std::filesystem::exists(manifestPath.parent_path()))
            std::filesystem::create_directories(manifestPath.parent_path());

        LYNX_LOG_P(DEBUG_PREFIX, "Path del manifiest %s", manifestPath.c_str());

        std::ofstream outFile(manifestPath);
        outFile << data.dump(2);
        outFile.close();

    }

    //assert(needSaveNewManifest == false && "Se escribió un nuevo manifiesto. Vuelve a iniciar la app para que ahora si funcione de forma correcta.");
    if (needSaveNewManifest)
    {
        createAndOrganizeFiles(lynxEngineFolder);
        return;
    }

    LYNX_WARNING_P(DEBUG_PREFIX, "Indexación de nuevos elementos completada. Generando binarios.");
}

//New version of generation and binarization ()
//(IA GENERATED CODE, not refactorized nor supervised creation) En este caso es un refuerzo del codigo mio, para soportar el nuevo sistema de carpetas más complejo. No debería dar problemas.
/*void createAndOrganizeFiles(std::filesystem::path lynxEngineFolder)
{
    static int recursionGuard = 0;
    recursionGuard++;
    if (recursionGuard > 5)
    {
        LYNX_ERROR_P(DEBUG_PREFIX, "Posible bucle infinito detectado en createAndOrganizeFiles");
        recursionGuard--;
        return;
    }

    std::string m_manifestFilename = "assets.meta_lynxmanifest";

    std::filesystem::path projectDataFolder = lynxEngineFolder / "project_data";
    std::filesystem::path assetsFolder      = projectDataFolder / "assets";
    std::filesystem::path manifestFolder    = projectDataFolder / "manifests";

    std::filesystem::create_directories(assetsFolder);
    std::filesystem::create_directories(manifestFolder);

    auto manifestPath = manifestFolder / m_manifestFilename;

    // Cargar manifiesto
    json data;
    std::ifstream file(manifestPath);
    if (file.is_open())
    {
        file >> data;
    }
    else
    {
        LYNX_LOG_P(DEBUG_PREFIX, "Manifiesto no encontrado, creando uno nuevo.");
        auto predefinedPath = manifestFolder / "assets_defaults.meta_lynxmanifest";
        std::ifstream predefined(predefinedPath);
        if (predefined.is_open())
            data = json::parse(predefined);
        else
        {
            data[LYNX_ASSET_MANIFEST_TOTAL_OF_ELEMENTS] = 0;
            data[LYNX_ASSET_MANIFEST_MODEL]     = json::array();
            data[LYNX_ASSET_MANIFEST_TEXTURES]  = json::array();
            data[LYNX_ASSET_MANIFEST_MATERIALS] = json::array();
            data[LYNX_ASSET_MANIFEST_AUDIO]     = json::array();
            data[LYNX_ASSET_MANIFEST_SHADERS]   = json::array();
        }
    }

    uint32_t maxID = data.value(LYNX_ASSET_MANIFEST_TOTAL_OF_ELEMENTS, 0);

    std::vector<std::string> existingAssetPaths;
    for (const auto& type : LYNX_ASSET_MANIFEST_ASSETS_TYPES)
    {
        if (data.contains(type))
            for (const auto& asset : data[type])
                existingAssetPaths.push_back(asset.value(LYNX_ASSET_MANIFEST_ASSET_PATH, ""));
    }

    const std::vector<std::string> extensionCantRegistrate = {
        ".metal", ".hlsl", ".glsl", ".spv", ".mtl", ".msl", ".slang",
        ".meta_lynxtex", ".meta_lynxshader", ".meta_lynxmat",
        ".meta_lynxmanifest", ".meta_lynxschema", ".meta_lynxscene"
    };

    const std::vector<std::string> ignoredFiles = { ".DS_Store", "Thumbs.db", "desktop.ini" };

    // Escanear archivos
    std::vector<std::filesystem::path> filesToIndex;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(projectDataFolder))
    {
        if (!entry.is_regular_file()) continue;

        std::string filename = entry.path().filename().string();
        if (filename == m_manifestFilename) continue;
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
            LYNX_LOG_P(DEBUG_PREFIX, "Movido: %s → %s", 
                       currentPath.string().c_str(), 
                       finalDestination.string().c_str());
        }
        catch (const std::exception& e)
        {
            LYNX_ERROR_P(DEBUG_PREFIX, "Error al mover %s: %s", 
                         currentPath.string().c_str(), e.what());
        }
    }

    if (folderReorganized)
    {
        LYNX_WARNING_P(DEBUG_PREFIX, "Estructura reorganizada. Re-ejecutando indexación...");
        recursionGuard--;
        createAndOrganizeFiles(lynxEngineFolder);
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
            newAsset[LYNX_ASSET_MANIFEST_ASSET_ID]   = newID;
            newAsset[LYNX_ASSET_MANIFEST_ASSET_NAME] = name;
            newAsset[LYNX_ASSET_MANIFEST_ASSET_PATH] = relStr;

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
        data[LYNX_ASSET_MANIFEST_TOTAL_OF_ELEMENTS] = maxID;

        std::ofstream outFile(manifestPath);
        outFile << data.dump(2);
        outFile.close();

        LYNX_WARNING_P(DEBUG_PREFIX, "Manifiesto actualizado con nuevos assets.");
        recursionGuard--;
        createAndOrganizeFiles(lynxEngineFolder);
        return;
    }

    recursionGuard--;
    LYNX_WARNING_P(DEBUG_PREFIX, "Indexación y organización de project_data completada correctamente.");
}*/

std::string generateFriendlyName(const std::string& filename){

    std::string extension = std::filesystem::path(filename).extension().string();
    std::string name = std::filesystem::path(filename).stem().string();

    return name + "-" + extensionToAssetType.find(extension)->second;
}



void binarizeFiles(std::filesystem::path lynxEngineFolder) {
    using namespace lynx::resources;
    std::string m_manifestFilename = "assets.meta_lynxmanifest";

    std::filesystem::path engineSourceFolder = lynxEngineFolder;
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

    auto manifestPath = sourceAssetFolders.parent_path() / "manifests" / m_manifestFilename;

    std::ifstream file(manifestPath);
    json manifestData;

    if(file.is_open()){
        file >> manifestData;
    } else {
        LYNX_EXECUTE_ERROR_P(DEBUG_PREFIX, "No se pudo abrir el archivo del manifiesto, o no existe en la ruta: %s", manifestPath.string().c_str());
    }

    LYNX_WARNING_P(DEBUG_PREFIX, "Start of binarization: %.4f", lynx::time::Time::getTotalTime());
    //Los archivos binarios que usará el motor van a estar en una carpeta llamada /.lynx-gen-assets

    //1- Leer datos del manifiesto para saber que archivos hay que hacer binario.
    uint32_t maxID = manifestData.value(LYNX_ASSET_MANIFEST_TOTAL_OF_ELEMENTS, 0);
    std::set<std::string> existingPaths;

    //Obteniendo todas las rutas declaradas anteriormente en el manifiesto.
    for(const auto& assetType : LYNX_ASSET_MANIFEST_ASSETS_TYPES) {
        if(manifestData.contains(assetType)){
            for(const auto& assetData : manifestData[assetType]){
                existingPaths.insert(assetData.value(LYNX_ASSET_MANIFEST_ASSET_PATH, ""));
            }
        }
    }

    //std::filesystem::path engineSourceFolder(LYNX_SOURCES_FOLDER); //Carpeta del engine con los assets en crudo.
    //std::filesystem::path sourceAssetFolders = engineSourceFolder / "assets";
    std::filesystem::path binaryDataFolder = engineSourceFolder / ".lynx_bin_data";
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
    if(manifestData.contains(LYNX_ASSET_MANIFEST_MODEL)){
        std::string targetFolder = "models";
        std::filesystem::path binaryModelFolder = binaryDataFolder / targetFolder;
        if(!std::filesystem::exists(binaryModelFolder)){
            std::filesystem::create_directories(binaryModelFolder);
        }

        auto data = manifestData[LYNX_ASSET_MANIFEST_MODEL];

        //std::vector<std::future<void>> futures;
        size_t total = data.size();
        size_t threads = jobSystem.AvailableThreads();
        size_t chunkSize = (total + threads - 1)/ threads;
        size_t numChunks = (total + chunkSize - 1) / chunkSize;

        LYNX_LOG_P(DEBUG_PREFIX, "Datos Utiles: total %i, hilos %i, tamaño del chunk: %i, numero de chunks: %i", total, threads, chunkSize, numChunks);

        lynx::job_system::JobGroup group;

        std::vector<std::unordered_map<std::string, uint32_t>> localFriendlyNameToID(numChunks);
        std::vector<std::unordered_map<uint32_t, ModelAsset>> localIDToModelAsset(numChunks);

        for(size_t i = 0; i < numChunks; i++){
            size_t start = i * chunkSize;
            size_t end = std::min(start + chunkSize, total);

            LYNX_LOG_P(DEBUG_PREFIX, "Trabajando chunk %i, que va del indice %i al %i", i, start, end);


            jobSystem.SubmitJob([=, &sourceAssetFolders, &engineSourceFolder, &binaryModelFolder, &idToModelAsset, &friendlyNameToID, &data, &localFriendlyNameToID, &localIDToModelAsset]() {
                for(size_t j = start; j < end; j++){
                    std::string filePath = data[j][LYNX_ASSET_MANIFEST_ASSET_PATH];

                    auto modelPath = sourceAssetFolders / filePath;
                    lynx::graphics::MeshData toBinarize = loadModelData(modelPath);
                    auto savePath = binaryModelFolder / std::filesystem::relative(engineSourceFolder / filePath, engineSourceFolder / targetFolder);
                    savePath.replace_extension(".lynxmesh");
                    //Header del binario
                    lynx::binary::headers::LynxMeshHeader header{};
                    strncpy(header.magicNumber, "HSEM_NELI", sizeof(header.magicNumber) - 1);
                    header.version = 1;
                    header.vertexCount = toBinarize.vertexBufferData.size();
                    header.indexCount = toBinarize.indexBufferData.size();

                    
                    if(!std::filesystem::exists(savePath.parent_path())){
                        std::error_code ec; //Estp es para evitar que multiples hilos creen la misma carpeta, y de un error catastrofico
                        std::filesystem::create_directories(savePath.parent_path(), ec);
                    }

                    std::ofstream outFile(savePath, std::ios::binary);
                    outFile.write(reinterpret_cast<const char*>(&header), sizeof(lynx::binary::headers::LynxMeshHeader));
                    outFile.write(reinterpret_cast<const char*>(toBinarize.vertexBufferData.data()), sizeof(lynx::render::buffers::VertexBufferData) * toBinarize.vertexBufferData.size());
                    outFile.write(reinterpret_cast<const char*>(toBinarize.indexBufferData.data()), sizeof(uint32_t) * toBinarize.indexBufferData.size());

                    outFile.close();

                    //std::lock_guard<std::mutex> lock(fileWrite);

                    uint32_t id = data[j][LYNX_ASSET_MANIFEST_ASSET_ID];
                    std::string name = data[j][LYNX_ASSET_MANIFEST_ASSET_NAME];
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

        /*for(const json& assetData : manifestData[LYNX_ASSET_MANIFEST_MODEL]){
            std::string filePath = assetData[LYNX_ASSET_MANIFEST_ASSET_PATH];
            futures.push_back(
                jobSystem.SubmitJob([=, &sourceAssetFolders, &engineSourceFolder, &binaryModelFolder, &idToModelAsset, &friendlyNameToID, &assetData]() {
                    auto modelPath = sourceAssetFolders / filePath;
                    lynx::graphics::MeshData toBinarize = loadModelData(modelPath);
                    auto savePath = binaryModelFolder / std::filesystem::relative(engineSourceFolder / filePath, engineSourceFolder / targetFolder);
                    savePath.replace_extension(".lynxmesh");
                    //Header del binario
                    lynx::binary::headers::lynxMeshHeader header{};
                    strncpy(header.magicNumber, "HSEM_NELI", sizeof(header.magicNumber) - 1);
                    header.version = 1;
                    header.vertexCount = toBinarize.vertexBufferData.size();
                    header.indexCount = toBinarize.indexBufferData.size();

                    if(!std::filesystem::exists(savePath.parent_path())){
                        std::filesystem::create_directories(savePath.parent_path());
                    }

                    std::ofstream outFile(savePath, std::ios::binary);
                    outFile.write(reinterpret_cast<const char*>(&header), sizeof(lynx::binary::headers::lynxMeshHeader));
                    outFile.write(reinterpret_cast<const char*>(toBinarize.vertexBufferData.data()), sizeof(lynx::render::buffers::VertexBufferData) * toBinarize.vertexBufferData.size());
                    outFile.write(reinterpret_cast<const char*>(toBinarize.indexBufferData.data()), sizeof(uint32_t) * toBinarize.indexBufferData.size());

                    outFile.close();

                    std::lock_guard<std::mutex> lock(fileWrite);

                    uint32_t id = assetData[LYNX_ASSET_MANIFEST_ASSET_ID];
                    std::string name = assetData[LYNX_ASSET_MANIFEST_ASSET_NAME];
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
    if(manifestData.contains(LYNX_ASSET_MANIFEST_SHADERS)){
        std::string targetFolder = "shaders";
        std::filesystem::path shaderBinaryFolder = binaryDataFolder / targetFolder;
        if (!std::filesystem::exists(shaderBinaryFolder))
        {
            std::filesystem::create_directories(shaderBinaryFolder);
        }

        for (const auto& shaderData: manifestData[LYNX_ASSET_MANIFEST_SHADERS])
        {

            std::string originalFilePath = shaderData[LYNX_ASSET_MANIFEST_ASSET_PATH];

            auto sourcePath = sourceAssetFolders / originalFilePath;
            auto savePath = binaryDataFolder / std::filesystem::relative(sourcePath, engineSourceFolder / "project_data/assets");
            //zsavePath.replace_extension(".b_lynxshader");

            auto path = std::filesystem::relative(savePath, binaryDataFolder);

            uint32_t id = shaderData[LYNX_ASSET_MANIFEST_ASSET_ID];
            std::string name = shaderData[LYNX_ASSET_MANIFEST_ASSET_NAME];

            std::ifstream shaderFile(sourcePath);
            json shader;

            if(shaderFile.is_open()){
                try
                {
                    shader = json::parse(shaderFile);
                }catch (json::parse_error& e)
                {
                    LYNX_ERROR_P(DEBUG_PREFIX, "Problemas al leer shader en ruta %s. Error: %s", sourcePath.c_str(), e.what());
                }
            }else{
                LYNX_ERROR_P(DEBUG_PREFIX, "Tratando de leer shader en la ruta %s y no se encontró, o logro abrir", sourcePath.c_str());
                //return ErrorShader; //Enviando shader predefinido de error
            }

            lynx::graphics::LynxShader generatedShader(shader[LYNX_SHADER_FILE_NAME], shader[LYNX_SHADER_DIRECTORY], shader[LYNX_SHADER_HUMAN_REDEABLE_NAME]);

            //auto generatedShader = lynx::graphics::lynxShaderUtils::generatelynxShaderFromAsset(ShaderAsset(id, path));

            //Header del archivo binario
            lynx::binary::headers::LynxShaderHeader header{};
            header.version = 1;
            strncpy(header.magicNumber, "REDAHS_NELI", sizeof(header.magicNumber) - 1);
            header.fileNameSize = generatedShader.shaderFileName.size();
            header.fileRouteSize = generatedShader.shaderRoute.string().size(); //Esta ruta es solo carpeta,  el nombre del archivo final se compone usando el filename y la extensión correspondiente a la API gráfica.
            header.shaderNameSize = generatedShader.shaderName.size();

            if(!std::filesystem::exists(savePath.parent_path()))
            {
                std::filesystem::create_directories(savePath.parent_path());
            }

            savePath.replace_extension(".lynxshader");

            std::ofstream outFile(savePath,std::ios::binary);
            outFile.write(reinterpret_cast<const char*>(&header),sizeof(lynx::binary::headers::LynxShaderHeader));
            outFile.write(generatedShader.shaderFileName.c_str(), header.fileNameSize);
            outFile.write(generatedShader.shaderRoute.c_str(), header.fileRouteSize);
            outFile.write(generatedShader.shaderName.c_str(), header.shaderNameSize);
            //outFile.write(reinterpret_cast<const char*>(&generatedShader), sizeof(lynx::graphics::lynxShader));

            friendlyNameToID[name] = id;
            idToShaderAsset.emplace(id, ShaderAsset(id, std::filesystem::relative(savePath,shaderBinaryFolder)));
        }
    }

    //Material loading
    if(manifestData.contains(LYNX_ASSET_MANIFEST_MATERIALS)){
        std::string targetFolder = "materials";
        std::filesystem::path materialBinaryFolder = binaryDataFolder / targetFolder;
        if (!std::filesystem::exists(materialBinaryFolder))
        {
            std::filesystem::create_directories(materialBinaryFolder);
        }

        for (const auto& materialData: manifestData[LYNX_ASSET_MANIFEST_MATERIALS])
        {

            std::string originalFilePath = materialData[LYNX_ASSET_MANIFEST_ASSET_PATH];

            auto sourcePath = sourceAssetFolders / originalFilePath;
            auto savePath = binaryDataFolder / std::filesystem::relative(sourcePath, engineSourceFolder / "project_data/assets");
            //zsavePath.replace_extension(".b_lynxshader");

            auto path = std::filesystem::relative(savePath, binaryDataFolder);

            uint32_t id = materialData[LYNX_ASSET_MANIFEST_ASSET_ID];
            std::string name = materialData[LYNX_ASSET_MANIFEST_ASSET_NAME];

            std::ifstream materialFile(sourcePath);
            json materialJson;

            if(materialFile.is_open()){
                try
                {
                    materialJson = json::parse(materialFile);
                }catch (json::parse_error& e)
                {
                    LYNX_ERROR_P(DEBUG_PREFIX, "Problemas al leer shader en ruta %s. Error: %s", sourcePath.c_str(), e.what());
                }
            }else
            {
                LYNX_ERROR_P(DEBUG_PREFIX, "Tratando de leer shader en la ruta %s y no se encontró, o logro abrir", sourcePath.c_str());
            }


            lynx::graphics::LynxMaterial material;
            material.materialName = materialJson.value(LYNX_MATERIAL_NAME, "Unnamed Material");
            material.vertexShaderID = materialJson.value(LYNX_MATERIAL_VERTEX_SHADER_ID, 0);
            material.fragmentShaderID = materialJson.value(LYNX_MATERIAL_FRAGMENT_SHADER_ID, 0);

            // Corregí el typo en las constantes para que coincidan con el archivo de cabecera
            material.vertexFunctionName = materialJson.value(LYNX_MATERIAL_VERTEX_SHADER_FUNCTION, "");
            material.fragmentFunctionName = materialJson.value(LYNX_MATERIAL_FRAGMENT_SHADER_FUNCTION, "");

            std::string renderQueueString = materialJson.value(LYNX_MATERIAL_RENDER_QUEUE, "Opaque");
            auto it = stringToCorrectRenderQueue.find(renderQueueString);
            if (it != stringToCorrectRenderQueue.end()) {
                material.renderQueue = it->second;
            }else {
                material.renderQueue = lynx::enumerations::LYNX_GPU_RENDER_QUEUE_TYPE::LYNX_GPU_RENDER_QUEUE_OPAQUE;
            }

            // Procesar todas las propiedades con la nueva lógica
            auto propData = materialJson.value(LYNX_MATERIAL_PROPERTIES_GROUP, json::array());
            processScalarProperties<float>(propData, LYNX_MATERIAL_FLOAT_PROPERTY, material.properties);
            processScalarProperties<uint32_t>(propData, LYNX_MATERIAL_INTEGER_PROPERTY, material.properties);
            processScalarProperties<uint32_t>(propData, LYNX_MATERIAL_TEXTURE_PROPERTY, material.properties); // Las texturas se guardan como ID (uint32_t)

            processVectorProperties<lynx::maths::vector::vec2>(propData, LYNX_MATERIAL_FLOAT2_PROPERTY, material.properties);
            processVectorProperties<lynx::maths::vector::vec3>(propData, LYNX_MATERIAL_FLOAT3_PROPERTY, material.properties);
            processVectorProperties<lynx::maths::vector::vec4>(propData, LYNX_MATERIAL_FLOAT4_PROPERTY, material.properties);
            processVectorProperties<lynx::graphics::Color>(propData, LYNX_MATERIAL_COLOR_PROPERTY, material.properties);

            //Save and binarize material file
            lynx::binary::headers::LynxMaterialHeader header{};
            header.version = 1;
            strncpy(header.magicNumber, "LAIRETAM_NELI", sizeof(header.magicNumber) - 1);
            header.materialNameSize = material.materialName.size();
            header.vertexFunctionSize = material.vertexFunctionName.size();
            header.fragmentFunctionSize = material.fragmentFunctionName.size();

            if(!std::filesystem::exists(savePath.parent_path()))
            {
                std::filesystem::create_directories(savePath.parent_path());
            }

            savePath.replace_extension(".lynxmat");


            //Orden de escritura!!!
            //Header -> Ints del material (incluye el RenderQueue) -> strings del material ->  Properties (TODO)
            std::ofstream outFile(savePath,std::ios::binary);
            outFile.write(reinterpret_cast<const char*>(&header),sizeof(lynx::binary::headers::LynxMaterialHeader));
            outFile.write(reinterpret_cast<const char*>(&material.vertexShaderID), sizeof(uint32_t));
            outFile.write(reinterpret_cast<const char*>(&material.fragmentShaderID), sizeof(uint32_t));
            outFile.write(reinterpret_cast<const char *>(&material.renderQueue), sizeof(LYNX_GPU_RENDER_QUEUE_TYPE));
            outFile.write(material.materialName.c_str(), header.materialNameSize);
            outFile.write(material.vertexFunctionName.c_str(), header.vertexFunctionSize);
            outFile.write(material.fragmentFunctionName.c_str(), header.fragmentFunctionSize);

            friendlyNameToID[name] = id;
            idToMaterialAsset.emplace(id, MaterialAsset(id, std::filesystem::relative(savePath, materialBinaryFolder)));
        }
    }

    //Guardando las texturas
    if(manifestData.contains(LYNX_ASSET_MANIFEST_TEXTURES)) {
        std::string targetFolder = "textures";
        std::filesystem::path binaryTextureFolder = binaryDataFolder / targetFolder;

        if(!std::filesystem::exists(binaryTextureFolder)) {
            std::filesystem::create_directories(binaryTextureFolder);
        }

        auto data = manifestData[LYNX_ASSET_MANIFEST_TEXTURES];
        size_t total = data.size();
        size_t threads = jobSystem.AvailableThreads();
        size_t chunkSize = (total + threads - 1) / threads;
        size_t numChunks = (total + chunkSize - 1) / chunkSize;

        LYNX_LOG_P(DEBUG_PREFIX, "Texturas - Total: %i, Chunks: %i", total, numChunks);

        lynx::job_system::JobGroup group;
        std::vector<std::unordered_map<std::string, uint32_t>> localFriendlyNameToID(numChunks);
        std::vector<std::unordered_map<uint32_t, TextureAsset>> localIDToTextureAsset(numChunks);

        for(size_t i = 0; i < numChunks; i++) {
            size_t start = i * chunkSize;
            size_t end = std::min(start + chunkSize, total);

            jobSystem.SubmitJob([=, &sourceAssetFolders, &engineSourceFolder, &binaryTextureFolder, &data, &localFriendlyNameToID, &localIDToTextureAsset]() {
                for(size_t j = start; j < end; j++){
                    std::string filePath = data[j][LYNX_ASSET_MANIFEST_ASSET_PATH];
                    auto texturePath = sourceAssetFolders / filePath;

                    // Calculamos la ruta de destino ANTES de hacer nada
                    auto metaPath = texturePath;
                    metaPath.replace_extension(".meta_lynxtex");

                    auto savePath = binaryTextureFolder / std::filesystem::relative(texturePath, engineSourceFolder / "project_data/assets/textures");
                    savePath.replace_extension(".lynxtex");

                    LYNX_LOG_P(DEBUG_PREFIX, "Path de la textura es: %s", texturePath.c_str());
                    LYNX_LOG_P(DEBUG_PREFIX, "Engine sources folder es: %s", engineSourceFolder.c_str());
  


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

                        LYNX_LOG_P(DEBUG_PREFIX, "Auto-generado .meta_lynxtex por defecto para: %s", filePath.c_str());
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
                        // para que el assets.b_lynxmanifest final sepa que existe.
                        uint32_t id = data[j][LYNX_ASSET_MANIFEST_ASSET_ID];
                        std::string name = data[j][LYNX_ASSET_MANIFEST_ASSET_NAME];
                        std::string path = std::filesystem::relative(savePath, binaryTextureFolder).string();

                        localFriendlyNameToID[i][name] = id;
                        localIDToTextureAsset[i].emplace(id, TextureAsset(id, path));
                        
                        LYNX_LOG_P(DEBUG_PREFIX, "Saltando textura ya cacheada: %s", name.c_str());
                        continue; // Pasar a la siguiente textura del bucle
                    }

                    int width, height, channels;
                    //Dato importante, stbi_load fuerza la carga en 4 canales, algo importante para la compresión de las texturas.
                    uint8_t* rawPixels = stbi_load(texturePath.string().c_str(), &width, &height, &channels, 4);

                    if(!rawPixels) {
                        LYNX_WARNING_P(DEBUG_PREFIX, "Fallo la carga de la textura cruda: %s", texturePath.string().c_str());
                        continue;
                    }

                    ApplyEdgeBleeding(rawPixels, width, height); //Filtrado de la textura :)
                    
                    //Parseo de los metadatos y calculo del atlas de la textura
                    std::vector<lynx::binary::headers::LynxAtlasSpriteInfo> atlasSprite;
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
                                lynx::binary::headers::LynxAtlasSpriteInfo spriteInfo{};
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


                    std::vector<lynx::binary::headers::TexturePayloadInfo> toc; //1 payload por cada textura usada
                    std::vector<std::vector<uint8_t>> compressedBlobs;

                    bool needsASTC = 
                    compressTexture && 
                    (
                        actualCompileOption == CompileTargetOptions::macOS || 
                        actualCompileOption == CompileTargetOptions::macOS_arm || 
                        actualCompileOption == CompileTargetOptions::iOS || 
                        actualCompileOption == CompileTargetOptions::tvOS || 
                        actualCompileOption == CompileTargetOptions::Android ||
                        actualCompileOption == CompileTargetOptions::iOS_simulator ||
                        actualCompileOption == CompileTargetOptions::tvOS_simulator
                    );

                    bool needsBC7 = compressTexture && 
                    (
                        actualCompileOption == CompileTargetOptions::Windows || 
                        actualCompileOption == CompileTargetOptions::Linux || 
                        actualCompileOption == CompileTargetOptions::macOS || 
                        actualCompileOption == CompileTargetOptions::macOS_x86_64 ||
                        actualCompileOption == CompileTargetOptions::iOS_simulator ||
                        actualCompileOption == CompileTargetOptions::tvOS_simulator
                    );
                
                    //Comprimir a ASTC
                    if(needsASTC){
                        auto blob = CompressToASTC(rawPixels, width, height);
                        compressedBlobs.push_back(blob);

                        lynx::binary::headers::TexturePayloadInfo info{};
                        info.format = lynx::enumerations::LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_ASTC_4X4_UNORM;
                        info.dataSize = blob.size();
                        toc.push_back(info);
                    }

                    //Comprimir usando BC7
                    if(needsBC7) {
                        auto blob = CompressToBC7(rawPixels, width, height);
                        compressedBlobs.push_back(blob);

                        lynx::binary::headers::TexturePayloadInfo info{};
                        info.format = lynx::enumerations::LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_BC7_UNORM;
                        info.dataSize = blob.size();
                        toc.push_back(info);
                    }

                    //Fallback por si es desconocida la plataforma
                    if(toc.empty()){
                        size_t byteSize = width * height * 4;
                        std::vector<uint8_t> blob(rawPixels, rawPixels + byteSize);
                        compressedBlobs.push_back(blob);

                        lynx::binary::headers::TexturePayloadInfo info{};
                        info.format = lynx::enumerations::LYNX_GPU_TEXTURE_PIXEL_FORMAT_TYPE::LYNX_GPU_TEXTURE_PIXEL_FORMAT_RGBA8_UNORM;
                        info.dataSize = blob.size();
                        toc.push_back(info);
                    }

                    stbi_image_free(rawPixels);

                    //Calculo de los offsets
                    uint64_t baseDataSize = sizeof(lynx::binary::headers::LynxTextureHeader) +
                        (sizeof(lynx::binary::headers::TexturePayloadInfo) * toc.size()) +
                        (sizeof(lynx::binary::headers::LynxAtlasSpriteInfo) * atlasSprite.size());
                    
                    uint64_t currentOffset = baseDataSize;

                    for(size_t k = 0; k < toc.size(); k++){
                        toc[k].offset = currentOffset;
                        currentOffset += toc[k].dataSize;
                    }

                    //Generar el archivo binario
                    lynx::binary::headers::LynxTextureHeader header{};
                    strncpy(header.magicNumber, "ERUTXET_NELI", sizeof(header.magicNumber) - 1);
                    header.version = 1;
                    header.width = width;
                    header.height = height;
                    header.channels = 4;
                    header.mipLevels = 1;
                    header.payloadCount = toc.size();
                    header.atlasSpriteCount = atlasSprite.size();

                    //auto savePath = binaryTextureFolder / std::filesystem::relative(engineSourceFolder / filePath, engineSourceFolder / targetFolder);
                    //savePath.replace_extension(".lynxtex");

                    if(!std::filesystem::exists(savePath.parent_path())){
                        std::error_code ec;
                        std::filesystem::create_directories(savePath.parent_path(), ec);
                    }

                    std::ofstream outFile(savePath, std::ios::binary);
                    //Escribiendo los datos
                    outFile.write(reinterpret_cast<const char*>(&header), sizeof(lynx::binary::headers::LynxTextureHeader));
                    outFile.write(reinterpret_cast<const char*>(toc.data()), sizeof(lynx::binary::headers::TexturePayloadInfo) * toc.size());
                    if(!atlasSprite.empty()) {
                        outFile.write(reinterpret_cast<const char*>(atlasSprite.data()), sizeof(lynx::binary::headers::LynxAtlasSpriteInfo) * atlasSprite.size());
                    }
                    for(const auto& blob: compressedBlobs){
                        outFile.write(reinterpret_cast<const char*>(blob.data()), blob.size());
                    }
                    outFile.close();

                    //Registrar en el manifiesto
                    uint32_t id = data[j][LYNX_ASSET_MANIFEST_ASSET_ID];
                    std::string name = data[j][LYNX_ASSET_MANIFEST_ASSET_NAME];

                    LYNX_LOG_P(DEBUG_PREFIX, "Save Path de la textura: %s", savePath.c_str());
                    LYNX_LOG_P(DEBUG_PREFIX, "Binary Data Folder: %s", binaryDataFolder.c_str());

                    std::string path = std::filesystem::relative(savePath, binaryDataFolder / "textures").string();
                    LYNX_LOG_P(DEBUG_PREFIX, "Path guardado es : %s", path.c_str());

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

        LYNX_LOG_P(DEBUG_PREFIX, "Texturas binarizadas de forma correcta");
    }

    //3 - Generar un manifiesto para los binarios.
    //No necesito un manifiesto en JSON, mejor volcar los mapas de memoria en un archivo, así no tenemos que volver a cargar y configurar estos, sino que sea solo una lectura.

    //4 - Guardar ese manifiesto
    std::filesystem::path manifiestPath = binaryDataFolder / "manifests" / "assets.lynxmanifest";
    if(!std::filesystem::exists(manifiestPath.parent_path())){
        std::filesystem::create_directories(manifiestPath.parent_path());
    }

    lynx::binary::headers::LynxManifestHeader manifestHeader{};
    strncpy(manifestHeader.magicNumber, "TSEFINAM_NELI", sizeof(manifestHeader.magicNumber) - 1);
    manifestHeader.version = 1;
    manifestHeader.totalOfNamesToID = friendlyNameToID.size();
    manifestHeader.totalOfModels = idToModelAsset.size();
    manifestHeader.totalOfTextures = idToTextureAsset.size();
    manifestHeader.totalOfShaders = idToShaderAsset.size();
    manifestHeader.totalOfMaterials = idToMaterialAsset.size();

    std::ofstream binaryFile(manifiestPath, std::ios::binary);
    binaryFile.write(reinterpret_cast<const char*>(&manifestHeader), sizeof(lynx::binary::headers::LynxManifestHeader));
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
        //auto shaderPath = shader.shaderRoute / shader.shaderName / ".b_lynxshader";
        std::string path = material.assetPath.string();
        uint32_t pathSize = path.length();

        binaryFile.write(reinterpret_cast<const char*>(&ID), sizeof(ID));
        binaryFile.write(reinterpret_cast<const char*>(&pathSize), sizeof(pathSize));
        binaryFile.write(path.c_str(),pathSize);
    }

    binaryFile.close();
    LYNX_WARNING_P(DEBUG_PREFIX, "End of binarization: %.4f", lynx::time::Time::getTotalTime());
}

lynx::graphics::MeshData loadModelData(std::filesystem::path modelPath){
        //std::filesystem::path meshFilePath = core::FileManager::fileRoute(meshPath, LYNX_FILE_TYPE_NONE);
        Assimp::Importer importer;

        const aiScene* scene = importer.ReadFile(modelPath, aiProcess_Triangulate | aiProcess_ConvertToLeftHanded | aiProcess_JoinIdenticalVertices);        
        if(!scene || !scene->HasMeshes()){
            LYNX_WARNING_P(DEBUG_PREFIX, "El objeto en la ruta %s no contenía ningún dato del tipo Mesh o ocurrió un error. Favor de revisar la información del archivo. Error: %s", modelPath.string().c_str(), importer.GetErrorString());
            return lynx::graphics::MeshData{};
        }

        //De momento soporte a un mesh por archivo. A futuro se debe de cambiar.
        aiMesh* mesh = scene->mMeshes[0];

        std::vector<lynx::render::buffers::VertexBufferData> vertexData;
        std::vector<uint32_t> indexData;

        //Obteniendo los vertices
        for(uint32_t i = 0; i < mesh->mNumVertices; i++){
            lynx::render::buffers::VertexBufferData vertex{};
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
                vertex.vertexColor = lynx::graphics::color::White;
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

        lynx::graphics::MeshData meshData{};
        meshData.indexBufferData = indexData;
        meshData.vertexBufferData = vertexData;

        return meshData;
    }


// Función auxiliar para procesar un objeto de propiedades escalares (float, int).
template<typename T>
void processScalarProperties(const nlohmann::json& jsonData, const std::string& key, std::unordered_map<std::string, lynx::graphics::MaterialProperty>& properties) {
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
void processVectorProperties(const nlohmann::json& jsonData, const std::string& key, std::unordered_map<std::string, lynx::graphics::MaterialProperty>& properties) {
    if (!jsonData.is_array()) {
        LYNX_ERROR_P(DEBUG_PREFIX, "Recibimos un objeto en vez de una lista de propiedades. Revisar el .lynxmat usado");
        return;
    }

    for (const auto& item: jsonData) {
        LYNX_LOG_P(DEBUG_PREFIX, "Objeto válido");
        if (item.contains(key)) {
            LYNX_LOG_P(DEBUG_PREFIX, "Contiene llave");
            const auto& property = item[key];

            if (property.contains(LYNX_MATERIAL_PROPERTY_NAME) && property.contains(LYNX_MATERIAL_PROPERTY_VALUE)) {
                // DEBUG TEMPORAL
                if (property.is_array()) {
                    LYNX_LOG_P(DEBUG_PREFIX, "ERROR INMINENTE: 'property' YA es un array: %s", property.dump().c_str());
                } else if (property.is_object()) {
                    LYNX_LOG_P(DEBUG_PREFIX, "INFO: 'property' es un objeto (correcto): %s", property.dump().c_str());
                } else {
                    LYNX_LOG_P(DEBUG_PREFIX, "INFO: 'property' es otro tipo: %s", property.dump().c_str());
                }
                LYNX_LOG_P(DEBUG_PREFIX, "Contiene elementos deseados");
                std::string propertyName = property[LYNX_MATERIAL_PROPERTY_NAME].get<std::string>();
                LYNX_LOG_P(DEBUG_PREFIX, "Leido el nombre %s", propertyName.c_str());
                VecType value;

                property[LYNX_MATERIAL_PROPERTY_VALUE].get_to(value);
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