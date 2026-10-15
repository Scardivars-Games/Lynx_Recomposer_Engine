#if LYNX_LINUX
#include <lynx/core/filemanager.h>
#include <lynx/headers/lynx_logs.h>
#include <lynx/core/application.h>


using namespace lynx::core;

std::filesystem::path FileManager::s_applicationResourcesPath;
std::filesystem::path FileManager::s_applicationWrittablePath;

void FileManager::Init(){
    std::filesystem::path exe_path = std::filesystem::read_symlink("/proc/self/exe");
    auto exe_folder = exe_path.parent_path();
    s_applicationResourcesPath /= "Assets";

    //Create the folder to save data for the game
    const char* xdg_data_dir = std::getenv("XDG_DATA_HOME");
    if(xdg_data_dir != nullptr && xdg_data_dir[0] == '\0'){
        s_applicationWrittablePath = xdg_data_dir;
    }else{
        const char* home = std::getenv("HOME");
        if(home == nullptr || home[0] == '\0'){
            LYNX_FATAL_ERROR_P(DEBUG_PREFIX, "No se puedo obtener una ruta apropiada para guardar los datos del juego.");
        }

        s_applicationWrittablePath = home;
        s_applicationWrittablePath /= ".local/share";
    }

    auto appName = lynx::application::Application::getApplicationName();
    
    s_applicationWrittablePath /= appName;


    LYNX_LOG_P(DEBUG_PREFIX, "Resources path: %s.", s_applicationResourcesPath.string().c_str());
    LYNX_LOG_P(DEBUG_PREFIX, "Writtable path: %s.", s_applicationWrittablePath.string().c_str());

    isInited = true;
}

#endif