//Para evitar usar ángulos, esta función abstrae tanto los degrees como los radianes
#pragma once

#include <cmath>
#include <ilen/headers/enumerations.h>

namespace ilen::maths::angles{
    using namespace ilen::enumerations;
    //De momento solo se tiene degrees y radians, pero si se añaden más, se pueden usar estos enum para indicar a la hora de inicializar que tipo de ángulo es.
    //Movido a headers/enumerations.h

    //NEW API
    struct Radians;

    struct Degrees {
        double value;

        //Constructores
        Degrees() = default;
        template<typename T>
        explicit Degrees(const T& val) : value(static_cast<double>(val)) {}

        inline double toDegrees() const {return value; }
        inline Radians toRadians() const; //Implementar luego de los constructores de Radianes.

        constexpr explicit operator double() const { return value; }
    };

    struct Radians {
        double value;

        Radians() = default;
        template<typename T>
        explicit Radians(const T& val) : value(static_cast<double>(val)) {}
        
        //Inicializar desde otro tipo de ángulo
        template<typename T>
        explicit Radians(const T& val, const ILEN_ANGLES_TYPE& type) {
            double numericValue = static_cast<double>(val); 
            
            if (type == ILEN_ANGLE_TYPE_DEGREES) {
                this->value = numericValue * (M_PI / 180.0);
            } else {
                this->value = numericValue;
            }
        }

        inline double toRadians() const { return value;} 
        inline Degrees toDegrees() const {
            return Degrees(value * (180.0 / M_PI));
        }

        template<typename T>
        Radians operator*(T number) const{
            return Radians(value * number);
        }

        template<typename T>
        Radians operator/(T number) const {
            return Radians(value / number);
        }

        constexpr explicit operator double() const { return value; }
    };

    inline Radians Degrees::toRadians() const {
        return Radians(value * (M_PI / 180.0));
    }


    //Clase con las cosas comunes necesarias para las estructuras de Degrees y Radians, así como cualquier otro ángulo a futuro
    //OLD API (se mantiene por razones de referencia, borrar cuando ya no cumpla función como consulta)
    /*class Angles {
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
            if (type == ILEN_ANGLE_TYPE_DEGREES) {
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
   };*/
}