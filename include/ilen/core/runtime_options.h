// Aquí van todas las estructuras que almacenen configuraciones para el motor, como opciones de renderizado, musica,
// etc. Estos tienen valores por defecto, pero al inicializar el motor El juego puede cambiarlas. Los valores por
// defecto los asigna el motor.
#pragma once

#include <ilen/core/bitflags.h>

//Por razones de eficiencia en Memoria, es mejor mantener todos estos bools como bitflags
namespace ilen::core::options {
    struct GraphicsOptions {
        static inline IlenGraphicsOptionsFlags graphicsOptionsFlags = 0;
        static inline int actualFPS = 60;
        static constexpr int instanceBufferSizeInMB = 4; //4MB tamaño por defecto para los instance buffer.
        static constexpr int generalBufferSizeInMB = 64; //Los buffers de vertices, indices, etc, se crearan usando bloques de 64MB por defecto.
        //A partir de aquí es codigo viejo. Estos bools ahora son un bitflag, que se configura al crear el render (es la responsibilidad del render iniciar las flags)
        /*static inline bool isPostProcessEnabled = true;
        static inline bool isStencilEnabled = false;
        static inline bool isDepthEnabled = false;*/
    };

    //Opciones generales del motor
    struct PhysicsOptions {
        static constexpr double PhysicsUpdateTime = 60.0; //Se actualizan las físicas 60 veces por segundo. 
        static inline float gravity = -9.81f; //Velocidad de la gravedad
    };

    struct PredefinedAssetsID {
        //ID de datos predefinidos
        //Error
        static constexpr inline uint32_t ErrorShaderID = 7;
        static constexpr inline uint32_t ErrorMaterialID = 4;
        static constexpr inline uint32_t ErrorTextureID = 12;

        //Necesarios por otros sistemas
        static constexpr inline uint32_t QuadMeshID = 2;
        static constexpr inline uint32_t PostProcessPresentShaderID = 9;
        static constexpr inline uint32_t PostProcessCalculateShaderID = 10;
        static constexpr inline uint32_t DefaultTextureID = 11;
        static constexpr inline uint32_t DefaultUnlitOpaqueMatID = 5;
        static constexpr inline uint32_t DefaultUnlitTransparentMatID = 6;
    };



    /*struct PostProcessOptions {
        static inline bool isMotionBlurEnabled = true;
        static inline bool isBloomEnabled = false;
    };*/
} // namespace ilen::core::options
