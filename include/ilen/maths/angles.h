//Para evitar usar ángulos, esta función abstrae tanto los degrees como los radianes
#pragma once

#include <cmath>

namespace ilen::maths::angles{
    //De momento solo se tiene degrees y radians, pero si se añaden más, se pueden usar estos enum para indicar a la hora de inicializar que tipo de ángulo es.
    enum ILEN_ANGLES_TYPE {
        ILEN_ANGLE_DEGREES,
        ILEN_ANGLE_RADIANS
    };


    //Clase con las cosas comunes necesarias para las estructuras de Degrees y Radians, así como cualquier otro ángulo a futurl
    class Angles {
        protected:
            double value;
            explicit Angles(double value) : value(value) {};
        public:
            double getValue() const { return value; }
            virtual double toDegrees() const = 0;
            virtual double toRadians() const = 0;
        
        private:
            Angles() = delete;
    };

    struct Degrees : public Angles {

        double toRadians() const override { return value * (M_PI / 180.0 ); }
        double toDegrees() const override { return value; } //No necesita conversión...

        //Constructores
        template<typename T>
        explicit Degrees(const T& value) : Angles(value) {};
        
        explicit Degrees(const Angles& otherAngle) : Angles(otherAngle.toDegrees()) {};
    };

    struct Radians : public Angles {
        double toRadians() const override { return value; } //No necesita conversión
        double toDegrees() const override { return value * (180.0 / M_PI ); }

        //Constructores
        template<typename T>
        explicit Radians(const T& value) : Angles(value) {};
        
        template<typename T>
        explicit Radians(const T& value, const ILEN_ANGLES_TYPE& type) : Angles(0.0) {
            if (type == ILEN_ANGLE_DEGREES) {
                this->value = static_cast<Degrees>(value).getValue() * (M_PI / 180.0);
            } else {
                this->value = static_cast<Degrees>(value).getValue();
            }
        };

        template<typename T>
        Radians operator*(T number) const {
            return Radians(value * number);
        }
        
        explicit Radians(const Angles& otherAngle) : Angles(otherAngle.toRadians()) {};
   };
}