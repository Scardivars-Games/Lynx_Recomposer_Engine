cmake_minimum_required(VERSION 3.30)
project(ILEN_MODULES LANGUAGES CXX VERSION 0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${PARENT_INCLUDE_DIR})

add_executable(ILEN_METAL_COMPILER modules/ilen_metal_compiler.cpp)
add_executable(ILEN_SLANG_COMPILER modules/ilen_slang_converter.cpp)
add_executable(ILEN_FILE_BINARIZATION
        modules/ilen_file_binarization.cpp
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

target_compile_definitions(ILEN_SLANG_COMPILER PRIVATE "MODULE_SOURCE_FOLDER=\"${CMAKE_SOURCE_DIR}\"")

add_custom_command(TARGET ILEN_SLANG_COMPILER POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/external/slang" "$<TARGET_FILE_DIR:ILEN_SLANG_COMPILER>/slang"
)
install(TARGETS ILEN_METAL_COMPILER DESTINATION executable/$<CONFIG>)
install(TARGETS ILEN_FILE_BINARIZATION DESTINATION executable/$<CONFIG>)
install(TARGETS ILEN_SLANG_COMPILER DESTINATION executable/$<CONFIG>)