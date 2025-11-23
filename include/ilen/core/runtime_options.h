// Aquí van todas las estructuras que almacenen configuraciones para el motor, como opciones de renderizado, musica,
// etc. Estos tienen valores por defecto, pero al inicializar el motor El juego puede cambiarlas. Los valores por
// defecto los asigna el motor.
#pragma once

namespace ilen::core::options {
    struct RenderingOptions {
        static inline bool isPostProcessEnabled = false;
        static inline bool isStencilEnabled = false;
        static inline bool isDepthEnabled = false;
        static inline int actualFPS = 120;
    };

    struct PostProcessOptions {
        static inline bool isMotionBlurEnabled = true;
        static inline bool isBloomEnabled = false;
    };
} // namespace ilen::core::options
