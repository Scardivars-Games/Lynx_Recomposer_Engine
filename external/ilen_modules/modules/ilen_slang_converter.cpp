#include <sstream>
#include <iostream>
#include <filesystem>
#include <vector>

enum fileBackend {
    metal,
    spirv,
    hlsl,
    undefined
};

bool executeCommand(const std::string& comand){
    int result = system(comand.c_str());
    if(result != 0){
        std::cerr << "Error al ejecutar el comando!!!" << result << std::endl;
        return false;
    }
    return true;
}

int main(int argc, char* argv[]){
    std::filesystem::path folderPath;
    bool addDebug = false;
    fileBackend destinyFileType = undefined;

    std::filesystem::path executablePath = argv[0];
    std::filesystem::path slangcExecutable = executablePath.parent_path() / "slang" / "bin" / "slangc";

    for(int i = 0; i < argc; i++){
        std::string arg = argv[i];
        if(arg == "--ilen-folder" || arg == "-f"){
            if(i + 1 < argc){
                folderPath = argv[i + 1];
            } else {
                std::cerr << "Error: El indicador --ilen-folder requiere una ruta a una carpeta." << std::endl;
                return 1;
            }
        }else if (arg == "--convert-to" || arg == "-c"){
            if(i + 1 < argc){
                std::string destinyCode = argv[i + 1];
                std::cout << "Destiny conversion: " << destinyCode << std::endl;
                if(destinyCode == "metal"){
                    destinyFileType = metal;
                }else if(destinyCode == "spirv"){
                    destinyFileType = spirv;
                }else if(destinyCode == "hlsl"){
                    destinyFileType = hlsl;
                }
            }else{
                std::cerr << "Error: Hace falta indicar a que archivo hay que compilar (metal/spirv/hlsl)" << std::endl;
                return 1;
            }
        }else if (arg == "--add-debug" || arg == "-d") {
            addDebug = true;
        }
    }

    if(destinyFileType == undefined){
        std::cerr << "Error: No se definio un tipo de destino válido (metal/spirv/hlsl)" << std::endl;
    }


    //Normalmente buscaria un archvio .ilenproject (para saber que es la carpeta del proyecto y así poder hacer la busqueda). De momento se asume que es proyecto.
    std::filesystem::path sourceFolder = folderPath / "assets" / "shader-code";
    std::filesystem::path metalIncludes = folderPath / "include";
    std::vector<std::filesystem::path> shaderFiles;

    std::filesystem::path buildFolder = folderPath / ".ilen-bin-data" / "shader-code";

    for(const auto& entry: std::filesystem::recursive_directory_iterator(sourceFolder)){
        if(entry.is_regular_file()){
            if(entry.path().extension() == ".slang"){
                shaderFiles.push_back(entry);
            }
        }
    }

    if(shaderFiles.empty()){
        std::cout << "No hay archivos .slang, no hay que procesar datos" << std::endl;
        return 1;
    }

    if(!std::filesystem::exists(buildFolder)){
        std::filesystem::create_directories(buildFolder);
    }

    std::vector<std::filesystem::path> compiledFiles;
    for(const auto& shaderFile: shaderFiles){
        std::string shaderName = shaderFile.filename().replace_extension("");
        std::string finalExtension;
        std::string compilerTargetString;

        switch(destinyFileType){
            case metal: 
                finalExtension = ".metal";
                compilerTargetString = "metal";
                break;
            case spirv:
                finalExtension = ".spv";
                compilerTargetString = "spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name -entry " + shaderName + "_vertex -entry " + shaderName + "_fragment";
                break;
            case hlsl:
                finalExtension = ".hlsl";
                break;
            default:
                finalExtension = ".error";
                compilerTargetString = "";
                break;
        }
        std::filesystem::path convertedFile = buildFolder / shaderFile.filename().replace_extension(finalExtension);
        
        std::stringstream command;
        command << slangcExecutable.string() << " " << shaderFile.string() << " -target " << compilerTargetString << " -o " << convertedFile.string();

        std::cout << command.str() << std::endl;

        if(!executeCommand(command.str())){
            return 1;
        }
    }
}