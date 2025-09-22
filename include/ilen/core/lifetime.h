//PUNTO DE ENTRADA DEL MOTOR. ESTE ENCABEZADO CONTINE LA LóGICA DE TODO EL MOTOR. AQUI SE INICIALIZA DESDE EL ENTRY POINT DEL OS.
#pragma once
#include <ilen/headers/visual.h>
#include <ilen/headers/core.h>
#include <memory>

namespace ilen {
    class AppRuntime {
        public:
            static AppRuntime& getInstance();
            void init(application::ApplicationMetadata metadata); //Inicia el motor
            void update(); //Ejecuta el loop del motor
            void shutdown(); //Detiene el motor
            bool isRunning();

            render::Renderer& getGameRenderer() {
                return *pGameRenderer;
            } 
        private:
            AppRuntime() = default;
            const static inline std::string DEBUG_PREFIX = "AppRuntime";
            std::unique_ptr<window::GameWindow> pGameWindow;
            std::unique_ptr<render::Renderer> pGameRenderer;
    };
}