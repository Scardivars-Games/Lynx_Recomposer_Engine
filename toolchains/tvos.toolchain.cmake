set(CMAKE_SYSTEM_NAME tvOS)
set(CMAKE_SYSTEM_PROCESSOR arm64)

if(NOT DEFINED PLATFORM)
    set(PLATFORM TV64)
endif()

find_program(XCRUN_EXECUTABLE xcrun)
if(NOT XCRUN_EXECUTABLE)
    message(FATAL_ERROR "xcrun no encontrado")
endif()

# Configuración de SDK y Arquitecturas según la Plataforma
if(PLATFORM STREQUAL "TV64")
    set(TVOS_SDK_NAME "appletvos")
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "tvOS device architecture" FORCE)
    set(CMAKE_SYSTEM_PROCESSOR arm64)
elseif(PLATFORM STREQUAL "SIMULATOR")
    set(TVOS_SDK_NAME "appletvsimulator")
    # El simulador de tvOS moderno usa arm64 (Apple Silicon) o x86_64
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "tvOS device architecture" FORCE)
else()
    message(FATAL_ERROR "Plataforma inválida. Usa TV64 o SIMULATOR")
endif()

# Obtener la ruta del SDK automáticamente
execute_process(
    COMMAND ${XCRUN_EXECUTABLE} --sdk ${TVOS_SDK_NAME} --show-sdk-path
    OUTPUT_VARIABLE CMAKE_OSX_SYSROOT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Versión mínima de despliegue para tvOS
set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0" CACHE STRING "Versión mínima de despliegue para tvOS")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Atributos específicos de Xcode
set(CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "YES")
set(CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "3") # "3" identifica a la Apple TV