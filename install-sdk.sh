#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<'EOF'
Uso: ./install-sdk.sh [carpeta-de-instalación]

Compila e instala el SDK Release optimizado de Lynx Recomposer Engine para macOS.
Por defecto lo instala en dist/LynxSDK-macos-<arquitectura>-release.
Requiere CMake 4.0 o superior, Xcode y su toolchain de Metal.

Opcional: LRE_SLANG_DIR puede apuntar a una distribución de Slang para
macOS que contenga bin/slangc y sus bibliotecas.
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

if [[ $# -gt 1 || ( $# -eq 1 && -z "$1" ) ]]; then
    usage >&2
    exit 2
fi

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "Este script requiere macOS." >&2
    exit 1
fi

if ! command -v cmake >/dev/null; then
    echo "Instala CMake 4.0 o superior y añádelo al PATH." >&2
    exit 1
fi

if ! xcrun --sdk macosx -f metal >/dev/null; then
    echo "Xcode debe tener instalado el toolchain de Metal." >&2
    exit 1
fi

repo_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
arch="$(uname -m)"
sdk_name="LynxSDK-macos-${arch}-release"
build_dir="$repo_dir/build/sdk-macos-${arch}-release"
install_dir="${1:-$repo_dir/dist/$sdk_name}"
if [[ "$install_dir" != /* ]]; then
    install_dir="$PWD/$install_dir"
fi

slang_dir="${LRE_SLANG_DIR:-}"
if [[ -n "$slang_dir" ]] && ! "$slang_dir/bin/slangc" -version >/dev/null; then
    echo "LRE_SLANG_DIR debe contener un bin/slangc ejecutable en este Mac." >&2
    exit 1
fi

cmake -S "$repo_dir" -B "$build_dir" -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DLYNX_OPTIMIZED_BUILD_FLAGS=ON \
    -DCMAKE_OSX_ARCHITECTURES="$arch" \
    -DCMAKE_TOOLCHAIN_FILE="$repo_dir/toolchains/macos.toolchain.cmake" \
    -DCMAKE_INSTALL_PREFIX="$install_dir" \
    -DLRE_BUILD_GAME_PROJECT=OFF \
    -DLRE_BUILD_ENGINE_ASSETS=ON \
    -DLRE_TOOLS_USE_BUNDLED_ASSIMP=ON

cmake --build "$build_dir" --target lre_workspace --parallel "$(sysctl -n hw.ncpu)"
cmake --install "$build_dir"
cmake --install "$build_dir/lre_tools/build" --prefix "$install_dir" --component LynxSDKTools

if [[ -n "$slang_dir" ]]; then
    cmake -E copy_directory "$slang_dir" "$install_dir/bin/slang"
fi

printf '\nSDK instalado en %s\n' "$install_dir"
