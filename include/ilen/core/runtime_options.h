// Aquí van todas las estructuras que almacenen configuraciones para el motor, como opciones de renderizado, musica,
// etc. Estos tienen valores por defecto, pero al inicializar el motor El juego puede cambiarlas. Los valores por
// defecto los asigna el motor.
#pragma once

#include <ilen/core/bitflags.h>

//Por razones de eficiencia en Memoria, es mejor mantener todos estos bools como bitflags
namespace ilen::core::options {
    struct GraphicsOptions {
        static inline IlenGraphicsOptionsFlags graphicsOptionsFlags = 0;
        static inline int actualFPS = 120;
        static inline int instanceBufferSizeInMB = 4; //4MB tamaño por defecto para los instance buffer.
        //A partir de aquí es codigo viejo. Estos bools ahora son un bitflag, que se configura al crear el render (es la responsibilidad del render iniciar las flags)
        /*static inline bool isPostProcessEnabled = true;
        static inline bool isStencilEnabled = false;
        static inline bool isDepthEnabled = false;*/
    };



    /*struct PostProcessOptions {
        static inline bool isMotionBlurEnabled = true;
        static inline bool isBloomEnabled = false;
    };*/
} // namespace ilen::core::options
