#pragma once
//Header con la definición de color que usará ilen-engine
#include <ilen/maths/vector.h>

namespace ilen::graphics {
    using namespace ilen::maths::vector;

    struct Color {
        union {
            float components[4]; //RGBA
            struct { float red, green, blue, alpha; };
            struct { float r, g, b, a; };
            struct { float x, y, z, w; };
        };
        
        Color() = default;
        Color(float red, float green, float blue, float alpha = 1.0f) : r(red), g(green), b(blue), a(alpha) { };
        Color(float4 vecData) : r(vecData.r), g(vecData.g), b(vecData.b), a(vecData.a) {};
        Color& operator=(const float4& data) {
            this->red = data.r;
            this->blue = data.b;
            this->green = data.g;
            this->alpha = data.a;

            return *this;
        }
        //Métodos para los colores (Lerp, Mix, etc) TODO.
    };

    namespace color {
        const Color White(1,1,1,1);
        const Color Black(0,0,0,1);
        const Color Blue(0,0,1,1);
        const Color Red(1,0,0,1);
        const Color Green(0,1,0,1);
        const Color Clear(0,0,0,0);
    }
}