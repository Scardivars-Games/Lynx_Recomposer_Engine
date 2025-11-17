cmake_minimum_required(VERSION 3.30)
project(ILEN_MODULES LANGUAGES CXX VERSION 0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${PARENT_INCLUDE_DIR})

add_executable(ILEN_METAL_COMPILER modules/ilen_metal_compiler.cpp)
add_executable(ILEN_FILE_BINARIZATION
        modules/ilen_file_binarization.cpp
        modules/common/logs.cpp
)

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/executable)
#
#foreach(config ${CMAKE_CONFIGURATION_TYPES})
#    string(TOUPPER ${config} config_upper)
#    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config_upper} ${CMAKE_BINARY_DIR}/executable)
#endforeach()


#target_link_libraries(ILEN_METAL_COMPILER PRIVATE ilen_global_includes)
#target_link_libraries(ILEN_FILE_BINARIZATION PRIVATE ilen_global_includes)

# Desactivar la compilación de herramientas, tests y ejemplos
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_BUILD_SAMPLES OFF)
set(ASSIMP_NO_EXPORT ON)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF)
set(ASSIMP_BUILD_STATIC_LIB ON)
set(ASSIMP_BUILD_ZLIB ON)

# Desactivar formatos de importación específicos (opcional)
set(ASSIMP_BUILD_OBJ_IMPORTER ON)
set(ASSIMP_BUILD_FBX_IMPORTER OFF)
set(ASSIMP_BUILD_GLTF_IMPORTER OFF)

add_subdirectory(external/assimp)

target_link_libraries(ILEN_FILE_BINARIZATION PRIVATE assimp)

install(TARGETS ILEN_METAL_COMPILER DESTINATION executable/$<CONFIG>)
install(TARGETS ILEN_FILE_BINARIZATION DESTINATION executable/$<CONFIG>)