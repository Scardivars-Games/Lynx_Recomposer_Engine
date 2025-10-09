#include <ilen/graphics/shader.h>

using namespace ilen::core;

const std::string ILEN_SHADER_FILE_NAME = "shader-file-name";
const std::string ILEN_SHADER_DIRECTORY = "shader-dir";
const std::string ILEN_SHADER_HUMAN_REDEABLE_NAME = "shader-name";

namespace ilen::graphics {
    IlenShader IlenShaderUtils::generateIlenShaderFromAsset(const ilen::resources::ShaderAsset& asset){
        using json = nlohmann::json;
        auto shaderPath = FileManager::fileRoute(asset.assetPath, ILEN_FILE_TYPE_NONE);

        std::ifstream file(shaderPath);
        json data;

        if(file.is_open()){
            file >> data;
        }else{
            ILEN_ERROR_P(DEBUG_PREFIX, "Tratando de leer shader en la ruta %s y no se encontró, o logro abrir", asset.assetPath.c_str());
            return ErrorShader; //Enviando shader predefinido de error
        }

        IlenShader fileToArchive(data[ILEN_SHADER_FILE_NAME], data[ILEN_SHADER_DIRECTORY], data[ILEN_SHADER_HUMAN_REDEABLE_NAME]);
        return fileToArchive;
    }
}