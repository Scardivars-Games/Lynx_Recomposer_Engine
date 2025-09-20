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
}