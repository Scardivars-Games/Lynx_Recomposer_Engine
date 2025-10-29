//Esto solo es válido en las plataformas de Apple.
#if defined (__APPLE__)

#include <shaders-metal.h>
#import <Metal/Metal.h>

namespace  ilen::render {
    MetalShader::MetalShader(id<MTLLibrary> globalLibrary, ilen::graphics::IlenShader shaderRecipe) {
        @autoreleasepool {
            shaderData = std::move(shaderRecipe);
            _shaderLibrary = [globalLibrary retain];
            //Generar los nombres de las funciones del shader
            std::string vertNameString = this->shaderData.shaderFileName + "_vertex";
            std::string fragNameString = this->shaderData.shaderFileName + "_fragment";

            //Convertir a NSSTring
            NSString* vertName = [NSString stringWithUTF8String:vertNameString.c_str()];
            NSString* fragName = [NSString stringWithUTF8String:fragNameString.c_str()];

            //Obtener las funciones de la librería global
            auto vertexFunction = [globalLibrary newFunctionWithName:vertName];
            auto fragmentFunction = [globalLibrary newFunctionWithName:fragName];
            
            if(vertexFunction){
                MTLFunctionType type = [vertexFunction functionType];
                if(type == MTLFunctionTypeVertex){
                    ILEN_LOG_P(DEBUG_PREFIX, "Éxito: %s si es una función vertex", [vertName UTF8String]);
                }else{
                    ILEN_LOG_P(DEBUG_PREFIX, "ERROR!!! %s no es una función vertex, es %i", [vertName UTF8String], (uint32_t)type);
                }
            }
            
            //Busqueda y reporte de errores
            if(!vertexFunction){
                ILEN_ERROR_P(DEBUG_PREFIX, "La vertex function %s no fue encontrada en la biblioteca", vertNameString.c_str());
                vertexFunction = nullptr;
            }
            if(!fragmentFunction){
                ILEN_ERROR_P(DEBUG_PREFIX, "La fragment function %s no fue encontrada en la biblioteca", fragNameString.c_str());
                fragmentFunction = nullptr;
            }
            
            _shaderFunctions.emplace(vertNameString, (__bridge void*)vertexFunction);
            _shaderFunctions.emplace(fragNameString, (__bridge void*)fragmentFunction);
        }
    }

    MetalShader::~MetalShader() {
        _shaderLibrary = nil;
        
        for(auto pair: _shaderFunctions){
            if(pair.second){
                [(__bridge id<MTLFunction>)pair.second release];
            }
        }
        _shaderFunctions.clear();
    }

    const ilen::graphics::IlenShader& MetalShader::getShaderMetadata() const {
        return shaderData;
    }

    const void* MetalShader::getNativeShaderFromFunctionName(std::string functionName){
        auto it = _shaderFunctions.find(functionName);
        if (it != _shaderFunctions.end()) {
            return it->second;
        }
        //Si no existe,se genera la libreria y añade
        auto newFunction = [_shaderLibrary newFunctionWithName:[NSString stringWithUTF8String:functionName.c_str()]];
        if(!newFunction){
            ILEN_ERROR_P(DEBUG_PREFIX, "La funcion %s no fue encontrada en la biblioteca de Metal", functionName.c_str());
        }
        
        void* newFunctionPtr = (__bridge void*)newFunction;
        _shaderFunctions.emplace(functionName, newFunctionPtr);
        return newFunctionPtr;
    }
}


#endif


