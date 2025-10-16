#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <vector>
#include <sstream>

bool executeCommand(const std::string& comand){
    int result = system(comand.c_str());
    if(result != 0){
        std::cerr << "Error al ejecutar el comando!!!" << result << std::endl;
        return false;
    }
    return true;
}

void copyOriginalFiles(std::vector<std::filesystem::path> metalFiles, std::filesystem::path shaderFolder, std::filesystem::path saveFolder){
    for(const auto& file: metalFiles){
        auto recursivePath = std::filesystem::relative(file, shaderFolder);
        auto saveFile = saveFolder / recursivePath;
        if(!std::filesystem::exists(saveFile.parent_path())){
            std::filesystem::create_directories(saveFile.parent_path());
        }
    }
}

void deleteAirFiles(std::vector<std::filesystem::path> airFiles){
    for(const auto& file: airFiles){
        std::filesystem::remove(file);
    }
}

int main(int argc, char* argv[]){
    std::filesystem::path folderPath;
    std::string sdk;

    for(int i = 0; i < argc; i++){
        std::string arg = argv[i];
        if(arg == "--ilen-folder" || arg == "-f"){
            if(i + 1 < argc){
                folderPath = argv[i + 1];
            } else {
                std::cerr << "Error: El indicador --ilen-folder requiere una ruta a una carpeta." << std::endl;
                return 1;
            }
        }else if (arg == "--sdk"){
            if(i + 1 < argc){
                sdk = argv[i + 1];
            }else{
                std::cerr << "Error: Hace falta indicar el sdk a usar para crear la libreria de metal" << std::endl;
                return 1;
            }
        }
    }

    //Normalmente buscaria un archvio .ilenproject (para saber que es la carpeta del proyecto y así poder hacer la busqueda). De momento se asume que es proyecto.
    std::filesystem::path sourceFolder = folderPath / "assets" / "shader-code";
    std::vector<std::filesystem::path> shaderFiles;

    std::filesystem::path buildFolder = folderPath / ".ilen-bin-data" / "shader-code";

    for(const auto& entry: std::filesystem::recursive_directory_iterator(sourceFolder)){
        if(entry.is_regular_file()){
            if(entry.path().extension() == ".metal"){
                shaderFiles.push_back(entry);
            }
        }
    }

    if(shaderFiles.empty()){
        std::cout << "No hay archivos .metal, no hay que procesar datos" << std::endl;
        copyOriginalFiles(shaderFiles, sourceFolder, buildFolder);
        return 1;
    }

    if(!std::filesystem::exists(buildFolder)){
        std::filesystem::create_directories(buildFolder);
    }

    std::vector<std::filesystem::path> airFiles;
    for(const auto& metalFile: shaderFiles){
        std::filesystem::path intermediateFile = buildFolder / metalFile.filename().replace_extension(".air");

        std::stringstream command;
        command << "xcrun -sdk " << sdk << " metal -c " << metalFile.string() << " -o " << intermediateFile.string();

        std::cout << command.str() << std::endl;

        if(!executeCommand(command.str())){
            return 1;
        }
        airFiles.push_back(intermediateFile);
    }

    if(airFiles.empty()){
        std::cerr << "ERROR: No se generaron los archivos intermedios. Favor de revisar" << std::endl;

        //Copiar los archivos fuentes como respaldo
        copyOriginalFiles(shaderFiles, sourceFolder, buildFolder);
        return 1;
    }

    //Creamos la .metallib
    std::stringstream linkCommand;
    std::filesystem::path metalLib = buildFolder / "ilen.metallib";

    linkCommand << "xcrun -sdk " << sdk << " metallib ";
    for(const auto& airFile: airFiles){
        linkCommand << airFile.string() << " ";
    }

    linkCommand << " -o " << metalLib;

    if(!executeCommand(linkCommand.str())){
        //Copiar los archivos fuentes como respaldo
        copyOriginalFiles(shaderFiles, sourceFolder, buildFolder);
        deleteAirFiles(airFiles);
        return 1;
    }

    deleteAirFiles(airFiles);

    return 0;
}