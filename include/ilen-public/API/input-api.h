#pragma once

#include <ilen/input/input-buffer.h>
#include <ilen/core/engine-context.h>
#include <ilen/maths/vector.h>

using namespace ilen::maths::vector;

namespace ilen::engine::api {
    class InputAPI {
        public:
            static void Init(const ilen::EngineContext* context);
            static bool IsKeyDown(ilen::input::Key key); // Retorna un bool indicando si cierta tecla esta apretada en este momento
            static bool IsKeyPressed(ilen::input::Key key); //Retorna true si la tecla acaba de ser presionada
            static bool IsKeyReleased(ilen::input::Key key); //Retorna true si la techa fue liberada este frame.
            static bool IsPrimaryMouseButtonPressed();
            static bool IsSecundaryMouseButtonPressed();
            static vec2 GetMousePosition();
        private:
            static ilen::input::InputBuffer* inputBuffer;
            static ilen::input::Pointer* mouse;
        };
}