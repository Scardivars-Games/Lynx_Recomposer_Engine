//Esta función controla datos esenciales de la aplicación como el framerate, los metadatos de esta (de donde Vulkan deberían obtener los datos)
//Las rutas de lectura, ubicación de la app, etc. También debería ser el centro de todos los eventos críticos durante el runtime. 
//NOTA: No es el punto de entrada de la app o motor, es solo para leer datos y configurar opciones básicas para el funcionamento de la app. Un nucleo para consultar datos durante runtime.

#pragma once
#include <cstddef>
#include <string>
#include <lynx/core/bitflags.h>
#include <lynx/core/time.h>
#include <lynx/window/window.h>
#include <thread>
#include <memory>
#include <lynx/headers/lynx_logs.h>
#include <lynx/core/engine_context.h>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <immintrin.h>     
#endif

namespace lynx::application{
    struct ApplicationMetadata {
        std::string applicationName = "";
        std::string applicationDeveloper = "";
        std::string applicationVersion = "";
        int applicationFramerate = 5;
        #if LYNX_DESKTOP //Plataforma móviles no necesitan el tamaño de la ventana
        int startWidth = 0, startHeight = 0;
        LynxWindowPropertiesFlags windowPropertiesFlags = 0; //Fullscreen
        #endif
        #if LYNX_MOBILE
        void* mobileNativeWindow = nullptr;
        #endif

        bool isMetadataComplete() {
            bool platformSpecific = false;
            #if LYNX_DESKTOP
            platformSpecific = startWidth != 0 && startHeight != 0;
            #endif

            #if LYNX_MOBILE
            platformSpecific = mobileNativeWindow != nullptr;
           #endif
            

            return !applicationName.empty() && !applicationDeveloper.empty() && !applicationVersion.empty() && applicationFramerate > 0 && platformSpecific;
        }
    };

    class Application final{
        public:
            static void setFrameRate(int newFramerate) {
                s_framerateInMiliseconds = 1.0f / newFramerate;
                s_context->runtimeConfiguration->graphics.actualFPS = newFramerate;
            }
            static float getFrameRate() { return s_framerateInMiliseconds * 1000; }
            static std::string getApplicationName() { return s_applicationName; }
            static std::string getApplicationDeveloper() { return s_applicationDeveloper; }
            static std::string getApplicationVersion() { return s_applicationVersion; }
            static uint8_t getMajorVersion() { return s_majorVersion; }
            static uint8_t getMinorVersion() { return s_minorVersion; }
            static uint8_t getPatchVersion() { return s_patchVersion; }
            static EngineContext* getEngineContext() {return s_context; }

            //Pausa la app hasta el siguiente frame
            static void stopUntilNextFrame() {
                //Algunos macros para usar caracteristicas de la CPU para ahorrar energia sin cederle el hilo al procesador.
                #if defined(__aarch64__) || defined(_M_ARM64)
                    #define CPU_RELAX() __asm__ volatile("yield" ::: "memory") //Apple/ARM
                #elif defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
                    #define CPU_RELAX() _mm_pause(); //Intel/AMD
                #else
                    #define CPU_RELAX(); //Fallback
                #endif

                const auto wakeTime = lynx::time::Time::getWakeTime(s_framerateInMiliseconds);
                std::chrono::duration<double, std::milli> margin(2.5);

                //Dormir la CPU hasta justo antes de ser necesaria.
                if(std::chrono::high_resolution_clock::now() < (wakeTime - margin)){
                    std::this_thread::sleep_until(wakeTime - margin);
                }
            

                while(std::chrono::high_resolution_clock::now() < wakeTime) {
                    //std::this_thread::yield(); //Dejamos el thread disponible para otras apps.
                    CPU_RELAX(); //Mantenemos al hilo, pero sin cederlo al sistema
                } //Lopeamos la app, así gastamos una cantidad menor de recursos y obtenemos un framerate más perfecto
            }
            
            static void setApplicationData(ApplicationMetadata metadata, EngineContext* context){
                if (!metadata.isMetadataComplete()) {
                    throw std::invalid_argument("Se requiere que todos los metadatos estén completos para funcionar");
                }

                s_context = context;
                
                s_applicationName = metadata.applicationName;
                s_applicationVersion = metadata.applicationVersion;
                s_applicationDeveloper = metadata.applicationDeveloper;
                unsigned int major = 0;
                unsigned int minor = 0;
                unsigned int patch = 0;

                char firstSeparator = '\0';
                char secondSeparator = '\0';

                std::istringstream versionParser(
                    s_applicationVersion
                );

                if(
                    !(versionParser
                        >> major
                        >> firstSeparator
                        >> minor
                        >> secondSeparator
                        >> patch) ||
                    firstSeparator != '.' ||
                    secondSeparator != '.' ||
                    major > std::numeric_limits<std::uint8_t>::max() ||
                    minor > std::numeric_limits<std::uint8_t>::max() ||
                    patch > std::numeric_limits<std::uint8_t>::max()
                ) {
                    throw std::invalid_argument(
                        "La versión debe usar el formato major.minor.patch"
                    );
                }

                s_majorVersion = static_cast<std::uint8_t>(major);
                s_minorVersion = static_cast<std::uint8_t>(minor);
                s_patchVersion = static_cast<std::uint8_t>(patch);
                LYNX_LOG_P("APPLICATION_CONFIG", "Actual framerate: %i", metadata.applicationFramerate);
                setFrameRate(metadata.applicationFramerate);
            }
        private:
            inline static std::string s_applicationName;
            inline static std::string s_applicationDeveloper;
            inline static std::string s_applicationVersion;
            inline static uint8_t s_majorVersion = 0;
            inline static uint8_t s_minorVersion = 0;
            inline static uint8_t s_patchVersion = 0;
            inline static float s_framerateInMiliseconds = 1.0f / 120; //120 FPS por defecto
            inline static EngineContext* s_context = nullptr;
    };
}
