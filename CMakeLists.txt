cmake_minimum_required(VERSION 4.0)

project(LynxRecomposerEngineDevelopmentWorkspace VERSION 0.1.0 LANGUAGES C CXX)
#Antes Ilusion's Engine... pero ya esta en uso :(
#Nuevo nombre Lynx Recomposer
#Interno Lynx Recomposer Engine or LRE
#Slogan: The keys to recompose reality, in the paws of a lynx.

if(APPLE)
    enable_language(OBJCXX)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(ExternalProject)

# Rutas del Workspace

set(LRE_WORKSPACE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(LRE_ENGINE_ROOT "${LRE_WORKSPACE_ROOT}/engine")
set(LRE_PROJECT_ROOT "${LRE_WORKSPACE_ROOT}/project")
set(LRE_TOOLS_SOURCE_DIR "${LRE_WORKSPACE_ROOT}/external/lre_modules")

set(LRE_GENERATED_ROOT "${CMAKE_BINARY_DIR}/lre_generated")
set(LRE_ENGINE_SCHEMA_FILE "${LRE_GENERATED_ROOT}/schemas/engine_defaults.meta_lynxschema" CACHE FILEPATH "Generated LRE engine component schema" FORCE)
set(LRE_TOOLS_BUILD_DIR "${CMAKE_BINARY_DIR}/lre_tools")
set(LRE_TOOLS_DIR "${LRE_TOOLS_BUILD_DIR}/bin")

set(LRE_ENGINE_INCLUDE_DIR "${LRE_ENGINE_ROOT}/include" CACHE PATH "LRE public headers" FORCE)
set(LRE_ENGINE_DEFAULT_ASSET_MANIFEST 
    "${LRE_ENGINE_ROOT}/project_data/manifests/assets_defaults.meta_lynxmanifest"
    CACHE FILEPATH "LRE predefined asset manifest" FORCE)

# Plataforma objetivo

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(LRE_TARGET_NAME "iOS")
elseif(CMAKE_SYSTEM_NAME STREQUAL "tvOS")
    set(LRE_TARGET_NAME "tvOS")
elseif(APPLE)
    set(LRE_TARGET_NAME "macOS")
elseif(LINUX)
    set(LRE_TARGET_NAME "linux")
elseif(WIN32)
    set(LRE_TARGET_NAME "windows")
else()
    message(FATAL_ERROR "LRE no soporta la plataforma: ${CMAKE_SYSTEM_NAME}")
endif()

set(LRE_ENGINE_BINARY_ASSET_DIR "${LRE_GENERATED_ROOT}/engine_assets/${LRE_TARGET_NAME}" CACHE PATH "Compiled LRE assets" FORCE)
set(LRE_ENGINE_ASSET_STAMP "${LRE_ENGINE_BINARY_ASSET_DIR}/.build_complete" CACHE FILEPATH "LRE ASSET compilation stamp" FORCE)

set(LRE_BUILD_ENGINE_ASSETS ON CACHE BOOL "Create the LRE engine asset target")

#Herramientas nativas al OS de compilación
if(CMAKE_HOST_WIN32)
    set(LRE_HOST_EXECUTABLE_SUFFIX ".exe")
else()
    set(LRE_HOST_EXECUTABLE_SUFFIX "")
endif()

set(LRE_NATIVE_TOOL_NAMES
    LYNX_RE_METAL_COMPILER
    LYNX_RE_TEMPORAL_SCENE_EDITOR
    LYNX_RE_SLANG_COMPILER
    LYNX_RE_FILE_BINARIZER
    LYNX_RE_SCENE_BINARIZER
    LYNX_RE_COMPONENTS_REFLECTION_TOOL
)

set(LRE_NATIVE_TOOL_BYPRODUCTS)

foreach(LRE_TOOL_NAME IN LISTS LRE_NATIVE_TOOL_NAMES)
    list(APPEND LRE_NATIVE_TOOL_BYPRODUCTS "${LRE_TOOLS_DIR}/${LRE_TOOL_NAME}${LRE_HOST_EXECUTABLE_SUFFIX}")
endforeach()

ExternalProject_Add(
    lre_tools_build

    SOURCE_DIR "${LRE_TOOLS_SOURCE_DIR}"
    BINARY_DIR "${LRE_TOOLS_BUILD_DIR}/build"

    CMAKE_CACHE_ARGS "-DLRE_ENGINE_INCLUDE_DIR:PATH=${LRE_ENGINE_INCLUDE_DIR}"
    "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH=${LRE_TOOLS_DIR}"
    "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG:PATH=${LRE_TOOLS_DIR}"
    "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE:PATH=${LRE_TOOLS_DIR}"
    "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO:PATH=${LRE_TOOLS_DIR}"
    "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL:PATH=${LRE_TOOLS_DIR}"
    "-DBUILD_SHARED_LIBS:BOOL=OFF"

    BUILD_COMMAND "${CMAKE_COMMAND}" --build <BINARY_DIR> --config $<CONFIG>
    BUILD_BYPRODUCTS ${LRE_NATIVE_TOOL_BYPRODUCTS}
    INSTALL_COMMAND ""

    EXCLUDE_FROM_ALL TRUE
    LOG_CONFIGURE TRUE
    LOG_BUILD TRUE
)

add_custom_target(lre_tools DEPENDS lre_tools_build)

#Hacer los ejecutables de ExternalProject en comandos usables desde add_custom_command

function(lre_import_native_tool TOOL_NAME)
    set(TOOL_PATH "${LRE_TOOLS_DIR}/${TOOL_NAME}${LRE_HOST_EXECUTABLE_SUFFIX}")
    add_executable(${TOOL_NAME} IMPORTED GLOBAL)
    set_target_properties(${TOOL_NAME} PROPERTIES
        IMPORTED_LOCATION "${TOOL_PATH}"
        IMPORTED_LOCATION_DEBUG "${TOOL_PATH}"
        IMPORTED_LOCATION_RELEASE "${TOOL_PATH}"
        IMPORTED_LOCATION_RELWITHDEBINFO "${TOOL_PATH}"
        IMPORTED_LOCATION_MINSIZEREL "${TOOL_PATH}"
    )
    add_dependencies(${TOOL_NAME} lre_tools_build)
endfunction()

foreach(LRE_TOOL_NAME IN LISTS LRE_NATIVE_TOOL_NAMES)
    lre_import_native_tool(${LRE_TOOL_NAME})
endforeach()

add_subdirectory(engine)
add_subdirectory(project)

#General workspace

add_custom_target(lre_workspace)

add_dependencies(lre_workspace lre_engine lre_game_project)

if(TARGET lre_engine_assets)
    add_dependencies(lre_workspace lre_engine_assets)
endif()

if(TARGET lre_game_assets)
    add_dependencies(lre_workspace lre_game_assets)
endif()