#include <ilen/graphics/shader.h>

using namespace ilen::core;

const std::string ILEN_SHADER_FILE_NAME = "shader-file-name";
const std::string ILEN_SHADER_DIRECTORY = "shader-dir";
const std::string ILEN_SHADER_HUMAN_REDEABLE_NAME = "shader-name";

namespace ilen::graphics {
    IlenShader IlenShaderUtils::generateIlenShaderFromAsset(const ilen::core::ShaderAsset& asset){
        using json = nlohmann::json;
        auto shaderPath = FileManager::fileRoute(asset.assetPath, ILEN_FILE_TYPE_NONE);

        std::ifstream file(shaderPath);
        json data;

        if(file.is_open()){
            file >> data;
        }else{
            Debug::Error("Tratando de leer shader en la ruta " + asset.assetPath.string() + " y no se encontró, o logro abrir", "SHADER_LOADING");
            return ErrorShader; //Enviando shader predefinido de error
        }

        IlenShader fileToArchive(data[ILEN_SHADER_FILE_NAME], data[ILEN_SHADER_DIRECTORY], data[ILEN_SHADER_HUMAN_REDEABLE_NAME]);
        return fileToArchive;
    }
}