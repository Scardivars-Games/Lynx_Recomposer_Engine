#pragma once

#include <cstdint>

//Definición de parte del motor para una entrada de puntero, como el mouse, o el tactil de un movil.
namespace lynx::input {
    enum class PointerType {
        Unknow = 0,
        Mouse,
        Touch,
        Pen
    };

    struct Pointer {
        uint32_t id = 0; //0 es el mouse, y si el valor es diferentes, es el index del touch.
        PointerType type = PointerType::Unknow;
        float x = 0, y = 0;
        bool isDown = false; //Clock o contacto del dedo
        bool previousIsDown = false; //Para saber si se acabe de presionar o no
        float pressure = 1.0f; //Esto creo que le sacara más provecho una tableta digitalizadora
    };

    //Estado global de los punteros, accesible por el sistema
    struct PointerState {
        static constexpr int MAX_POINTERS = 10;
        Pointer pointers[MAX_POINTERS];

        //Atajos para acceder a los datos del mouse
        //Recordar que el click izquierdo forma parte del puntero 0.
        bool rightClick = false;
        bool middleClick = false;
        float scrollWheel = 0.0f;
    };
}