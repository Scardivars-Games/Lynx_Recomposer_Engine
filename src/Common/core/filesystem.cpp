#include <ilen/core/filemanager.h>
#include <ilen/headers/ilen-logs.h>
#include <ilen/job-system/job-system.h>


using namespace ilen::core;

std::filesystem::path getFilePath(ILEN_FILE_TYPE category){
    std::string resourceSpecificFolder;
    switch (category)
    {
    case ILEN_FILE_TYPE_MODEL:
        resourceSpecificFolder = "models";
        break;
    case ILEN_FILE_TYPE_NATIVE_SHADER:
        resourceSpecificFolder = "shader-code";
        break;
    case ILEN_FILE_TYPE_TEXTURE:
        resourceSpecificFolder = "textures";
        break;
    case ILEN_FILE_TYPE_ASSETS_MANIFEST:
        resourceSpecificFolder = "manifests";
        break;
    case ILEN_FILE_TYPE_NONE:
        resourceSpecificFolder = "";
        break;
    case ILEN_FILE_TYPE_ENGINE_SHADER:
        resourceSpecificFolder = "shaders";
        break;
    default:
        ILEN_EXECUTE_ERROR_P("FILE_MANAGER", "Dada una categoría desconocida");
        break;
    }

    return FileManager::getApplicationResourcesPath() / resourceSpecificFolder;
} 

#if !defined (__APPLE__)
void FileManager::Init(){

    isInited = true;
}
#endif


std::string FileManager::readFile(const std::string& filename, ILEN_FILE_TYPE category) {
    auto finalFileRoute = getFilePath(category) / filename;
    
    if(!std::filesystem::exists(finalFileRoute)){
        ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "Archivo %s no existente en %s." ,filename.c_str(), finalFileRoute.c_str());
    } 

    std::ifstream archivo(finalFileRoute);
    if(!archivo.is_open()){
        ILEN_EXECUTE_ERROR_P(DEBUG_PREFIX, "No se logro abrir el archivo ubicado en %s.", finalFileRoute.c_str());
    }

    std::stringstream buffer;
    buffer << archivo.rdbuf();

    archivo.close();

    return buffer.str();
}

std::filesystem::path FileManager::fileRoute(const std::string& filename, ILEN_FILE_TYPE category){
    auto finalFileRoute = getFilePath(category) / filename;
    return finalFileRoute;
}