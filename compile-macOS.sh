#!/bin/bash

echo "------------------------------------------------"
echo "   ILEN Engine - macOS Build Configuration"
echo "------------------------------------------------"

read -p "¿Activar optimizaciones de ILEN? (s/n): " opt_opt

DIR_NAME="build-macOS"

if [[ "$opt_opt" =~ ^[Ss]$ ]]; then
    OPTIMIZED="ON"
    BUILD_TYPE="Release"
else
    OPTIMIZED="OFF"
    BUILD_TYPE="Debug"
fi

echo "------------------------------------------------"
echo "Configurando para: macOS (Optimizado: $OPTIMIZED)"
echo "Directorio: $DIR_NAME"
echo "------------------------------------------------"

# 3. Crear directorio y ejecutar CMake
mkdir -p $DIR_NAME
cd $DIR_NAME

cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../toolchains/macos.toolchain.cmake \
    -DILEN_OPTIMIZED_BUILD_FLAGS=$OPTIMIZED \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -G Xcode \
    -DCMAKE_XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS=YES \
    -DCMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT="dwarf-with-dsym"

echo "------------------------------------------------"
echo "Configuración terminada. Abre el proyecto en $DIR_NAME"