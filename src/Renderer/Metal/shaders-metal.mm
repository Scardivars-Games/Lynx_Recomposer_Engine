//Esto solo es válido en las plataformas de Apple.
#if defined (__APPLE__)

#include <shaders-metal.h>
#import <Metal/Metal.h>

namespace  ilen::render {
    MetalShader::MetalShader(id<MTLLibrary> globalLibrary, ilen::graphics::IlenShader shaderRecipe) {
        shaderData = std::move(shaderRecipe);
        _shaderLibrary = globalLibrary;
        //Generar los nombres de las funciones del shader
        std::string vertNameString = this->shaderData.shaderFileName + "_vertex";
        std::string fragNameString = this->shaderData.shaderFileName + "_fragment";

        //Convertir a NSSTring
        NSString* vertName = [NSString stringWithUTF8String:vertNameString.c_str()];
        NSString* fragName = [NSString stringWithUTF8String:fragNameString.c_str()];

        //Obtener las funciones de la librería global
        auto vertexFunction = [globalLibrary newFunctionWithName:vertName];
        auto fragmentFunction = [globalLibrary newFunctionWithName:fragName];

        //Busqueda y reporte de errores
        if(!vertexFunction){
            ILEN_ERROR_P(DEBUG_PREFIX, "La vertex function %s no fue encontrada en la biblioteca", vertNameString.c_str());
            vertexFunction = nullptr;
        }
        if(!fragmentFunction){
            ILEN_ERROR_P(DEBUG_PREFIX, "La fragment function %s no fue encontrada en la biblioteca", fragNameString.c_str());
            fragmentFunction = nullptr;
        }
        
        _shaderFunctions.emplace(vertNameString, vertexFunction);
        _shaderFunctions.emplace(fragNameString, fragmentFunction);
    }

    const ilen::graphics::IlenShader& MetalShader::getShaderMetadata() const {
        return shaderData;
    }

    const void* MetalShader::getNativeShaderFromFunctionName(std::string functionName){
        auto it = _shaderFunctions.find(functionName);
        if (it != _shaderFunctions.end()) {
            return (__bridge const void*)it->second;
        }
        //Si no existe,se genera la libreria y añade
        auto newFunction = [_shaderLibrary newFunctionWithName:[NSString stringWithUTF8String:functionName.c_str()]];
        if(!newFunction){
            ILEN_ERROR_P(DEBUG_PREFIX, "La vertex function %s no fue encontrada en la biblioteca", functionName.c_str());
            newFunction = nullptr;
        }
        _shaderFunctions.emplace(functionName, newFunction);
        return (__bridge const void*)newFunction;
    }
}


#endif


