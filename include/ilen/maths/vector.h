//Matemática vectorial para motores de videojuegos
//Especificacion: En este caso se van a tratar las estructuras como operaciones aritméticas básicas, luego esto hay que ampliarlo para dar soporte a las instrucciones SIMD ampliadas de cada arquitectura. 
#pragma once

#include <iostream>
#include <cstddef>
#include <sstream>
#include <cmath>

#ifdef __clang__
//Espero que los tipos nativos en macOS mantengan su tamaño de memoria de 32 o 64 bits.
using float32_t = float;
#elif __GNUC__
#include <stdfloat>
using float32_t = std::float32_t;
#endif

namespace ilen::maths::vector{
    template<typename T, size_t N>
    struct alignas(16) Vector{
        T data[N];
    };

    template<typename T>
    struct alignas(16) Vector <T, 2>{
        union{
            T data[2];
            struct {T x, y; };
            struct {T r, g; };
        };

        //Constructor
        Vector() = default;
        explicit Vector(T scalar) : x(scalar), y(scalar) {};
        explicit Vector(T x, T y) : x(x), y(y) {};

        T& operator[](size_t index) { return data[index]; }
        const T& operator[](size_t index) const { return data[index]; }
        
        bool operator==(const Vector<T, 2>& other) const {
            return x == other.x && y == other.y;
        }
    };

    //Vector3
    template<typename T>
    struct alignas(16) Vector <T, 3>{
        union{
            T data[3];
            struct {T x, y, z; };
            struct {T r, g, b; };
        };

        //Constructor
        Vector() = default;
        explicit Vector(T scalar) : x(scalar), y(scalar), z(scalar) {};
        explicit Vector(T x, T y, T z) : x(x), y(y), z(z) {};
        explicit Vector(const Vector<T, 2>& vec2, T z = 0) : x(vec2.x), y(vec2.y), z(z) {};

        static Vector<T, 3> up(){
            return Vector<T, 3>(T(0), T(1), T(0));
        } 
        static Vector<T, 3> down(){
            return Vector<T, 3>(T(0), T(-1), T(0));
        } 
        static Vector<T, 3> right(){
            return Vector<T, 3>(T(1), T(0), T(0));
        } 
        static Vector<T, 3> left(){
            return Vector<T, 3>(T(-1), T(0), T(0));
        } 
        static Vector<T, 3> forward(){
            return Vector<T, 3>(T(0), T(0), T(1));
        } 
        static Vector<T, 3> backward(){
            return Vector<T, 3>(T(0), T(0), T(-1));
        } 

        T& operator[](size_t index) { return data[index]; }
        const T& operator[](size_t index) const { return data[index]; }

        bool operator==(const Vector<T, 3>& other) const {
            return x == other.x && y == other.y && z == other.z;
        }
    };

    //Vector4
    template<typename T>
    struct alignas(16) Vector <T, 4>{
        union{
            T data[4];
            struct {T x, y, z, w; };
            struct {T r, g, b, a; };
        };

        //Constructor
        Vector() = default;
        explicit Vector(T scalar) : x(scalar), y(scalar), z(scalar), w(scalar) {};
        explicit Vector(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {};
        explicit Vector(const Vector<T, 2>& vec2, T z = 0, T w = 0) : x(vec2.x), y(vec2.y), z(z), w(w) {};
        explicit Vector(const Vector<T, 3>& vec3, T w = 0) : x(vec3.x), y(vec3.y), z(vec3.z), w(w) {};

        T& operator[](size_t index) { return data[index]; }
        const T& operator[](size_t index) const { return data[index]; }
    };


    using vec2 = Vector<float, 2>;
    using Vector2 = Vector<float, 2>;
    using float2 = Vector<float, 2>;
    using vec3 = Vector<float, 3>;
    using Vector3 = Vector<float, 3>;
    using float3 = Vector<float, 3>;
    using vec4 = Vector<float, 4>;
    using Vector4 = Vector<float, 4>;
    using float4 = Vector<float, 4>;

    //Operaciones con vectores

    //Las operaciones suponen que el vector está en el origen. Al final, todas estas
    //operaciones tienen un scope local, por eso las manejamos así, y luegos las
    //matrices de transformación hacen su trabajo de llevarlo a su lugar final en el espacio.
    template<typename T, size_t N>
    Vector<T, N> operator+(const Vector<T, N>& v1, const Vector<T, N>& v2){        
        Vector <T, N> result;
        for(size_t i = 0; i < N; i++){
            result[i] = v1[i] + v2[i];
        }
        return result;
    }

    template<typename T, size_t N>
    Vector<T, N> operator-(const Vector<T, N>& v1, const Vector<T, N>& v2){
        Vector <T, N> result;
        for(size_t i = 0; i < N; i++){
            result[i] = v1[i] - v2[i];
        }
        return result;
    }

    //Multiplicación por un escalar
    template<typename T, size_t N, typename Scalar>
    Vector<T, N> operator*(const Vector<T, N>& v1, const Scalar& scalar){
        Vector<T, N> result;
        for(size_t i = 0; i < N; i++){
            result[i] = v1[i] * static_cast<T>(scalar);
        }
        return result;
    }

    //Calculo de magnitud
    template<typename T, size_t N>
    float32_t magnitude(const Vector<T, N>& v) {
        float32_t data = 0;
        for(size_t i = 0; i < N; i++){
            data += (float32_t)pow(v[i], 2);
        }
        data = (float32_t)sqrt(data);
        return data;
    };

    //Normalizar un vector
    template<typename T, size_t N>
    Vector<T, N> normalize(const Vector<T, N>& v){
        auto magnitud = magnitude(v);
        Vector<T, N> result;
        for(size_t i = 0; i < N; i++){
            result[i] = v[i] / magnitud;
        }
        return result;
    }

    //Producto dot de un vector (no importa su dimension)
    template<typename T, size_t N>
    T dot(const Vector<T, N>& v1, const Vector<T, N>& v2){
        T result = 0;
        for(size_t i = 0; i < N; i++){
            result += v1[i] * v2[i];
        }
        return result;
    }

    //Producto cruz vectorial (Solo disponible en vectores 3D)
    template<typename T>
    Vector<T, 3> cross(const Vector<T, 3>& v1, const Vector<T, 3>& v2){
        return Vector<T, 3>(
            v1.y * v2.z - v1.z * v2.y,
            v1.z * v2.x - v1.x * v2.z,
            v1.x * v2.y - v1.y * v2.x
        );
    }

    template<typename T, size_t N>
    std::ostream& operator<<(std::ostream& os, const Vector<T, N>& vec){
        os << "Vector[";
        for (size_t i = 0; i < N; ++i){
            os << vec[i] << (i < N - 1 ? ", " : "");
        }
        os << "]";
        return os;
    }

    template<typename T, size_t N>
    std::string to_string(const Vector<T, N>& vec){
        std::ostringstream oss;
        oss << "Vector[";
        for (size_t i = 0; i < N; ++i){
            oss << vec[i] << (i < N - 1 ? ", " : "");
        }
        oss << "]";
        return oss.str();
    }    
};

//Funciones hash para los vectores.
namespace std {
    using namespace ilen::maths::vector;

    template<>
    struct hash<vec2> {
        size_t operator()(const vec2& v) const {
            size_t seed = 0;
            seed ^= hash<float>{}(v.x) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= hash<float>{}(v.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            return seed;
        }
    };

    template<>
    struct hash<vec3> {
        size_t operator()(const vec3& v) const {
            size_t seed = 0;
            seed ^= hash<float>{}(v.x) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= hash<float>{}(v.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= hash<float>{}(v.z) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            return seed;
        }
    };
}