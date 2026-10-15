#include <lynx/core/filemanager.h>
#include <lynx/headers/lynx_logs.h>
#include <lynx/job_system/job_system.h>


using namespace lynx::core;

std::filesystem::path getFilePath(LYNX_FILE_TYPE category){
    std::string resourceSpecificFolder;
    switch (category)
    {
        case LYNX_FILE_TYPE_MODEL:
            resourceSpecificFolder = "models";
            break;
        case LYNX_FILE_TYPE_NATIVE_SHADER:
            resourceSpecificFolder = "shader_code";
            break;
        case LYNX_FILE_TYPE_TEXTURE:
            resourceSpecificFolder = "textures";
            break;
        case LYNX_FILE_TYPE_ASSETS_MANIFEST:
            resourceSpecificFolder = "manifests";
            break;
        case LYNX_FILE_TYPE_NONE:
            resourceSpecificFolder = "";
            break;
        case LYNX_FILE_TYPE_ENGINE_SHADER:
            resourceSpecificFolder = "shaders";
            break;
        case LYNX_FILE_TYPE_MATERIAL:
            resourceSpecificFolder = "materials";
            break;
        case LYNX_FILE_TYPE_SCENE:
            resourceSpecificFolder = "scenes";
            break;
        case LYNX_FILE_TYPE_PREFAB:
            resourceSpecificFolder = "prefabs";
            break;
        default:
            LYNX_EXECUTE_ERROR_P("FILE_MANAGER", "Dada una categoría desconocida");
            break;
    }

    return FileManager::getApplicationResourcesPath() / resourceSpecificFolder;
} 

/*
#if !defined (__APPLE__)
void FileManager::Init(){

    isInited = true;
}
#endif
*/

std::string FileManager::readFile(const std::string& filename, LYNX_FILE_TYPE category) {
    auto finalFileRoute = getFilePath(category) / filename;
    
    if(!std::filesystem::exists(finalFileRoute)){
        LYNX_EXECUTE_ERROR_P(DEBUG_PREFIX, "Archivo %s no existente en %s." ,filename.c_str(), finalFileRoute.c_str());
    } 

    std::ifstream archivo(finalFileRoute);
    if(!archivo.is_open()){
        LYNX_EXECUTE_ERROR_P(DEBUG_PREFIX, "No se logro abrir el archivo ubicado en %s.", finalFileRoute.c_str());
    }

    std::stringstream buffer;
    buffer << archivo.rdbuf();

    archivo.close();

    return buffer.str();
}

std::filesystem::path FileManager::fileRoute(const std::string& filename, LYNX_FILE_TYPE category){
    auto finalFileRoute = getFilePath(category) / filename;
    return finalFileRoute;
}