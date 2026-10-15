#pragma once
#include <lynx/core/application.h>

namespace lynx::application
{
    struct ApplicationMetadata;
}

namespace lynx::window{
    //Ilen solo soportará una ventana, no es un gestor de estas, esta clase genera una abstracción de alto nivel para las ventanas, comunicándose a bajo nivel con la API de ventanas del sistema.
    //Windows - WinAPI
    //macOS - Cocoa (Quartz Composer)
    //Linux - X11 (Wayland en el futuro)
    //iOS y Android, el sistema en específico de estas.

    class WindowNative
    {
        public:
            virtual ~WindowNative() = default;
            virtual void pollEvents() = 0;
            virtual void updateGamepads() = 0;
            virtual bool shouldClose() = 0;
            virtual bool shouldPause() = 0; //Esto es para indicar que la app debe pausar su ejecución.
            virtual void* getNativeWindow() const = 0;
            virtual const void* getNativeView() const = 0;
            virtual bool checkWindowResize(uint32_t &outWidth, uint32_t &outHeight) = 0;
    };

    class IWindow{
        public:
            //Getters
            static uint32_t getWidth() {return s_width; }
            static uint32_t getHeight() { return s_height; }

            //Constructores
            //GameWindow(void* metadata); //ES UN PUNTERO A lynx::application::ApplicationMetadata
            //class WindowNative; //Esta clase es la que enlazará esta clase con la implementación del OS.
            //Up is old engine architecture.

            void processEvents(); 
            void updateGamepads();
            bool shouldClose();
            bool shouldPause();

            void* getWindow();
            const void* getSurface(); //View en Apple ecosystem.
            void* getNativeWindow();

            static std::unique_ptr<IWindow> CreateWindow(const lynx::application::ApplicationMetadata& metadata);

            //bool checkWindowResize(uint32_t &outWidth, uint32_t &outHeight);

            IWindow(std::unique_ptr<WindowNative> nativeWindow);
            ~IWindow();
        protected:
            std::unique_ptr<WindowNative> p_windowNative;
            static uint32_t s_width, s_height;
            //static WindowNative* s_pNative;
    };
}
