#pragma once

#include <cstdint>

//Definición de parte del motor de todas las teclas físicas de un teclado. Esto es interno del motor, y no propio del OS.
namespace lynx::input {
    enum class Key: std::uint16_t {
        //Aquí registramos todas las teclas que nos interesa que el juego escuche.
        Unknown = 0,
        // Letras
        A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        // Números
        Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
        // Controles
        Space, Enter, Tab, Escape, Delete, Backspace,
        // Modificadores
        LeftShift, RightShift, LeftControl, RightControl, LeftOption, RightOption, LeftCommand, RightCommand,
        // Flechas
        Up, Down, Left, Right,
        // Límites
        MAX_KEYS
    };


    //Función de traducción (debe ser implementada según el OS)
    Key TranslateNativeKey(uint16_t nativeKeyCode);
}