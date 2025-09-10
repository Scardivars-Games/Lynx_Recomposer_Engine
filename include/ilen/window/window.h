#pragma once
#include <ilen/core/application.h>


namespace ilen::window{
    //Ilen solo soportará una ventana, no es un gestor de estas, esta clase genera una abstracción de alto nivel para las ventanas, comunicándose a bajo nivel con la API de ventanas del sistema.
    //Windows - WinAPI
    //macOS - Cocoa (Quartz Composer)
    //Linux - X11 (Wayland en el futuro)
    //iOS y Android, el sistema en específico de estas.
    class GameWindow{
        public:
            //Getters
            static int getWidth() {return s_width; }
            static int getHeight() { return s_height; }

            //Constructores
            GameWindow(void* metadata); //ES UN PUNTERO A ilen::application::ApplicationMetadata

            void processEvents();
            bool shouldClose();

            void* getWindow();
            void* getSurface(); //View en Apple ecosystem.

            class WindowNative; //Esta clase es la que enlazará esta clase con la implementación del OS.
        private:
            static int s_width, s_height;
            static WindowNative* s_pNative;
    };

    enum IlenWindowPropertiesBitFlags{
        ILEN_WINDOW_FULLSCREEN = 0, //Si una pantalla esta en fullscreen, no necesita ninguna opción extra.
        ILEN_WINDOW_RESIZABLE = 1 << 0,
        ILEN_WINDOW_CLOSABLE = 1 << 1,
        ILEN_WINDOW_MAXIMIZABLE = 1 << 2,
        ILEN_WINDOW_BORDERLESS = 1 << 3,
        ILEN_WINDOW_WINDOWED = 0 << 3,
    };
}