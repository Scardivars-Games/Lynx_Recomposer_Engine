#!/bin/bash

echo "------------------------------------------------"
echo "   ILEN Engine - tvOS Build Configuration"
echo "------------------------------------------------"

# 1. Preguntar por la plataforma
echo "Selecciona el destino del build:"
echo "1) Nativo (Apple TV Hardware - arm64)"
echo "2) Simulador (Mac - arm64/x86_64)"
read -p "Opción [1-2]: " platform_opt

if [ "$platform_opt" == "1" ]; then
    PLATFORM_VAL="TV64"
    DIR_NAME="build-tvOS"
else
    PLATFORM_VAL="SIMULATOR"
    DIR_NAME="build-tvOS"
fi

# 2. Preguntar por optimización
read -p "¿Activar optimizaciones de ILEN? (s/n): " opt_opt

if [[ "$opt_opt" =~ ^[Ss]$ ]]; then
    OPTIMIZED="ON"
    BUILD_TYPE="Release"
else
    OPTIMIZED="OFF"
    BUILD_TYPE="Debug"
fi

echo "------------------------------------------------"
echo "Configurando para: $PLATFORM_VAL (Optimizado: $OPTIMIZED)"
echo "Directorio: $DIR_NAME"
echo "------------------------------------------------"

# 3. Crear directorio y ejecutar CMake
mkdir -p $DIR_NAME
cd $DIR_NAME

cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../toolchains/tvos.toolchain.cmake \
    -DPLATFORM=$PLATFORM_VAL \
    -DILEN_OPTIMIZED_BUILD_FLAGS=$OPTIMIZED \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -G Xcode \
    -DCMAKE_XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS=YES \
    -DCMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT="dwarf-with-dsym"

echo "------------------------------------------------"
echo "Configuración terminada. Abre el proyecto en $DIR_NAME"