#pragma once
//Header con la definición de color que usará ilen-engine

namespace ilen::graphics {
    struct Color {
        union {
            float components[4]; //RGBA
            struct { float red, green, blue, alpha; };
            struct { float r, g, b, a; };
            struct { float x, y, z, w; };
        };
        
        Color() = default;
        Color(float red, float green, float blue, float alpha = 1.0f) : r(red), g(green), b(blue), a(alpha) { };

        //Métodos para los colores (Lerp, Mix, etc) TODO.
    };
}