#include <ilen/core/filemanager.h>
#include <ilen/core/logs.h>


using namespace ilen::core;

std::filesystem::path getFilePath(FileCategory category){
    std::string resourceSpecificFolder;
    switch (category)
    {
    case ILEN_MODEL:
        resourceSpecificFolder = "models";
        break;
    case ILEN_SHADER:
        resourceSpecificFolder = "shaders";
        break;
    case ILEN_TEXTURE:
        resourceSpecificFolder = "textures";
        break;
    case ILEN_ASSETS_MANIFEST:
        resourceSpecificFolder = "manifests";
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


std::string FileManager::readFile(const std::string& filename, FileCategory category) {
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

std::filesystem::path FileManager::fileRoute(const std::string& filename, FileCategory category){
    auto finalFileRoute = getFilePath(category) / filename;
    return finalFileRoute;
}