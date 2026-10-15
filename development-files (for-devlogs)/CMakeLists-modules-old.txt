cmake_minimum_required(VERSION 3.30)
project(ILEN_MODULES LANGUAGES CXX VERSION 0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Forzar dependencias estáticas" FORCE)

include_directories(${PARENT_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/include)

add_executable(ILEN_METAL_COMPILER modules/ilen_metal_compiler.cpp)
add_executable(ILEN_TEMPORAL_SCENE_EDITOR modules/ilen_temporal_scene_editor.cpp)
target_include_directories(ILEN_TEMPORAL_SCENE_EDITOR PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
add_executable(ILEN_SLANG_COMPILER modules/ilen_slang_converter.cpp)
add_executable(ILEN_FILE_BINARIZATION
        modules/ilen_file_binarization.cpp
        modules/common/logs.cpp
)

add_executable(ILEN_SCENE_BINARIZER
        modules/ilen_scene_binarizer.cpp
        modules/common/logs.cpp
)

add_executable(ILEN_COMPONENTS_REFLECTION_TOOLS
        modules/ilen_components_reflection_tools.cpp
        modules/common/logs.cpp
)


#Firmado en macOS
#if(APPLE)
#        message(STATUS "Configurando la firma de las herramientas para macOS")
#        set(ENGINE_TOOLS ILEN_METAL_COMPILER ILEN_FILE_BINARIZATION) #Slang no se lista porque no se usa en macOS
#        foreach(TOOL ${ENGINE_TOOLS})
#                add_custom_command(TARGET ${TOOL} POST_BUILD
#                        COMMAND codesign --sign - --force "$<TARGET_FILE:${TOOL}>"
#                        COMMENT "Firmando ${TOOL} para macOS con ad-hoc")
#        endforeach()
#endif()

#ASSIMP
find_package(assimp QUIET)
if(assimp_FOUND)
        message(STATUS "ASSIMP presente en el sistema")
        set(ASSIMP_LIBRARIES assimp::assimp)
else()
# Desactivar la compilación de herramientas, tests y ejemplos
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_BUILD_SAMPLES OFF)
set(ASSIMP_NO_EXPORT ON)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF)
set(ASSIMP_BUILD_STATIC_LIB ON)
if (NOT APPLE)
    set(ASSIMP_BUILD_ZLIB ON)
endif()

# Desactivar formatos de importación específicos (opcional)
set(ASSIMP_BUILD_OBJ_IMPORTER ON)
set(ASSIMP_BUILD_FBX_IMPORTER OFF)
set(ASSIMP_BUILD_GLTF_IMPORTER OFF)

add_subdirectory(external/assimp)
set(ASSIMP_LIBRARIES assimp)
endif()

target_link_libraries(ILEN_FILE_BINARIZATION PRIVATE ${ASSIMP_LIBRARIES})

#END ASSIMP

#SLANG
target_compile_definitions(ILEN_SLANG_COMPILER PRIVATE "MODULE_SOURCE_FOLDER=\"${CMAKE_SOURCE_DIR}\"")

add_custom_command(TARGET ILEN_SLANG_COMPILER POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/external/slang" "$<TARGET_FILE_DIR:ILEN_SLANG_COMPILER>/slang"
)

#TEXTURE_COMPRESSION
set(ASTCENC_CLI OFF CACHE BOOL "Desactivar ejecutable CLI de astc-encoder" FORCE)

set(ASTCENC_SHAREDLIB OFF CACHE BOOL "Disable ASTC shared libs" FORCE)
set(ASTCENC_CLI OFF CACHE BOOL "Disable ASTC command line tool" FORCE)
set(ASTCENC_UNIVERSAL_BUILD OFF CACHE BOOL "Disable ASTC internal lipo scripts" FORCE)
add_subdirectory(external/astc-encoder)

file(GLOB BC7ENC_SOURCES "external/bc7enc/*.cpp" "external/bc7enc/*.c")

# ¡CUIDADO! Como su repositorio era un ejecutable, seguramente tiene un 
# archivo con una función 'int main()'. Si lo compilamos junto a nuestro motor, 
# chocará con tu propio main.cpp. Vamos a excluir cualquier archivo de prueba o consola:
list(FILTER BC7ENC_SOURCES EXCLUDE REGEX "(main\\.cpp|tool\\.cpp|test\\.cpp)$")

# Ahora creamos nosotros la librería estática usando sus archivos limpios:
add_library(BC7_Wrapper STATIC ${BC7ENC_SOURCES})

# Le decimos a nuestra nueva librería dónde están sus propios headers
target_include_directories(BC7_Wrapper PUBLIC "external/bc7enc")

target_link_libraries(ILEN_FILE_BINARIZATION PRIVATE BC7_Wrapper)
target_include_directories(ILEN_FILE_BINARIZATION PRIVATE "external/bc7enc" "external/astc-encoder/Source" "include")

#if(APPLE)
target_link_libraries(ILEN_FILE_BINARIZATION PRIVATE astcenc-native-static)
#endif()

#OLD IDEA
# Enlazamos automáticamente dependiendo de la arquitectura de la computadora
#if(APPLE AND CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64")
    # Para Mac con Apple Silicon (M1/M2/M3)
#    target_link_libraries(ILEN_FILE_BINARIZATION PRIVATE astcenc-neon-static)
#else()
    # Para PC (Windows/Linux) o Mac con Intel
#    target_link_libraries(ILEN_FILE_BINARIZATION PRIVATE astcenc-avx2-static)
#endif()

install(TARGETS ILEN_METAL_COMPILER DESTINATION executable/$<CONFIG>)
install(TARGETS ILEN_TEMPORAL_SCENE_EDITOR DESTINATION executable/$<CONFIG>)
install(TARGETS ILEN_FILE_BINARIZATION DESTINATION executable/$<CONFIG>)
install(TARGETS ILEN_SLANG_COMPILER DESTINATION executable/$<CONFIG>)
install(TARGETS ILEN_SCENE_BINARIZER DESTINATION executable/$<CONFIG>)
install(TARGETS ILEN_COMPONENTS_REFLECTION_TOOLS DESTINATION executable/$<CONFIG>)
