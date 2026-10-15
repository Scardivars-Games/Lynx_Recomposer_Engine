cmake_minimum_required(VERSION 4.0)

project(LynxRecomposerTools VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build dependencies statically" FORCE)

set(LRE_ENGINE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../engine/include")

if(NOT LRE_ENGINE_INCLUDE_DIR)
    message(FATAL_ERROR "LRE_ENGINE_INCLUDE_DIR must point to the public engine headers")
endif()

#headers compartidos
add_library(lre_tools_common INTERFACE)

target_include_directories(lre_tools_common INTERFACE "${LRE_ENGINE_INCLUDE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/include")

#Herramientas
add_executable(LYNX_RE_METAL_COMPILER modules/lynx_re_metal_compiler.cpp)
add_executable(LYNX_RE_TEMPORAL_SCENE_EDITOR modules/lynx_re_temporal_scene_editor.cpp)
add_executable(LYNX_RE_SLANG_COMPILER modules/lynx_re_slang_converter.cpp)
add_executable(LYNX_RE_FILE_BINARIZER modules/lynx_re_file_binarization.cpp modules/common/logs.cpp)
add_executable(LYNX_RE_SCENE_BINARIZER modules/lynx_re_scene_binarizer.cpp modules/common/logs.cpp)
add_executable(LYNX_RE_COMPONENTS_REFLECTION_TOOL modules/lynx_re_components_reflection_tools.cpp modules/common/logs.cpp)

set(LRE_TOOL_TARGET
    LYNX_RE_METAL_COMPILER
    LYNX_RE_TEMPORAL_SCENE_EDITOR
    LYNX_RE_SLANG_COMPILER
    LYNX_RE_FILE_BINARIZER
    LYNX_RE_SCENE_BINARIZER
    LYNX_RE_COMPONENTS_REFLECTION_TOOL
)

foreach(LRE_TOOL IN LISTS LRE_TOOL_TARGET)
    target_link_libraries(${LRE_TOOL} PRIVATE lre_tools_common)
endforeach()

#Assimp
find_package(assimp QUIET)
if(assimp_FOUND)
    set(LRE_ASSIMP_TARGET assimp::assimp)
else()
    set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_STATIC_LIB ON CACHE BOOL "" FORCE)

    set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_FBX_IMPORTER OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_GLTF_IMPORTER OFF CACHE BOOL "" FORCE)

    if(NOT APPLE)
        set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
    endif()

    add_subdirectory(external/assimp)
    set(LRE_ASSIMP_TARGET assimp)
endif()

target_link_libraries(LYNX_RE_FILE_BINARIZER PRIVATE ${LRE_ASSIMP_TARGET})

#ASTC Encoder

set(ASTCENC_CLI OFF CACHE BOOL "" FORCE)
set(ASTCENC_SHAREDLIB OFF CACHE BOOL "" FORCE)
set(ASTCENC_UNIVERSAL_BUILD OFF CACHE BOOL "" FORCE)

add_subdirectory(external/astc-encoder)
target_link_libraries(LYNX_RE_FILE_BINARIZER PRIVATE astcenc-native-static)
target_include_directories(LYNX_RE_FILE_BINARIZER PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/external/astc-encoder/Source")

#BC7 enconder
file(GLOB LRE_BC7_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/external/bc7enc/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/external/bc7enc/*.c")
list(FILTER LRE_BC7_SOURCES EXCLUDE REGEX "(main\\.cpp|tool\\.cpp|test\\.cpp)$")
add_library(lre_bc7_encoder STATIC ${LRE_BC7_SOURCES})

target_include_directories(lre_bc7_encoder PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/external/bc7enc")
target_link_libraries(LYNX_RE_FILE_BINARIZER PRIVATE lre_bc7_encoder)

#SLANG
target_compile_definitions(LYNX_RE_SLANG_COMPILER PRIVATE MODULE_SOURCE_FOLDER="${CMAKE_CURRENT_SOURCE_DIR}")
add_custom_command(TARGET LYNX_RE_SLANG_COMPILER POST_BUILD
    COMMAND 
        "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/external/slang"
        "$<TARGET_FILE_DIR:LYNX_RE_SLANG_COMPILER>/slang"
    VERBATIM
)

#Instalación opcional de las tools

install(TARGETS ${LRE_TOOL_TARGET} RUNTIME DESTINATION bin)