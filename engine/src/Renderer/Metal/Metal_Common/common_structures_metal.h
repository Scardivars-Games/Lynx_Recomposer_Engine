#pragma once
#ifdef __APPLE__

#import <Metal/Metal.h>

//Estructuras comunes usadca para el manejo de datos dentro del renderer de Metal en el motor.
namespace lynx::render::utils::metal{
    struct MetalViewPredefinedValues
    {
        //Valores por defecto que tiene el Render de Metal (antes manejados por el MTKView) necesarios para que el motor funcione correctamente.
        MTLPixelFormat screenPixelFormat;
        MTLClearColor clearColor;
        MTLPixelFormat depthStencilPixelFormat;
    };
    
}

#endif