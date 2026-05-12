#pragma once

#include <cstdint>
#include <bitset>
#include <ilen/maths/vector.h>

namespace ilen::input {
    constexpr static int MAX_CONTROLLER = 4;

    enum class GamepadButton : uint8_t {
        South = 0, East, West, North, //Botones del lado derecho (Cross, Circle, Square, Triangle in PS, or A, B, X, Y in Xbox)
        LeftBumper, RightBumper, //Botones atras superiores (L1 y R1)
        DpadUp, DpadDown, DpadLeft, DpadRight, //Dpad
        Start, Select, //Botones espacial
        LeftTrigger, RightTrigger, //Triggers
        LeftStickClick, RightStickClick,
        LeftStick, RightStick,
        MAX_BUTTONS
    };

    enum class GamepadAxis {
        LeftStick,
        RightStick
    };

    enum class GamepadSpecialValues {
        LeftStickVertical,
        LeftStickHorizontal,
        RightStickVertical,
        RightStickHorizontal,
        LeftTrigger,
        RightTriger
    };

    struct GamepadState {
        bool connected = false;
        
        std::bitset<(size_t)GamepadButton::MAX_BUTTONS> buttons;
        std::bitset<(size_t)GamepadButton::MAX_BUTTONS> previousButtons;

        //Valores analógicos
        float leftStickX = 0.0f, leftStickY = 0.0f;
        float rightStickX = 0.0f, rightStickY = 0.0f;
        float leftTriggerValue = 0.0f;
        float rightTriggerValue = 0.0f;

        inline ilen::maths::vector::vec2 GetGamepadAxis(GamepadAxis axis) {
            if (axis == GamepadAxis::RightStick){
                return ilen::maths::vector::vec2(rightStickX, rightStickY);
            }else if (axis == GamepadAxis::LeftStick) {
                return ilen::maths::vector::vec2(leftStickX, leftStickY); 
            }

            return ilen::maths::vector::vec2{0.0f};
        } 

        inline float GetGamepadValue(GamepadSpecialValues values) {
            if  (values == GamepadSpecialValues::LeftStickHorizontal) {
                return leftStickX;
            } else if (values == GamepadSpecialValues::LeftStickVertical) {
                return leftStickY;
            } else if (values == GamepadSpecialValues::LeftTrigger){
                return leftTriggerValue;
            } else if (values == GamepadSpecialValues::RightStickHorizontal) {
                return rightStickX;
            } else if (values == GamepadSpecialValues::RightStickVertical) {
                return rightStickY;
            } else if (values == GamepadSpecialValues::RightTriger){
                return rightTriggerValue;
            }

            return 0.0f;
        }

        //inline bool IsButtonPressed(GamepadButton button) const { return buttons[(size_t)button]; }
    };
}