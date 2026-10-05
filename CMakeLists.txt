cmake_minimum_required(VERSION 4.0)

project(IlusionsWorkspace VERSION 0.1.0 LANGUAGES C CXX)

if(APPLE)
    enable_language(OBJCXX)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE)
    set(CMAKE_OBJCXX_STANDARD 23)
    set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
    set(CMAKE_OBJCXX_EXTENSIONS OFF)
endif()

include(ExternalProject)

# =============================================================================
# Rutas principales del workspace
# =============================================================================

set(ILEN_WORKSPACE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(ILEN_ENGINE_ROOT "${ILEN_WORKSPACE_ROOT}/engine")
set(ILEN_ENGINE_DATA_DIR "${ILEN_ENGINE_ROOT}/project_data")
set(ILEN_PROJECT_ROOT "${ILEN_WORKSPACE_ROOT}/project")
set(ILEN_PROJECT_DATA_DIR "${ILEN_PROJECT_ROOT}/project_data")
set(ILEN_GENERATED_ROOT "${CMAKE_BINARY_DIR}/ilen_generated")
set(ILEN_GENERATED_REFLECTION_ROOT "${ILEN_GENERATED_ROOT}/reflection")
set(ILEN_GENERATED_SERIALIZERS "${ILEN_GENERATED_REFLECTION_ROOT}/project_data/manifests/generated_serializers.inl")
set(ILEN_GENERATED_ENGINE_SCHEMA "${ILEN_GENERATED_REFLECTION_ROOT}/project_data/manifests/engine_defaults.meta_ilenschema")
set(ILEN_GENERATED_COMPONENT_SCHEMA "${ILEN_GENERATED_REFLECTION_ROOT}/project_data/manifests/components.meta_ilenschema")

# =============================================================================
# Opciones globales
# =============================================================================

option(ILEN_ONLY_ECS "Use the engine exclusively through ECS" OFF)
option(ILEN_BUILD_TESTS "Build the engine tests" OFF)
option(ILEN_BUILD_PHYSICS_BENCHMARK "Build isolated physics tests and benchmarks" OFF)
option(ILEN_ENABLE_PHYSICS_DEBUG "Enable physics debug draw" ON)
option(ILEN_ENABLE_PHYSICS_STATS "Enable physics statistics" ON)
option(ILEN_METAL_HUD "Enable the Metal debug HUD" OFF)
option(ILEN_OPTIMIZED_BUILD "Enable aggressive optimization in builds" OFF)
option(ILEN_IGNORE_RENDERING "Disable rendering to test other features" OFF)
option(ILEN_TEST_ERROR_WINDOW_AT_START "Test the error window at startup" OFF)
option(ILEN_SINGLECORE_TEST "Force the job system to only use a single thread" OFF)
option(ILEN_DISABLE_BASIC_LOGGING "Disable the basic engine logging" OFF)
option(ILEN_DISABLE_TIME_LOGGING "Disable automatic loggin in time evaluations" ON)

# =============================================================================
# Plataforma de compilación
# =============================================================================

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(ILEN_TARGET_NAME "iOS")
    set(ILEN_METAL_SDK "iphoneos")

    if(PLATFORM STREQUAL "SIMULATOR")
        set(ILEN_METAL_SDK "iphonesimulator")
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "tvOS")
    set(ILEN_TARGET_NAME "tvOS")
    set(ILEN_METAL_SDK "appletvos")

    if(PLATFORM STREQUAL "SIMULATOR")
        set(ILEN_METAL_SDK "appletvsimulator")
    endif()
elseif(APPLE)
    set(ILEN_TARGET_NAME "macOS")
    set(ILEN_METAL_SDK "macosx")
elseif(LINUX)
    set(ILEN_TARGET_NAME "linux")
elseif(WIN32)
    set(ILEN_TARGET_NAME "windows")
else()
    message(FATAL_ERROR
        "Ilusions Engine does not currently configure this platform: "
        "${CMAKE_SYSTEM_NAME}"
    )   
endif()

# =============================================================================
# Configuración común compartida por motor y proyecto
# =============================================================================

add_library(ilen_build_config INTERFACE)
add_library(ilen::build_config ALIAS ilen_build_config)

target_compile_features(ilen_build_config INTERFACE cxx_std_23)
target_compile_definitions(
    ilen_build_config
    INTERFACE
        ILEN_ONLY_ECS=$<BOOL:${ILEN_ONLY_ECS}>
        ILEN_ENABLE_PHYSICS_DEBUG=$<BOOL:${ILEN_ENABLE_PHYSICS_DEBUG}>
        ILEN_ENABLE_PHYSICS_STATS=$<BOOL:${ILEN_ENABLE_PHYSICS_STATS}>

        IGNORE_RENDERING=$<BOOL:${ILEN_IGNORE_RENDERING}>
        TEST_ERROR_WINDOW_AT_START=$<BOOL:${ILEN_TEST_ERROR_WINDOW_AT_START}>
        ILEN_SINGLECORE_TEST=$<BOOL:${ILEN_SINGLECORE_TEST}>
        ILEN_DISABLE_BASIC_LOGGIN=$<BOOL:${ILEN_DISABLE_BASIC_LOGGING}>
        ILEN_DISABLE_TIME_LOGGING=$<BOOL:${ILEN_DISABLE_TIME_LOGGING}>

        ILEN_SOURCES_FOLDER="${ILEN_PROJECT_ROOT}"

        $<$<CONFIG:Debug>:ILEN_DEBUG=1>
)

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_compile_definitions(
        ilen_build_config INTERFACE ILEN_MOBILE=1 ILEN_IOS=1
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "tvOS")
    target_compile_definitions(
        ilen_build_config INTERFACE ILEN_MOBILE=1 ILEN_TVOS=1
    )
elseif(APPLE)
    target_compile_definitions(
        ilen_build_config INTERFACE ILEN_DESKTOP=1 ILEN_MACOS=1
    )
elseif(LINUX)
    target_compile_definitions(
        ilen_build_config INTERFACE ILEN_DESKTOP=1 ILEN_LINUX=1 VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
    )
elseif(WIN32)
    target_compile_definitions(
        ilen_build_config INTERFACE ILEN_DESKTOP=1 ILEN_WINDOWS=1
    )
endif()

# =============================================================================
# Optimizaciones opcionales
# =============================================================================

if(ILEN_OPTIMIZED_BUILD)
    if(MSVC)
        set(ILEN_OPTIMIZED_FLAGS /O2)
    else()
        set(ILEN_OPTIMIZED_FLAGS -O3 -ffast-math -funroll-loops -fomit-frame-pointer)
        if(CMAKE_SYSTEM_NAME MATCHES "iOS|tvOS")
            list(APPEND ILEN_OPTIMIZED_FLAGS -mcpu=apple-a13)
        elseif(APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
            list(APPEND ILEN_OPTIMIZED_FLAGS -mcpu=apple-m1)
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64")
            list(APPEND ILEN_OPTIMIZED_FLAGS -march=x86-64-v3)
        endif()
    endif()

    foreach(_flag IN LISTS ILEN_OPTIMIZED_FLAGS)
        target_compile_options(
            ilen_build_config INTERFACE
            "$<$<OR:$<CONFIG:RELEASE>,$<CONFIG:RelWithDebInfo>>:${_flag}>"
        )
    endforeach()
    
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
endif()

# =============================================================================
# Herramientas nativas de Ilusions Engine
#
# Estas herramientas siempre deben compilarse para el host.
# Al compilar para iOS/tvOS, el host continúa siendo macOS.
# =============================================================================

set(ILEN_MODULES_BUILD_DIR "${CMAKE_BINARY_DIR}/ILEN_MODULES")
set(ILEN_MODULES_EXECUTABLE_FOLDER "${ILEN_MODULES_BUILD_DIR}/executable")

set(
    ILEN_NATIVE_TOOL_NAMES
    ILEN_TEMPORAL_SCENE_EDITOR
    ILEN_METAL_COMPILER
    ILEN_FILE_BINARIZATION
    ILEN_SCENE_BINARIZER
    ILEN_COMPONENTS_REFLECTION_TOOLS
)

set(ILEN_NATIVE_TOOL_BYPRODUCTS)

foreach(_tool IN LISTS ILEN_NATIVE_TOOL_NAMES)
    list(APPEND ILEN_NATIVE_TOOL_BYPRODUCTS
        "${ILEN_MODULES_EXECUTABLE_FOLDER}/${_tool}"
    )
endforeach()

set(ILEN_NATIVE_TOOLCHAIN_ARGUMENTS)

if(APPLE)
    list(APPEND ILEN_NATIVE_TOOLCHAIN_ARGUMENTS
        "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${ILEN_WORKSPACE_ROOT}/toolchains/macos.toolchain.cmake"
        "-DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=12.0"
    )
elseif(WIN32)
    list(APPEND ILEN_NATIVE_TOOLCHAIN_ARGUMENTS
        "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${ILEN_WORKSPACE_ROOT}/toolchains/windows.toolchain.cmake"
    )
elseif(LINUX)
    list(APPEND ILEN_NATIVE_TOOLCHAIN_ARGUMENTS
        "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${ILEN_WORKSPACE_ROOT}/toolchains/linux.toolchain.cmake"
    )
endif()

ExternalProject_Add(
    ilen_modules
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/ilen_modules
    BINARY_DIR ${ILEN_MODULES_BUILD_DIR}/build

    CMAKE_CACHE_ARGS
        "-DPARENT_INCLUDE_DIR:PATH=${ILEN_ENGINE_ROOT}/include"

        "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH=${ILEN_MODULES_EXECUTABLE_FOLDER}"
        "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG:PATH=${ILEN_MODULES_EXECUTABLE_FOLDER}"
        "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE:PATH=${ILEN_MODULES_EXECUTABLE_FOLDER}"
        "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO:PATH=${ILEN_MODULES_EXECUTABLE_FOLDER}"
        "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL:PATH=${ILEN_MODULES_EXECUTABLE_FOLDER}"

        "-DCMAKE_INSTALL_PREFIX:PATH=${ILEN_MODULES_BUILD_DIR}"
        "-DBUILD_SHARED_LIBS:BOOL=OFF"

        ${ILEN_NATIVE_TOOLCHAIN_ARGUMENTS}

    BUILD_BYPRODUCTS ${ILEN_NATIVE_TOOL_BYPRODUCTS}
    BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config $<CONFIG>
    INSTALL_COMMAND ""

    LOG_CONFIGURE 1
    LOG_BUILD 1
    LOG_INSTALL 1
)

function(ilen_add_native_tool TOOL_NAME)
    set(_tool_path "${ILEN_MODULES_EXECUTABLE_FOLDER}/${TOOL_NAME}${CMAKE_EXECUTABLE_SUFFIX}")

    add_executable(${TOOL_NAME} IMPORTED GLOBAL)
    set_target_properties(${TOOL_NAME} PROPERTIES
        IMPORTED_CONFIGURATIONS "Debug;Release;RelWithDebInfo;MinSizeRel"
        IMPORTED_LOCATION "${_tool_path}"
        IMPORTED_LOCATION_DEBUG "${_tool_path}"
        IMPORTED_LOCATION_RELEASE "${_tool_path}"
        IMPORTED_LOCATION_RELWITHDEBINFO "${_tool_path}"
        IMPORTED_LOCATION_MINSIZEREL "${_tool_path}"
    )
    add_dependencies(${TOOL_NAME} ilen_modules)
endfunction()

foreach(_tool IN LISTS ILEN_NATIVE_TOOL_NAMES)
    ilen_add_native_tool(${_tool})
endforeach()

# =============================================================================
# Generación temporal de serializers
#
# scene_manager.cpp todavía incluye:
#
#   <manifests/generated_serializers.inl>
#
# Por eso se genera antes de compilar ilen_engine.
#
# La herramienta espera:
#
#   <root>/include/ilen
#   <root>/project_data/src
#   <root>/project_data/manifests
#
# Se construye ese layout dentro de build/.
# =============================================================================

file(GLOB_RECURSE ILEN_REFLECTION_INPUTS CONFIGURE_DEPENDS
    "${ILEN_ENGINE_ROOT}/include/*.h"
    "${ILEN_ENGINE_ROOT}/include/*.hpp"
    "${ILEN_PROJECT_ROOT}/include/*.h"
    "${ILEN_PROJECT_ROOT}/include/*.hpp"
    "${ILEN_PROJECT_ROOT}/src/*.h"
    "${ILEN_PROJECT_ROOT}/src/*.hpp"
)

add_custom_command(OUTPUT 
    "${ILEN_GENERATED_SERIALIZERS}" 
    "${ILEN_GENERATED_ENGINE_SCHEMA}" 
    "${ILEN_GENERATED_COMPONENT_SCHEMA}"
    
    COMMAND
        "${CMAKE_COMMAND}" -E rm -rf
        "${ILEN_GENERATED_REFLECTION_ROOT}"
    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${ILEN_GENERATED_REFLECTION_ROOT}/include"
    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${ILEN_GENERATED_REFLECTION_ROOT}/project_data/src/include"
    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${ILEN_GENERATED_REFLECTION_ROOT}/project_data/src/source"
    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${ILEN_GENERATED_REFLECTION_ROOT}/project_data/manifests"
    COMMAND
        "${CMAKE_COMMAND}" -E copy_directory
        "${ILEN_ENGINE_ROOT}/include"
        "${ILEN_GENERATED_REFLECTION_ROOT}/include"
    COMMAND
        "${CMAKE_COMMAND}" -E copy_directory
        "${ILEN_PROJECT_ROOT}/include"
        "${ILEN_GENERATED_REFLECTION_ROOT}/project_data/src/include"
    COMMAND
        "${CMAKE_COMMAND}" -E copy_directory
        "${ILEN_PROJECT_ROOT}/src"
        "${ILEN_GENERATED_REFLECTION_ROOT}/project_data/src/source"
    COMMAND
        $<TARGET_FILE:ILEN_COMPONENTS_REFLECTION_TOOLS>
        --ilen-folder "${ILEN_GENERATED_REFLECTION_ROOT}"
        --engine-mode
    COMMAND
        $<TARGET_FILE:ILEN_COMPONENTS_REFLECTION_TOOLS>
        --ilen-folder "${ILEN_GENERATED_REFLECTION_ROOT}"

    DEPENDS
        ILEN_COMPONENTS_REFLECTION_TOOLS
        ${ILEN_REFLECTION_INPUTS}
    COMMENT
        "Generating engine and project components serializers"
    VERBATIM
)

add_custom_target(ilen_reflection_codegen DEPENDS
    "${ILEN_GENERATED_SERIALIZERS}"
    "${ILEN_GENERATED_ENGINE_SCHEMA}"
    "${ILEN_GENERATED_COMPONENT_SCHEMA}"
)

# =============================================================================
# Proyectos principales
# =============================================================================

add_subdirectory(engine)
add_subdirectory(project)