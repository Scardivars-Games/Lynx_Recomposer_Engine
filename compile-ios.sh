#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "-------------------------------------------------------"
echo "   Lynx Recomposer Engine - iOS Project"
echo "-------------------------------------------------------"
echo "Selecciona el destino:"
echo "1) iPhone/iPad (arm64)"
echo "2) Simulator (arm64/x86_64)"
read -r -p "Opcion [1-2]: " platform_option

case "${platform_option}" in
    1)
        PLATFORM="OS64"
        PLATFORM_DIR="iphoneos"
        ;;
    2)
        PLATFORM="SIMULATOR"
        PLATFORM_DIR="iphonesimulator"
        ;;
    *)
        echo "Opcion invalida" >&2
        exit 2
        ;;
esac

read -r -p "Build Release optimizado? (s/n): " release_option
if [[ "${release_option}" =~ ^[Ss]$ ]]; then
    CONFIGURATION="Release"
    OPTIMIZED="ON"
else
    CONFIGURATION="Debug"
    OPTIMIZED="OFF"
fi

XCFRAMEWORK="${ROOT_DIR}/dist/ios/${CONFIGURATION}/LynxRecomposer.xcframework"
BUILD_DIR="${ROOT_DIR}/build/ios-app-${PLATFORM_DIR}-${CONFIGURATION}"

if [[ ! -d "${XCFRAMEWORK}" ]]; then
    echo "No existe el engine ${CONFIGURATION}; creando el XCFramework primero."

    set -euo pipefail

    ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
    BUILD_ROOT="${ROOT_DIR}/build/ios-xcframework-${CONFIGURATION}"
    OUTPUT_DIR="${ROOT_DIR}/dist/ios/${CONFIGURATION}"
    XCFRAMEWORK="${OUTPUT_DIR}/LynxRecomposer.xcframework"

    case "${CONFIGURATION}" in
        Debug|Release|RelWithDebInfo|MinSizeRel) ;;
        *)
            echo "Usage: $0 [Debug|Release|RelWithDebInfo|MinSizeRel]" >&2
            exit 2
            ;;
    esac

    build_slice() {
        local platform="$1"
        local build_dir="$2"

        cmake -S "${ROOT_DIR}" -B "${build_dir}" -G "Unix Makefiles" \
            -DCMAKE_TOOLCHAIN_FILE="${ROOT_DIR}/toolchains/ios.toolchain.cmake" \
            -DPLATFORM="${platform}" \
            -DCMAKE_BUILD_TYPE="${CONFIGURATION}" \
            -DLRE_BUILD_GAME_PROJECT=OFF \
            -DLRE_BUILD_ENGINE_ASSETS=OFF \
            -DLRE_ENGINE_STATIC=ON \
            -DLYNX_OPTIMIZED_BUILD_FLAGS="$([[ "${CONFIGURATION}" == Release || "${CONFIGURATION}" == RelWithDebInfo ]] && echo ON || echo OFF)"

        cmake --build "${build_dir}" --target lre_engine --parallel
    }

    echo "Building Lynx Recomposer for iPhoneOS (${CONFIGURATION})"
    build_slice OS64 "${BUILD_ROOT}/iphoneos"

    echo "Building Lynx Recomposer for iPhoneSimulator (${CONFIGURATION})"
    build_slice SIMULATOR "${BUILD_ROOT}/iphonesimulator"

    DEVICE_LIBRARY="${BUILD_ROOT}/iphoneos/engine/libLynxRecomposer.a"
    SIMULATOR_LIBRARY="${BUILD_ROOT}/iphonesimulator/engine/libLynxRecomposer.a"

    test -f "${DEVICE_LIBRARY}"
    test -f "${SIMULATOR_LIBRARY}"

    mkdir -p "${OUTPUT_DIR}"
    rm -rf "${XCFRAMEWORK}"

    xcodebuild -create-xcframework \
        -library "${DEVICE_LIBRARY}" -headers "${ROOT_DIR}/engine/include" \
        -library "${SIMULATOR_LIBRARY}" -headers "${ROOT_DIR}/engine/include" \
        -output "${XCFRAMEWORK}"

    echo "Created ${XCFRAMEWORK}"
fi

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Xcode \
    -DCMAKE_TOOLCHAIN_FILE="${ROOT_DIR}/toolchains/ios.toolchain.cmake" \
    -DPLATFORM="${PLATFORM}" \
    -DLYNX_OPTIMIZED_BUILD_FLAGS="${OPTIMIZED}" \
    -DLRE_ENGINE_XCFRAMEWORK="${XCFRAMEWORK}" \
    -DCMAKE_XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS=YES \
    -DCMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT="dwarf-with-dsym"

echo "-------------------------------------------------------"
echo "Proyecto generado: ${BUILD_DIR}"
echo "Engine precompilado: ${XCFRAMEWORK}"
echo "Abre LynxRecomposerEngineDevelopmentWorkspace.xcodeproj en Xcode."
