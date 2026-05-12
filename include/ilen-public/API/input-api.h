#pragma once

#include <ilen/input/input-buffer.h>
#include <ilen/core/engine-context.h>
#include <ilen/maths/vector.h>

using namespace ilen::maths::vector;

namespace ilen::engine::api {
    class InputAPI {
        public:
            static void Init(const ilen::EngineContext* context);

            //Teclado
            static bool IsKeyDown(ilen::input::Key key); // Retorna un bool indicando si cierta tecla esta apretada en este momento
            static bool IsKeyPressed(ilen::input::Key key); //Retorna true si la tecla acaba de ser presionada
            static bool IsKeyReleased(ilen::input::Key key); //Retorna true si la techa fue liberada este frame.

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
            static bool IsButtonDown(ilen::input::GamepadButton button, int index = 0); // Retorna un bool indicando si cierta tecla esta apretada en este momento
            static bool IsButtonPressed(ilen::input::GamepadButton button, int index = 0); //Retorna true si la tecla acaba de ser presionada
            static bool IsButtonReleased(ilen::input::GamepadButton button, int index = 0); //Retorna true si la techa fue liberada este frame.
            static vec2 GetGamepadAxis(ilen::input::GamepadAxis axis, int index = 0);
            static float GetGamepadValue(ilen::input::GamepadSpecialValues value, int index = 0);

        private:
            static ilen::input::InputBuffer* inputBuffer;
            static ilen::input::Pointer* mouse;
        };
}