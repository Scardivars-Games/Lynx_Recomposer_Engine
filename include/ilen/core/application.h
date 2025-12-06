//Esta función controla datos esenciales de la aplicación como el framerate, los metadatos de esta (de donde Vulkan deberían obtener los datos)
//Las rutas de lectura, ubicación de la app, etc. También debería ser el centro de todos los eventos críticos durante el runtime. 
//NOTA: No es el punto de entrada de la app o motor, es solo para leer datos y configurar opciones básicas para el funcionamento de la app. Un nucleo para consultar datos durante runtime.

#pragma once
#include <string>
#include <ilen/core/bitflags.h>
#include <ilen/core/time.h>
#include <ilen/window/window.h>
#include <thread>
#include <memory>

#include "runtime_options.h"

namespace ilen::application{
    struct ApplicationMetadata {
        std::string applicationName = "";
        std::string applicationDeveloper = "";
        std::string applicationVersion = "";
        int applicationFramerate = 120;
        #if !TARGET_OS_IOS //Plataforma móviles no necesitan el tamaño de la ventana
        int startWidth = 0, startHeight = 0;
        IlenWindowPropertiesFlags windowPropertiesFlags = 0; //Fullscreen
        #endif
        #if TARGET_OS_IOS
        void* iOSUIWindow = nullptr;
        #endif

        bool isMetadataComplete() {
            bool platformSpecific = false;
            #if !TARGET_OS_IOS
            platformSpecific = startWidth != 0 && startHeight != 0;
            #endif

            #if TARGET_OS_IOS
            platformSpecific = iOSUIWindow != nullptr;
           #endif
            

            return applicationName != "" && applicationName != "" && applicationVersion != "" && platformSpecific;
        }
    };

    class Application final{
        public:
            static void setFrameRate(int newFramerate) {
                s_framerateInMiliseconds = 1.0f / newFramerate;
                ilen::core::options::GraphicsOptions::actualFPS = newFramerate;
            }
            static float getFrameRate() { return s_framerateInMiliseconds * 1000; }
            static std::string getApplicationName() { return s_applicationName; }
            static std::string getApplicationDeveloper() { return s_applicationDeveloper; }
            static std::string getApplicationVersion() { return s_applicationVersion; }
        static uint8_t getMajorVersion() { return s_majorVersion; }
        static uint8_t getMinorVersion() { return s_minorVersion; }
        static uint8_t getPatchVersion() { return s_patchVersion; }

            //Pausa la app hasta el siguiente frame
            static void stopUntilNextFrame() {
                const auto wakeTime = ilen::time::Time::getWakeTime(s_framerateInMiliseconds);
                std::chrono::duration<double, std::milli> margin(1);
                std::this_thread::sleep_until(wakeTime - margin);

                while(std::chrono::high_resolution_clock::now() < wakeTime) {
                    std::this_thread::yield(); //Dejamos el thread disponible para otras apps.
                } //Lopeamos la app, así gastamos una cantidad menor de recursos y obtenemos un framerate más perfecto
            }
            
            static void setApplicationData(ApplicationMetadata metadata){
                if (!metadata.isMetadataComplete()) {
                    throw std::invalid_argument("Se requiere que todos los metadatos estén completos para funcionar");
                }
                
                s_applicationName = metadata.applicationName;
                s_applicationVersion = metadata.applicationVersion;
                s_applicationDeveloper = metadata.applicationDeveloper;
                std::string stringSplit[3] = {"0", "0", "0"};

                int actualString = 0;
                int actualSpace = 0;
                std::string writtedData = "";
                while (actualString < s_applicationVersion.size()) {
                    if (s_applicationVersion != ".") {
                        writtedData += s_applicationVersion[actualString];
                    }else {
                        stringSplit[actualSpace] = writtedData;
                        writtedData = "";
                        actualSpace++;
                    }
                    actualString++;
                }

                s_majorVersion = std::stoi(stringSplit[0]);
                s_minorVersion = std::stoi(stringSplit[1]);
                s_patchVersion = std::stoi(stringSplit[2]);
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
    };
}
