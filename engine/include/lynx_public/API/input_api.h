#pragma once

#include <lynx/input/input_buffer.h>
#include <lynx/core/engine_context.h>
#include <lynx/maths/vector.h>

using namespace lynx::maths::vector;

namespace lynx::engine::api {
    class InputAPI {
        public:
            static void Init(const lynx::EngineContext* context);

            //Teclado
            static bool IsKeyDown(lynx::input::Key key); // Retorna un bool indicando si cierta tecla esta apretada en este momento
            static bool IsKeyPressed(lynx::input::Key key); //Retorna true si la tecla acaba de ser presionada
            static bool IsKeyReleased(lynx::input::Key key); //Retorna true si la techa fue liberada este frame.

            //Mouse
            static bool IsPrimaryMouseButtonPressed();
            static bool IsSecundaryMouseButtonPressed();
            static vec2 GetMousePosition();

            //Entrada touch
            static int GetTotalTouches();
            static bool IsTouchDown(int index); //Indica si el touch en ese indice esta abajo
            static bool IsTouchNew(int index); //Indica si ese touch es nuevo este fram
            static bool IsTouchLeave(int index); //Indica se ese touch justo acaba de irse ese frame
            static vec2 GetTouchPosition(int index);

            //Entrada de Mando
            static bool IsButtonDown(lynx::input::GamepadButton button, int index = 0); // Retorna un bool indicando si cierta tecla esta apretada en este momento
            static bool IsButtonPressed(lynx::input::GamepadButton button, int index = 0); //Retorna true si la tecla acaba de ser presionada
            static bool IsButtonReleased(lynx::input::GamepadButton button, int index = 0); //Retorna true si la techa fue liberada este frame.
            static vec2 GetGamepadAxis(lynx::input::GamepadAxis axis, int index = 0);
            static float GetGamepadValue(lynx::input::GamepadSpecialValues value, int index = 0);

        private:
            static lynx::input::InputBuffer* inputBuffer;
            static lynx::input::Pointer* mouse;
        };
}