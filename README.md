# Lynx Recomposer Engine (LYNX)

Lynx Recomposer Engine (LYNX) es un motor de videojuegos escrito en C++ moderno (C++23) orientado a crear juegos
visualmente impactantes aprovechando eficientemente CPU y GPU. El motor prioriza APIs gráficas modernas (Metal, Vulkan,
DirectX 12) y mantiene compatibilidad con APIs legadas (OpenGL, DirectX 11 cuando sea posible).

For now, this repository is intended for internal use by Scardivar's Games. It may be released as open source in the
future once it is stable and validated with an internal launch.

Nota: Actualmente la lógica del motor y del juego están mezcladas; en el futuro se separarán.

---

## Overview

- Language: C++23 (Clang/AppleClang)
- Build system: CMake (>= 3.20), generates Xcode projects
- Platforms: macOS and iOS (Apple ecosystem focus)
- Graphics backend: Metal on Apple platforms (with plans for Vulkan/DirectX)
- External tools (built as part of the build):
  - `ILEN_METAL_COMPILER`: compila y empaqueta shaders de Metal
  - `ILEN_FILE_BINARIZATION`: organiza y binariza assets del proyecto

### Entry points
- Desktop (macOS): `src/main.cpp` contiene `int main()` (excluido en iOS via `#if !TARGET_OS_IOS`).
- iOS: punto de entrada en Objective‑C++ dentro de `src/Platform/iOS` (administrado por Xcode/CMake). 

## Requirements

- macOS con Xcode instalado
  - Xcode 14+ recomendado (AppleClang con soporte C++23)
  - macOS SDK y/o iOS SDK
- CMake 3.20 o superior
- Soporte de Metal (hardware y sistema operativo compatibles)
- Repositorio `external/ilen-modules` presente (forma parte del repo como carpeta externa)

> Nota: `ilen-modules` se construye como `ExternalProject` durante la configuración/compilación.

## Quick start

Clonar el repositorio y generar el proyecto de Xcode con los scripts provistos.

- macOS:
  ```bash
  ./compile-macOS.sh
  # Abre el proyecto generado (build-macOS) en Xcode y compila/ejecuta
  ```

- iOS (device/OS64):
  ```bash
  ./compile-ios.sh
  # Abre el proyecto generado (build-ios) en Xcode, selecciona esquema iOS y compila/ejecuta en dispositivo o simulador
  ```

También puedes usar CMake manualmente:
```bash
# macOS
cmake -S . -B build-macOS -G Xcode -DCMAKE_TOOLCHAIN_FILE=toolchains/macos.toolchain.cmake
cmake --build build-macOS --config Debug

# iOS (ejemplo para arm64/OS)
cmake -S . -B build-ios -G Xcode -DCMAKE_TOOLCHAIN_FILE=toolchains/ios.toolchain.cmake -DPLATFORM=OS64
cmake --build build-ios --config Debug
```

## Running

- macOS: Al compilar, CMake genera un bundle `ilen-engine.app`. Puedes ejecutarlo desde Xcode (Run) o desde Finder.
  - En carpetas de ejemplo de CMake (p. ej., `cmake-build-debug` o `build-macOS`), el bundle residirá en el directorio de productos de Xcode.
- iOS: Ejecutar desde Xcode en un dispositivo o simulador.

## Assets pipeline

Durante el `POST_BUILD`:
- `ILEN_FILE_BINARIZATION` procesa assets del repositorio (`assets/`) y genera/binariza datos en `.ilen-bin-data`.
- En macOS, los datos se copian a `Resources/` del bundle; en iOS, a `assets/` dentro del paquete de la app.
- `ILEN_METAL_COMPILER` compila shaders de Metal para el SDK objetivo (`--sdk macosx` o `--sdk iphoneos`).

Si `.ilen-bin-data` no existe, CMake la crea en la primera compilación en macOS.

## Project structure (high level)

- `src/` – código fuente del motor y ejemplos
  - `Common/` – lógica compartida (core, ecs, graphics, resources, window)
  - `Renderer/Metal/` – integración de Metal (ObjC++)
  - `Platform/` – código específico de plataforma (`apple/`, `macOS/`, `iOS/`)
  - `Examples/` – ejemplos de uso (si aplica)
- `include/` – headers públicos/compartidos (`ilen/`, `ilen-game/`, etc.)
- `assets/` – modelos, materiales, shaders y texturas
- `external/ilen-modules/` – herramientas y módulos externos construidos por CMake
- `toolchains/` – toolchains CMake para macOS/iOS
- `compile-macOS.sh`, `compile-ios.sh` – scripts para generar proyectos de Xcode

## CMake configuration highlights

- Proyecto: `project(ilen-engine LANGUAGES CXX OBJCXX VERSION 0.0.1)`
- Estándar: `CMAKE_CXX_STANDARD 23`
- Flags de compilación condicionales:
  - macOS (desktop): `ILEN_DESKTOP=1; ILEN_MACOS=1; ILEN_SOURCES_FOLDER="<repo_root>"`
  - iOS (mobile): `ILEN_MOBILE=1; ILEN_IOS=1`
- Post-build comunes: ejecución de `ILEN_FILE_BINARIZATION`
- Apple-only:
  - Enlace a frameworks: Metal, MetalKit, Cocoa (macOS) o UIKit+Foundation+QuartzCore (iOS)
  - Compilación de shaders con `ILEN_METAL_COMPILER` según SDK

## Scripts

- `compile-macOS.sh`
  - Genera proyecto Xcode en `build-macOS` usando `toolchains/macos.toolchain.cmake`.
- `compile-ios.sh`
  - Genera proyecto Xcode en `build-ios` usando `toolchains/ios.toolchain.cmake` y `-DPLATFORM=OS64`.

## Environment variables

No se requieren variables de entorno obligatorias detectadas.

- TODO: Documentar variables de entorno si las herramientas externas (`ilen-modules`) soportan configuración adicional.

## Tests

No se encontraron pruebas automatizadas en el repositorio.

- TODO: Añadir y documentar una estrategia de testing (por ejemplo, tests de unidad para módulos `core`, `maths` y pruebas de integración para render/ventana).

## Troubleshooting

- La generación de `ilen-modules` ocurre como `ExternalProject`. Si falla:
  - Asegúrate de que Xcode y los SDKs correspondientes estén instalados y seleccionados (`xcode-select -p`).
  - Verifica que `external/ilen-modules` exista y sea accesible.
  - Limpia la carpeta de build y reconfigura: `rm -rf build-macOS && ./compile-macOS.sh`.
- Problemas con shaders: confirma que `ILEN_METAL_COMPILER` se generó en `ILEN_MODULES/executable/<config>/`.

## License

Para uso exclusivo de Scardivar's Games por ahora. No se encontró archivo de licencia en el repositorio.

- TODO: Añadir un archivo `LICENSE` y especificar la licencia cuando el proyecto sea liberado públicamente.
