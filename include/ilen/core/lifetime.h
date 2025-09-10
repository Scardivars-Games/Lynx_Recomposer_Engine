//PUNTO DE ENTRADA DEL MOTOR. ESTE ENCABEZADO CONTINE LA LóGICA DE TODO EL MOTOR. AQUI SE INICIALIZA DESDE EL ENTRY POINT DEL OS.
#pragma once
#include <ilen/headers/visual.h>
#include <ilen/headers/core.h>

namespace ilen {
    class AppRuntime {
        public:
            void init(application::ApplicationMetadata metadata); //Inicia el motor
            void update(); //Ejecuta el loop del motor
            void shutdown(); //Detiene el motor
            bool isRunning() const;
        private:
            const static inline std::string DEBUG_PREFIX = "AppRuntime";
            window::GameWindow* pGameWindow;
            render::Renderer* pGameRenderer;
    };
}