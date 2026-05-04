// Copyright (C) 2025, Scardivar's Games, Todos los derechos reservados.
// Código basado en Ilusion's Engine (IlEn), desarrollado por Owen Daniel Fernández Chacón.
// Este código es exclusivo para su uso dentro de proyectos relacionados con IlEn,
// y debe ser acreditado cuando se use en otros proyectos de acuerdo con los términos
// de la licencia. Si se distribuye el código, debe ser acreditado adecuadamente.

//Módulo de matrices del Ilusion's (Game) Engine (IlEn).
//Objetivo del módulo: Crear una implementación sencilla, fácil de leer y utilizar de matrices para el desarrollo de videojuego usando estándares modernos de programación en C++ (C++23)
//TODO: Calculos usand SIMD para disminuir los ciclos de procesador. De momento no es urgente para los primeros proyectos del motor, pero es algo necesario a futuro.

#pragma once

#include <iostream>
#include <sstream>
#include <cmath>
#include <cstddef>
#include <ilen/maths/vector.h>
#include <ilen/maths/angles.h>

//Namespace de bibliotecas internas
using namespace ilen::maths::vector;
using namespace ilen::maths::angles;


//Namespace que contiene todo lo relacionado con el módulo de matrices de IlEn. En este espacio se define, se crean varios tipos abreviados para un uso más fácil
//y se implementan numerosas funciones para el trabajo de estas en ámbitos de aplicaciones gráficas en tres dimensiones.
namespace ilen::maths::matrix{
    //Define una Matrix con tamaño Rows(Filas) y Cols(Columnas)
    //Esta se llena con elemento del tipo genérico T (int, float, double, etc)
    template<typename T, size_t Rows, size_t Cols>
    struct alignas(16) Matrix{
        //Debido a que este módulo es para gráficos, se usaran matrices column-major, o sea, cada vector contendrá los datos de una columna.
        //Solo en caso de DirectX se usará row-major.

        Vector<T, Rows> data[Cols]; //La matriz se compone de varios objetos Vector.
        //bool isRowMajor = false;
        
        //A partir de un índices dentro de [] obtienes el vector correspondiente que representa esa columna en la matrix.
        constexpr inline Vector<T, Rows>& operator[](size_t colIndex) { return data[colIndex]; };
        constexpr inline const Vector<T, Rows>& operator[](size_t colIndex) const { return data[colIndex]; };

        //Constructor por defecto. Genera una nueva matriz llena de 0;
        Matrix() = default;

        //Constructor por escalar
        //Genera una matriz donde la diagonal posee un valor escalar mientras que el resto de campos se inician en 0.
        //@param scalar El valor que inicializa la diagonal.
        explicit Matrix(T scalar) /*: isRowMajor(false)*/ {
            for(size_t i = 0; i < Cols; i++){
                for(size_t j = 0; j < Rows; j++){
                    data[i][j] = (i == j) ? scalar : T(0.0);
                }
            }
        }

        //Matriz de identidad
        //Esta función crea una matriz de identidad (su diagonal se compone del valor 1)
        static constexpr inline Matrix identity(){
            if constexpr (Cols == Rows && Cols == 4){
                Matrix result = {};
                result[0][0] = static_cast<T>(1.0);
                result[1][1] = static_cast<T>(1.0);
                result[2][2] = static_cast<T>(1.0);
                result[3][3] = static_cast<T>(1.0);
                return result;
            }else{
                Matrix result = {};
                size_t minDim = (Rows < Cols) ? Rows : Cols;
                for(size_t i = 0; i < minDim; i++) {
                    result[i][i] = static_cast<T>(1.0);
                }
                //result.isRowMajor = false;
                return result;
            }
        }

        //Constructor a partir de valores individuales
        //A partir de varios valores del tipo T se rellana cada valo de la matriz rellenando primera por filas la matrix
        //Para funcionar la cantidad de datos deben ser igual al resultado de las columnas * filas de la matrix.
        template<typename... Args>
        explicit Matrix(Args... args){
            static_assert(sizeof...(Args) == Rows * Cols, "Número incorrecto de argumentos para inicializar la matrix");
            T values[] = {static_cast<T>(args)...};
            for(size_t c = 0; c < Cols; c++){
                for(size_t r = 0; r < Rows; r++){
                    data[c][r] = values[c * Rows + r];
                }
            }
        }  
        
        //Constructor a partir de vectores TODO
        /*template<typename... VecArgs>
        explicit Matrix(const VecArgs&... rows) : data{rows...}{
            static_assert(sizeof...(rows) == Rows, "Número incorrecto de vectores para inicializar la matrix");
        }*/
    };

    //Operaciones de multiplicación
    // Matrix * Vector
    //Multiplica una matrix de tamaño R * C por un vector con dimensiones C, retornando un vector de dimension R.
    //@param mat Matriz a multiplicar
    //@param vec Vector a multiplicar
    // Matrix (R x C) * Vector (C) = Vector (R)
    template<typename T, size_t R, size_t C>
    constexpr inline Vector<T, R> operator*(const Matrix<T, R, C>& m, const Vector<T, C>& v){
        Vector<T, R> result;
        if constexpr (R == 4 && C == 4) {
            result[0] = m[0][0]*v[0] + m[1][0]*v[1] + m[2][0]*v[2] + m[3][0]*v[3];
            result[1] = m[0][1]*v[0] + m[1][1]*v[1] + m[2][1]*v[2] + m[3][1]*v[3];
            result[2] = m[0][2]*v[0] + m[1][2]*v[1] + m[2][2]*v[2] + m[3][2]*v[3];
            result[3] = m[0][3]*v[0] + m[1][3]*v[1] + m[2][3]*v[2] + m[3][3]*v[3];        
        } else{
            for(size_t r = 0; r < R; ++r) {
                T sum = 0;
                for(size_t c = 0; c < C; ++c) sum += m[c][r] * v[c];
                result[r] = sum;
            }
        }
        return result;
    }


    // Matrix * Matrix (column-major)
    template<typename T, size_t R1, size_t C1, size_t C2>
    constexpr inline Matrix<T, R1, C2> operator*(const Matrix<T, R1, C1>& a, const Matrix<T, R1, C2>& b) {
        Matrix<T, R1, C2> result;

        if constexpr (R1 == 4 && C1 == 4 && C2 == 4) {
            for(size_t col = 0; col < 4; col++){
                result[col][0] = a[0][0]*b[col][0] + a[1][0]*b[col][1] + a[2][0]*b[col][2] + a[3][0]*b[col][3];
                result[col][1] = a[0][1]*b[col][0] + a[1][1]*b[col][1] + a[2][1]*b[col][2] + a[3][1]*b[col][3];
                result[col][2] = a[0][2]*b[col][0] + a[1][2]*b[col][1] + a[2][2]*b[col][2] + a[3][2]*b[col][3];
                result[col][3] = a[0][3]*b[col][0] + a[1][3]*b[col][1] + a[2][3]*b[col][2] + a[3][3]*b[col][3];
            }
        } else {
            for(size_t c = 0; c < C2; ++c) {
                for(size_t r = 0; r < R1; ++r) {
                    T sum = 0;
                    for(size_t k = 0; k < C1; ++k) sum += a[k][r] * b[c][k];
                    result[c][r] = sum;
                }
            }
        }

        return result;
    }


    //Operación de traslación con matrices
    template<typename T>
    constexpr inline Matrix<T, 4, 4> translate(const Matrix<T, 4, 4>& m, const Vector<T, 3>& v) {
        Matrix<T, 4, 4> Result = m;
        Result[3][0] = m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z + m[3][0];
        Result[3][1] = m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z + m[3][1];
        Result[3][2] = m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z + m[3][2];
        Result[3][3] = m[0][3] * v.x + m[1][3] * v.y + m[2][3] * v.z + m[3][3];
        return Result;
    }

    //Operacion para escalar objetos mediante matrices
    template<typename T>
    constexpr inline Matrix<T, 4, 4> scale(const Matrix<T, 4, 4>& m, const Vector<T, 3>& v) {
        Matrix<T, 4, 4> Result;
        Result[0][0] = m[0][0] * v.x; Result[0][1] = m[0][1] * v.x; Result[0][2] = m[0][2] * v.x; Result[0][3] = m[0][3] * v.x;
        Result[1][0] = m[1][0] * v.y; Result[1][1] = m[1][1] * v.y; Result[1][2] = m[1][2] * v.y; Result[1][3] = m[1][3] * v.y;
        Result[2][0] = m[2][0] * v.z; Result[2][1] = m[2][1] * v.z; Result[2][2] = m[2][2] * v.z; Result[2][3] = m[2][3] * v.z;
        Result[3][0] = m[3][0];       Result[3][1] = m[3][1];       Result[3][2] = m[3][2];       Result[3][3] = m[3][3];
        return Result;
    }

    //Rotar matrices!!!
    template<typename T>
    constexpr inline Matrix<T, 4, 4> createTRS(const Vector<T, 3>& pos, const Vector<T, 3>& rotRadians, const Vector<T, 3>& scale) {
        Matrix<T, 4, 4> m;
        T cx = std::cos(rotRadians.x), sx = std::sin(rotRadians.x);
        T cy = std::cos(rotRadians.y), sy = std::sin(rotRadians.y);
        T cz = std::cos(rotRadians.z), sz = std::sin(rotRadians.z);

        m[0][0] = scale.x * (cy * cz);
        m[0][1] = scale.x * (sx * sy * cz - cx * sz);
        m[0][2] = scale.x * (cx * sy * cz + sx * sz);
        m[0][3] = static_cast<T>(0.0);

        m[1][0] = scale.y * (cy * sz);
        m[1][1] = scale.y * (sx * sy * sz + cx * cz);
        m[1][2] = scale.y * (cx * sy * sz - sx * cz);
        m[1][3] = static_cast<T>(0.0);

        m[2][0] = scale.z * (-sy);
        m[2][1] = scale.z * (sx * cy);
        m[2][2] = scale.z * (cx * cy);
        m[2][3] = static_cast<T>(0.0);

        m[3][0] = pos.x; m[3][1] = pos.y; m[3][2] = pos.z; m[3][3] = static_cast<T>(1.0);
        return m;
    }

    template<typename T>
    Matrix<T, 4, 4> lookAt(const Vector<T, 3>& cameraPos, const Vector<T, 3>& lookAtPoint, const Vector<T, 3>& worldUp){
        //Definiendo la orientación de la camara
        Vector<T, 3> forward = vector::normalize(cameraPos - lookAtPoint); //zaxis
        Vector<T, 3> right = vector::normalize(vector::cross(worldUp, forward)); //xaxis
        Vector<T, 3> up = vector::cross(forward, right); //yaxis
        
        //Log::Warning(to_string(forward));
        //Log::Warning(to_string(right));
       // Log::Warning(to_string(up));

        //Matriz vista
        auto lookAtMatrix = Matrix<T, 4, 4>::identity();
        lookAtMatrix[0][0] = right.x;
        lookAtMatrix[0][1] = right.y;
        lookAtMatrix[0][2] = right.z;

        lookAtMatrix[1][0] = up.x;
        lookAtMatrix[1][1] = up.y;
        lookAtMatrix[1][2] = up.z;

        lookAtMatrix[2][0] = forward.x;
        lookAtMatrix[2][1] = forward.y;
        lookAtMatrix[2][2] = forward.z;

        lookAtMatrix[3][0] = -vector::dot(right, cameraPos);
        lookAtMatrix[3][1] = -vector::dot(up, cameraPos);
        lookAtMatrix[3][2] = -vector::dot(forward, cameraPos);

        //lookAtMatrix = transposeColumnToRow(lookAtMatrix);

        return lookAtMatrix;
    }

    template<typename T>
    Matrix<T, 4, 4> perspective(const Degrees& fov, const T& aspectRatio, const T& nearPlane, const T& farPlane, ILEN_GPU_GRAPHICS_API_TYPE GraphicsAPI) {
        Radians fovInRadians(fov, ILEN_ANGLE_TYPE_DEGREES);
        
        /*T f = T(1.0) / tan(fovInRadians.getValue() / 2.0);

        Matrix<T, 4, 4> projectionMatrix(T(0.0));
        projectionMatrix[0][0] =  f / aspectRatio;
        projectionMatrix[1][1] =  f;
        projectionMatrix[2][2] = farPlane / (farPlane - nearPlane);
        projectionMatrix[2][3] = -(farPlane * nearPlane) / (farPlane - nearPlane);
        projectionMatrix[3][2] = T(1.0);
        projectionMatrix[3][3] = T(0.0);*/

        T scalingFactor = T(1.0) / tan(fovInRadians.value / T(2.0));
        Matrix<T, 4, 4> projectionMatrix(T(0.0));
        
        projectionMatrix[0][0] = scalingFactor / aspectRatio;
        projectionMatrix[1][1] = scalingFactor;

        if (GraphicsAPI == ILEN_GPU_GRAPHICS_API_TYPE::OpenGL) {
            projectionMatrix[2][2] = (farPlane + nearPlane) / (nearPlane - farPlane);
            projectionMatrix[3][2] = (2 * farPlane * nearPlane) / (nearPlane - farPlane);
            projectionMatrix[2][3] = T(-1);
        }else {
            projectionMatrix[2][2] = (farPlane) / (nearPlane - farPlane);
            projectionMatrix[3][2] = (farPlane * nearPlane) / (nearPlane - farPlane);
            projectionMatrix[2][3] = T(-1.0);
        }


        return projectionMatrix;
    }

    template<typename T>
    Matrix<T, 4, 4> orthographic(T left, T right, T bottom, T top, T nearPlane, T farPlane, ILEN_GPU_GRAPHICS_API_TYPE GraphicsAPI) {
        Matrix<T, 4, 4> projection(static_cast<T>(0.0));

        if (GraphicsAPI == ILEN_GPU_GRAPHICS_API_TYPE::OpenGL) {
            projection[2][2] = static_cast<T>(-2.0) / (farPlane - nearPlane); //Ojo: En directX posiblemente deba usarse -1
            projection[3][2] = -(farPlane + nearPlane) / (farPlane - nearPlane);
        }else {
            projection[2][2] = static_cast<T>(1) / (farPlane - nearPlane);
            projection[3][2] = -nearPlane / (farPlane - nearPlane);
        }

        projection[0][0] = static_cast<T>(2.0) / (right - left);
        projection[1][1] = static_cast<T>(2.0) / (top - bottom);
        projection[3][0] = -(right + left) / (right - left);
        projection[3][1] = -(top + bottom) / (top - bottom);
        projection[3][3] = static_cast<T>(1.0);

        return projection;
    }

    // Transponer matrix (Row-major -> Column-major)
    template<typename T, size_t Rows, size_t Cols>
    Matrix<T, Cols, Rows> transposeRowToColumn(const Matrix<T, Rows, Cols>& rowMajorMatrix) {
        Matrix<T, Cols, Rows> transposedMatrix(T(0));
        for (size_t row = 0; row < Rows; row++) {
            for (size_t col = 0; col < Cols; col++) {
                // intercambiamos fila por columna
                transposedMatrix[col][row] = rowMajorMatrix[row][col];
            }
        }
        //transposedMatrix.isRowMajor = false;
        return transposedMatrix;
    }

    // Transponer matrix (Column-major -> Row-major)
    template<typename T, size_t Cols, size_t Rows>
    Matrix<T, Rows, Cols> transposeColumnToRow(const Matrix<T, Cols, Rows>& colMajorMatrix) {
        Matrix<T, Rows, Cols> transposedMatrix(T(0));
        for (size_t col = 0; col < Cols; col++) {
            for (size_t row = 0; row < Rows; row++) {
                // intercambiamos columna por fila
                transposedMatrix[row][col] = colMajorMatrix[col][row];
            }
        }
        //transposedMatrix.isRowMajor = true;
        return transposedMatrix;
    }

    //Calculo del determinante de una matriz
    template<typename T>
    T determinant(const Matrix<T, 4, 4>& matrix) {
        T cofactor00 = matrix[2][2] * matrix[3][3] - matrix[3][2] * matrix[2][3];
        T cofactor01 = matrix[2][1] * matrix[3][3] - matrix[3][1] * matrix[2][3];
        T cofactor02 = matrix[2][1] * matrix[3][2] - matrix[3][1] * matrix[2][2];
        T cofactor03 = matrix[2][0] * matrix[3][3] - matrix[3][0] * matrix[2][3];
        T cofactor04 = matrix[2][0] * matrix[3][2] - matrix[3][0] * matrix[2][2];
        T cofactor05 = matrix[2][0] * matrix[3][1] - matrix[3][0] * matrix[2][1];

        T det = matrix[0][0] * (matrix[1][1] * cofactor00 - matrix[1][2] * cofactor01 + matrix[1][3] * cofactor02) -
                matrix[0][1] * (matrix[1][0] * cofactor00 - matrix[1][2] * cofactor03 + matrix[1][3] * cofactor04) +
                matrix[0][2] * (matrix[1][0] * cofactor01 - matrix[1][1] * cofactor03 + matrix[1][3] * cofactor05) -
                matrix[0][3] * (matrix[1][0] * cofactor02 - matrix[1][1] * cofactor04 + matrix[1][2] * cofactor05);

        return det;
    }

    template<typename T>
    constexpr inline Matrix<T, 4, 4> inverse(const Matrix<T, 4, 4>& m) {
        Matrix<T, 4, 4> inv;

        inv[0][0] =  m[1][1]*m[2][2]*m[3][3] - m[1][1]*m[2][3]*m[3][2] - m[2][1]*m[1][2]*m[3][3] + m[2][1]*m[1][3]*m[3][2] + m[3][1]*m[1][2]*m[2][3] - m[3][1]*m[1][3]*m[2][2];
        inv[1][0] = -m[1][0]*m[2][2]*m[3][3] + m[1][0]*m[2][3]*m[3][2] + m[2][0]*m[1][2]*m[3][3] - m[2][0]*m[1][3]*m[3][2] - m[3][0]*m[1][2]*m[2][3] + m[3][0]*m[1][3]*m[2][2];
        inv[2][0] =  m[1][0]*m[2][1]*m[3][3] - m[1][0]*m[2][3]*m[3][1] - m[2][0]*m[1][1]*m[3][3] + m[2][0]*m[1][3]*m[3][1] + m[3][0]*m[1][1]*m[2][3] - m[3][0]*m[1][3]*m[2][1];
        inv[3][0] = -m[1][0]*m[2][1]*m[3][2] + m[1][0]*m[2][2]*m[3][1] + m[2][0]*m[1][1]*m[3][2] - m[2][0]*m[1][2]*m[3][1] - m[3][0]*m[1][1]*m[2][2] + m[3][0]*m[1][2]*m[2][1];

        inv[0][1] = -m[0][1]*m[2][2]*m[3][3] + m[0][1]*m[2][3]*m[3][2] + m[2][1]*m[0][2]*m[3][3] - m[2][1]*m[0][3]*m[3][2] - m[3][1]*m[0][2]*m[2][3] + m[3][1]*m[0][3]*m[2][2];
        inv[1][1] =  m[0][0]*m[2][2]*m[3][3] - m[0][0]*m[2][3]*m[3][2] - m[2][0]*m[0][2]*m[3][3] + m[2][0]*m[0][3]*m[3][2] + m[3][0]*m[0][2]*m[2][3] - m[3][0]*m[0][3]*m[2][2];
        inv[2][1] = -m[0][0]*m[2][1]*m[3][3] + m[0][0]*m[2][3]*m[3][1] + m[2][0]*m[0][1]*m[3][3] - m[2][0]*m[0][3]*m[3][1] - m[3][0]*m[0][1]*m[2][3] + m[3][0]*m[0][3]*m[2][1];
        inv[3][1] =  m[0][0]*m[2][1]*m[3][2] - m[0][0]*m[2][2]*m[3][1] - m[2][0]*m[0][1]*m[3][2] + m[2][0]*m[0][2]*m[3][1] + m[3][0]*m[0][1]*m[2][2] - m[3][0]*m[0][2]*m[2][1];

        inv[0][2] =  m[0][1]*m[1][2]*m[3][3] - m[0][1]*m[1][3]*m[3][2] - m[1][1]*m[0][2]*m[3][3] + m[1][1]*m[0][3]*m[3][2] + m[3][1]*m[0][2]*m[1][3] - m[3][1]*m[0][3]*m[1][2];
        inv[1][2] = -m[0][0]*m[1][2]*m[3][3] + m[0][0]*m[1][3]*m[3][2] + m[1][0]*m[0][2]*m[3][3] - m[1][0]*m[0][3]*m[3][2] - m[3][0]*m[0][2]*m[1][3] + m[3][0]*m[0][3]*m[1][2];
        inv[2][2] =  m[0][0]*m[1][1]*m[3][3] - m[0][0]*m[1][3]*m[3][1] - m[1][0]*m[0][1]*m[3][3] + m[1][0]*m[0][3]*m[3][1] + m[3][0]*m[0][1]*m[1][3] - m[3][0]*m[0][3]*m[1][1];
        inv[3][2] = -m[0][0]*m[1][1]*m[3][2] + m[0][0]*m[1][2]*m[3][1] + m[1][0]*m[0][1]*m[3][2] - m[1][0]*m[0][2]*m[3][1] - m[3][0]*m[0][1]*m[1][2] + m[3][0]*m[0][2]*m[1][1];

        inv[0][3] = -m[0][1]*m[1][2]*m[2][3] + m[0][1]*m[1][3]*m[2][2] + m[1][1]*m[0][2]*m[2][3] - m[1][1]*m[0][3]*m[2][2] - m[2][1]*m[0][2]*m[1][3] + m[2][1]*m[0][3]*m[1][2];
        inv[1][3] =  m[0][0]*m[1][2]*m[2][3] - m[0][0]*m[1][3]*m[2][2] - m[1][0]*m[0][2]*m[2][3] + m[1][0]*m[0][3]*m[2][2] + m[2][0]*m[0][2]*m[1][3] - m[2][0]*m[0][3]*m[1][2];
        inv[2][3] = -m[0][0]*m[1][1]*m[2][3] + m[0][0]*m[1][3]*m[2][1] + m[1][0]*m[0][1]*m[2][3] - m[1][0]*m[0][3]*m[2][1] - m[2][0]*m[0][1]*m[1][3] + m[2][0]*m[0][3]*m[1][1];
        inv[3][3] =  m[0][0]*m[1][1]*m[2][2] - m[0][0]*m[1][2]*m[2][1] - m[1][0]*m[0][1]*m[2][2] + m[1][0]*m[0][2]*m[2][1] + m[2][0]*m[0][1]*m[1][2] - m[2][0]*m[0][2]*m[1][1];

        T det = m[0][0] * inv[0][0] + m[0][1] * inv[1][0] + m[0][2] * inv[2][0] + m[0][3] * inv[3][0];

        if (std::abs(det) < static_cast<T>(0.000001)) {
            return Matrix<T, 4, 4>::identity(); // Evitar división por cero
        }

        T invDet = static_cast<T>(1.0) / det;
        for (size_t i = 0; i < 4; i++) {
            for (size_t j = 0; j < 4; j++) inv[i][j] *= invDet;
        }

        return inv;
    }

    //Definiciones de matrices
    using mat4 = Matrix<float, 4, 4>;
    using float4x4 = Matrix<float, 4, 4>;
    using mat3 = Matrix<float, 3, 3>;
    using float3x3 = Matrix<float, 3, 3>;

    //Conversión a string
    template<typename T, size_t Rows, size_t Cols>
    std::string to_string(const Matrix<T, Rows, Cols>& mat){
        std::ostringstream oss;
        /*if(mat.isRowMajor){
            oss << "Matrix(" << Cols << "x" << Rows << ") [" << std::endl;
            for(size_t i = 0; i < Cols; i++){
                oss << to_string(mat[i]) << std::endl;
            }
            oss << "] Matrix_end";
        }else{*/
            //Hay que convertirlo para que sea bonito a los ojos
        oss << "Matrix Column-major. Orden en memoria: " << std::endl;
        for(size_t i = 0; i < Cols; i++){
            oss << to_string(mat[i]) << std::endl;
        }
        oss << "Orden tradicional legible: " << std::endl;
        auto matrix = transposeColumnToRow(mat);
        oss << "Matrix(" << Cols << "x" << Rows << ") [" << std::endl;
        for(size_t i = 0; i < Cols; i++){
            oss << to_string(matrix[i]) << std::endl;
        }
        oss << "] Matrix_end";
        //}

        return oss.str();
    };
};