#include <ilen/core/filemanager.h>
#include <ilen/core/logs.h>
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
    default:
        Debug::ExecuteError("Dada una categoría desconocida", "FileManager");
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
        Debug::ExecuteError("Archivo " + filename + " no existente en " + finalFileRoute.string(), DEBUG_PREFIX);
    } 

    std::ifstream archivo(finalFileRoute);
    if(!archivo.is_open()){
        Debug::ExecuteError("No se logro abrir el archivo ubicado en " + finalFileRoute.string(), DEBUG_PREFIX);
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