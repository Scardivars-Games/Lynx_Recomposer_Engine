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

    class Application{
        public:
            static void setFrameRate(int newFramerate) { s_framerateInMiliseconds = 1.0f / newFramerate; }
            static float getFrameRate() { return s_framerateInMiliseconds * 1000; }
            static const std::string getApplicationName() { return s_applicationName; }
            static const std::string getApplicationDeveloper() { return s_applicationDeveloper; }
            static const std::string getApplicationVersion() { return s_applicationVersion; }

            //Pausa la app hasta el siguiente frame
            static void stopUntilNextFrame() {
                auto wakeTime = ilen::time::Time::getWakeTime(s_framerateInMiliseconds);
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
                setFrameRate(metadata.applicationFramerate);
            }
        private:
            inline static std::string s_applicationName;
            inline static std::string s_applicationDeveloper;
            inline static std::string s_applicationVersion;
            inline static float s_framerateInMiliseconds = 1.0f / 120; //120 FPS por defecto
    };
}