//Esto solo es válido en las plataformas de Apple.
#if defined (__APPLE__)

#include <Metal/shaders_metal.h>
#import <Metal/Metal.h>

namespace  lynx::render {
    MetalShader::MetalShader(id<MTLLibrary> globalLibrary, lynx::graphics::LynxShader shaderRecipe) {
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
            
            /*if(vertexFunction){
                MTLFunctionType type = [vertexFunction functionType];
                if(type == MTLFunctionTypeVertex){
                    LYNX_LOG_P(DEBUG_PREFIX, "Éxito: %s si es una función vertex", [vertName UTF8String]);
                }else{
                    LYNX_LOG_P(DEBUG_PREFIX, "ERROR!!! %s no es una función vertex, es %i", [vertName UTF8String], (uint32_t)type);
                }
            }*/
            
            //Busqueda y reporte de errores
            if(!vertexFunction){
                LYNX_ERROR_P(DEBUG_PREFIX, "La vertex function %s no fue encontrada en la biblioteca", vertNameString.c_str());
                vertexFunction = nullptr;
            }
            if(!fragmentFunction){
                LYNX_ERROR_P(DEBUG_PREFIX, "La fragment function %s no fue encontrada en la biblioteca", fragNameString.c_str());
                fragmentFunction = nullptr;
            }
            
            _shaderFunctions.emplace(vertNameString, (__bridge void*)vertexFunction);
            _shaderFunctions.emplace(fragNameString, (__bridge void*)fragmentFunction);
        }
    }

    MetalShader::~MetalShader() {
        [_shaderLibrary release];
        _shaderLibrary = nil;
        
        for(auto pair: _shaderFunctions){
            if(pair.second){
                [(__bridge id<MTLFunction>)pair.second release];
            }
        }
        _shaderFunctions.clear();
    }

    const lynx::graphics::LynxShader& MetalShader::getShaderMetadata() const {
        return shaderData;
    }

    const void* MetalShader::getNativeShaderFromFunctionName(std::string functionName){
        @autoreleasepool {
            auto it = _shaderFunctions.find(functionName);
            if (it != _shaderFunctions.end()) {
                return it->second;
            }
            //Si no existe,se genera la libreria y añade
            auto newFunction = [_shaderLibrary newFunctionWithName:[NSString stringWithUTF8String:functionName.c_str()]];
            if(!newFunction){
                LYNX_ERROR_P(DEBUG_PREFIX, "La funcion %s no fue encontrada en la biblioteca de Metal", functionName.c_str());
            }
            
            void* newFunctionPtr = (__bridge void*)newFunction;
            _shaderFunctions.emplace(functionName, newFunctionPtr);
            return newFunctionPtr;
        }
    }

const void* MetalShader::getNativeShaderFromFunctionNameAndUberActivations(std::string functionName, std::vector<LynxConstantValuesForPipeline> activations){
    @autoreleasepool {
        std::string finalName = functionName;
        for(const auto& activation: activations){
            finalName += " " + std::to_string(activation.index) + ":" + std::to_string(activation.actualValue);
        }
        
        auto it = _shaderFunctions.find(finalName);
        if (it != _shaderFunctions.end()) {
            return it->second;
        }
        //Si no existe,se genera la libreria y añade
        MTLFunctionConstantValues* constantValues = [MTLFunctionConstantValues new];
        for(const auto& activation: activations){
            [constantValues setConstantValue:&activation.actualValue type:MTLDataTypeBool atIndex:activation.index];
        }
        NSError* error = nil;
        
        auto newFunction = [_shaderLibrary newFunctionWithName:[NSString stringWithUTF8String:functionName.c_str()] constantValues:constantValues error:&error];
        if(!newFunction){
            LYNX_ERROR_P(DEBUG_PREFIX, "La funcion %s no fue encontrada en la biblioteca de Metal. Error: %s", functionName.c_str(), error);
        }
        
        void* newFunctionPtr = (__bridge void*)newFunction;
        _shaderFunctions.emplace(finalName, newFunctionPtr);
        return newFunctionPtr;
    }
}
}


#endif


