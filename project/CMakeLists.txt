cmake_minimum_required(VERSION 4.0)

#Main game project

set(LRE_PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}")
set(LRE_PROJECT_FILE "${LRE_PROJECT_ROOT}/project.lynxproject")

if(NOT EXISTS "${LRE_PROJECT_FILE}")
    message(FATAL_ERROR "No existe el archivo del projecto ${LRE_PROJECT_FILE}")
endif()

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${LRE_PROJECT_FILE}")

file(READ "${LRE_PROJECT_FILE}" LRE_PROJECT_JSON)

string(JSON LRE_GAME_VERSION GET "${LRE_PROJECT_JSON}" project version)

project(LynxGameProject VERSION "${LRE_GAME_VERSION}" LANGUAGES C CXX)

if(APPLE)
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#Dependencias del motor

if(NOT TARGET Lynx::Engine)
    find_package(LynxRecomposerEngine CONFIG REQUIRED)
endif()

foreach(LRE_REQUIRED_TOOL LYNX_RE_FILE_BINARIZER LYNX_RE_SCENE_BINARIZER LYNX_RE_COMPONENTS_REFLECTION_TOOL)
    if(NOT TARGET ${LRE_REQUIRED_TOOL})
        if(NOT LRE_TOOLS_DIR)
            message(FATAL_ERROR "No existe ${LRE_REQUIRED_TOOL} y no se definió LRE_TOOLS_DIR")
        endif()

        find_program(
            LRE_REQUIRED_TOOL_PATH
            NAMES ${LRE_REQUIRED_TOOL}
            HINTS "${LRE_TOOLS_DIR}"
            REQUIRED
            NO_DEFAULT_PATH
        )

        add_executable(${LRE_REQUIRED_TOOL} IMPORTED GLOBAL)

        set_target_properties(${LRE_REQUIRED_TOOL} PROPERTIES IMPORTED_LOCATION "${LRE_REQUIRED_TOOL_PATH}")
    
        unset(LRE_REQUIRED_TOOL_PATH CACHE)
    endif()
endforeach()

if(NOT LRE_ENGINE_SCHEMA_FILE)
    message(FATAL_ERROR "El proyecto necesita LRE_ENGINE_SCHEMA_FILE")
endif()

if(NOT LRE_ENGINE_BINARY_ASSET_DIR)
    message(FATAL_ERROR "El proyecto necesita LRE_ENGINE_BINARY_ASSET_DIR")
endif()

if(NOT LRE_ENGINE_DEFAULT_ASSET_MANIFEST)
    message(FATAL_ERROR "El proyecto necesita LRE_ENGINE_DEFAULT_ASSET_MANIFEST")
endif()

#Plataforma objetivo

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
    message(FATAL_ERROR
        "Plataforma no soportada: ${CMAKE_SYSTEM_NAME}"
    )
endif()

set(LRE_ASSET_TARGET_NAME "${LRE_TARGET_NAME}")

if(CMAKE_SYSTEM_NAME MATCHES "^(iOS|tvOS)$" AND PLATFORM STREQUAL "SIMULATOR")
    string(APPEND LRE_ASSET_TARGET_NAME "-simulator")
endif()

#GAME METADATA

string(JSON LRE_GAME_NAME GET "${LRE_PROJECT_JSON}" project name)
string(JSON LRE_GAME_DEVELOPER GET "${LRE_PROJECT_JSON}" project developer)
string(JSON LRE_GAME_IDENTIFIER GET "${LRE_PROJECT_JSON}" project identifier)
string(JSON LRE_INITIAL_SCENE GET "${LRE_PROJECT_JSON}" project initialScene)
string(JSON LRE_WINDOW_TITLE GET "${LRE_PROJECT_JSON}" window title)
string(JSON LRE_WINDOW_WIDTH GET "${LRE_PROJECT_JSON}" window width)
string(JSON LRE_WINDOW_HEIGHT GET "${LRE_PROJECT_JSON}" window height)
string(JSON LRE_WINDOW_FRAMERATE GET "${LRE_PROJECT_JSON}" window framerate)
string(JSON LRE_WINDOW_CLOSABLE GET "${LRE_PROJECT_JSON}" window closable)
string(JSON LRE_WINDOW_RESIZABLE GET "${LRE_PROJECT_JSON}" window resizable)
string(JSON LRE_WINDOW_MAXIMIZABLE GET "${LRE_PROJECT_JSON}" window maximizable)
string(JSON LRE_WINDOW_FULLSCREEN GET "${LRE_PROJECT_JSON}" window fullscreen)
string(JSON LRE_GRAPHICS_POST_PROCESS GET "${LRE_PROJECT_JSON}" graphics postProcess)
string(JSON LRE_GRAPHICS_MOTION_BLUR GET "${LRE_PROJECT_JSON}" graphics motionBlur)
string(JSON LRE_GRAPHICS_DEPTH GET "${LRE_PROJECT_JSON}" graphics depth)
string(JSON LRE_GRAPHICS_STENCIL GET "${LRE_PROJECT_JSON}" graphics stencil)
string(JSON LRE_INSTANCE_BUFFER_SIZE GET "${LRE_PROJECT_JSON}" graphics instanceBufferSizeInMB)
string(JSON LRE_GENERAL_BUFFER_SIZE GET "${LRE_PROJECT_JSON}" graphics generalBufferSizeInMB)
string(JSON LRE_PHYSICS_UPDATES_PER_SECOND GET "${LRE_PROJECT_JSON}" physics updatedPerSecond)
string(JSON LRE_PHYSICS_GRAVITY GET "${LRE_PROJECT_JSON}" physics gravity)
string(JSON LRE_PHYSICS_RESISTANCE GET "${LRE_PROJECT_JSON}" physics linearMovementResistance)
string(JSON LRE_PHYSICS_ZERO_THRESHOLD GET "${LRE_PROJECT_JSON}" physics velocityZeroThreshold)
string(JSON LRE_PHYSICS_REALISTIC GET "${LRE_PROJECT_JSON}" physics solveRealisticCollision)
string(JSON LRE_PHYSICS_DEBUG_ARROWS GET "${LRE_PROJECT_JSON}" physics showDebugArrows)
string(JSON LRE_INITIAL_ENTITY_CAPACITY GET "${LRE_PROJECT_JSON}" ecs initialEntityCapacity)

function(lre_cpp_escape OUTPUT_VARIABLE INPUT_VALUE)
    string(
        REPLACE "\\"
        "\\\\"
        ESCAPED_VALUE
        "${INPUT_VALUE}"
    )

    string(
        REPLACE "\""
        "\\\""
        ESCAPED_VALUE
        "${ESCAPED_VALUE}"
    )

    set(
        ${OUTPUT_VARIABLE}
        "${ESCAPED_VALUE}"
        PARENT_SCOPE
    )
endfunction()

function(lre_cpp_bool OUTPUT_VARIABLE INPUT_VALUE)
    if(INPUT_VALUE)
        set(${OUTPUT_VARIABLE} true PARENT_SCOPE)
    else()
        set(${OUTPUT_VARIABLE} false PARENT_SCOPE)
    endif()
endfunction()

function(
    lre_find_predefined_asset_id
    OUTPUT_VARIABLE
    ASSET_GROUP
    ASSET_NAME
)
    string(
        JSON ASSET_COUNT
        LENGTH "${LRE_ENGINE_DEFAULT_ASSET_JSON}"
        "${ASSET_GROUP}"
    )

    if(ASSET_COUNT EQUAL 0)
        message(FATAL_ERROR
            "El grupo ${ASSET_GROUP} está vacío"
        )
    endif()

    math(
        EXPR LAST_ASSET_INDEX
        "${ASSET_COUNT} - 1"
    )

    set(FOUND_ASSET_ID "")

    foreach(ASSET_INDEX RANGE 0 ${LAST_ASSET_INDEX})
        string(
            JSON CURRENT_ASSET_NAME
            GET "${LRE_ENGINE_DEFAULT_ASSET_JSON}"
            "${ASSET_GROUP}"
            ${ASSET_INDEX}
            name
        )

        if(CURRENT_ASSET_NAME STREQUAL ASSET_NAME)
            string(
                JSON FOUND_ASSET_ID
                GET "${LRE_ENGINE_DEFAULT_ASSET_JSON}"
                "${ASSET_GROUP}"
                ${ASSET_INDEX}
                id
            )

            break()
        endif()
    endforeach()

    if(FOUND_ASSET_ID STREQUAL "")
        message(FATAL_ERROR
            "No existe el asset predefinido "
            "${ASSET_GROUP}/${ASSET_NAME}"
        )
    endif()

    set(
        ${OUTPUT_VARIABLE}
        "${FOUND_ASSET_ID}"
        PARENT_SCOPE
    )
endfunction()

# ----------------------------------------------------------------------
# Strings y booleanos que se escribirán como C++
# ----------------------------------------------------------------------

lre_cpp_escape(
    LRE_WINDOW_TITLE_CPP
    "${LRE_WINDOW_TITLE}"
)

lre_cpp_escape(
    LRE_GAME_DEVELOPER_CPP
    "${LRE_GAME_DEVELOPER}"
)

lre_cpp_escape(
    LRE_GAME_VERSION_CPP
    "${LRE_GAME_VERSION}"
)

lre_cpp_bool(
    LRE_PHYSICS_REALISTIC_CPP
    "${LRE_PHYSICS_REALISTIC}"
)

lre_cpp_bool(
    LRE_PHYSICS_DEBUG_ARROWS_CPP
    "${LRE_PHYSICS_DEBUG_ARROWS}"
)

# ----------------------------------------------------------------------
# Bitflags de ventana
# ----------------------------------------------------------------------

set(LRE_WINDOW_FLAGS 0)

if(LRE_WINDOW_FULLSCREEN)
    math(EXPR LRE_WINDOW_FLAGS
        "${LRE_WINDOW_FLAGS} | (1 << 0)"
    )
endif()

if(LRE_WINDOW_RESIZABLE)
    math(EXPR LRE_WINDOW_FLAGS
        "${LRE_WINDOW_FLAGS} | (1 << 1)"
    )
endif()

if(LRE_WINDOW_CLOSABLE)
    math(EXPR LRE_WINDOW_FLAGS
        "${LRE_WINDOW_FLAGS} | (1 << 2)"
    )
endif()

if(LRE_WINDOW_MAXIMIZABLE)
    math(EXPR LRE_WINDOW_FLAGS
        "${LRE_WINDOW_FLAGS} | (1 << 3)"
    )
endif()

# ----------------------------------------------------------------------
# Bitflags gráficos
# ----------------------------------------------------------------------

set(LRE_GRAPHICS_FLAGS 0)

if(LRE_GRAPHICS_DEPTH)
    math(EXPR LRE_GRAPHICS_FLAGS
        "${LRE_GRAPHICS_FLAGS} | (1 << 0)"
    )
endif()

if(LRE_GRAPHICS_STENCIL)
    math(EXPR LRE_GRAPHICS_FLAGS
        "${LRE_GRAPHICS_FLAGS} | (1 << 1)"
    )
endif()

if(LRE_GRAPHICS_POST_PROCESS)
    math(EXPR LRE_GRAPHICS_FLAGS
        "${LRE_GRAPHICS_FLAGS} | (1 << 10)"
    )
endif()

if(LRE_GRAPHICS_MOTION_BLUR)
    math(EXPR LRE_GRAPHICS_FLAGS
        "${LRE_GRAPHICS_FLAGS} | (1 << 11)"
    )
endif()

# ----------------------------------------------------------------------
# IDs predefinidos del motor
# ----------------------------------------------------------------------

file(
    READ
    "${LRE_ENGINE_DEFAULT_ASSET_MANIFEST}"
    LRE_ENGINE_DEFAULT_ASSET_JSON
)

lre_find_predefined_asset_id(
    LRE_ID_ERROR_SHADER
    shaders
    error
)

lre_find_predefined_asset_id(
    LRE_ID_ERROR_MATERIAL
    materials
    error
)

lre_find_predefined_asset_id(
    LRE_ID_ERROR_TEXTURE
    textures
    error-texture
)

lre_find_predefined_asset_id(
    LRE_ID_QUAD_MESH
    models
    quad
)

lre_find_predefined_asset_id(
    LRE_ID_POST_PROCESS_PRESENT
    shaders
    post-processing
)

lre_find_predefined_asset_id(
    LRE_ID_POST_PROCESS_CALCULATE
    shaders
    post-processing-calculations
)

lre_find_predefined_asset_id(
    LRE_ID_DEFAULT_TEXTURE
    textures
    default-texture
)

lre_find_predefined_asset_id(
    LRE_ID_DEFAULT_UNLIT_OPAQUE
    materials
    unlit_opaque
)

lre_find_predefined_asset_id(
    LRE_ID_DEFAULT_UNLIT_TRANSPARENT
    materials
    unlit_transparent
)

# ----------------------------------------------------------------------
# Directorios generados
# ----------------------------------------------------------------------

set(
    LRE_PROJECT_GENERATED_ROOT
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

set(
    LRE_PROJECT_GENERATED_INCLUDE_DIR
    "${LRE_PROJECT_GENERATED_ROOT}/include"
)

set(
    LRE_PROJECT_RUNTIME_HEADER
    "${LRE_PROJECT_GENERATED_INCLUDE_DIR}/game/runtime_options.h"
)

set(
    LRE_PROJECT_SCHEMA
    "${LRE_PROJECT_GENERATED_ROOT}/reflection/components.meta_lynxschema"
)

set(
    LRE_PROJECT_SERIALIZERS
    "${LRE_PROJECT_GENERATED_INCLUDE_DIR}/generated_serializers.inl"
)

set(
    LRE_PROJECT_REFLECTION_STAMP
    "${LRE_PROJECT_GENERATED_ROOT}/reflection/.build_complete"
)

set(
    LRE_REFLECTION_WORK_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/reflection_work"
)

set(
    LRE_PROJECT_ASSET_WORK_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/project_asset_work"
)

set(
    LRE_PROJECT_BINARY_ASSET_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/compiled_assets/${LRE_ASSET_TARGET_NAME}"
)

set(
    LRE_PROJECT_ASSET_STAMP
    "${LRE_PROJECT_BINARY_ASSET_DIR}/.build_complete"
)

file(
    MAKE_DIRECTORY
    "${LRE_PROJECT_GENERATED_INCLUDE_DIR}/game"
)

file(
    MAKE_DIRECTORY
    "${LRE_PROJECT_GENERATED_ROOT}/reflection"
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/runtime_options.h.in"
    "${LRE_PROJECT_RUNTIME_HEADER}"
    @ONLY
)

# ----------------------------------------------------------------------
# Entradas de reflexión
# ----------------------------------------------------------------------

file(
    GLOB_RECURSE
    LRE_GAME_HEADERS
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)

file(
    GLOB_RECURSE
    LRE_GAME_SOURCES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

set(LRE_REFLECTION_DEPENDENCIES
    ${LRE_GAME_HEADERS}
    ${LRE_GAME_SOURCES}
    "${LRE_ENGINE_SCHEMA_FILE}"
    LYNX_RE_COMPONENTS_REFLECTION_TOOL
)

if(TARGET lre_engine_assets)
    list(
        APPEND
        LRE_REFLECTION_DEPENDENCIES
        lre_engine_assets
    )
endif()

add_custom_command(
    OUTPUT
        "${LRE_PROJECT_REFLECTION_STAMP}"

    BYPRODUCTS
        "${LRE_PROJECT_SCHEMA}"
        "${LRE_PROJECT_SERIALIZERS}"

    COMMAND
        "${CMAKE_COMMAND}" -E rm -rf
        "${LRE_REFLECTION_WORK_DIR}"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${LRE_REFLECTION_WORK_DIR}/project_data/src"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${LRE_REFLECTION_WORK_DIR}/project_data/manifests"

    COMMAND
        "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${LRE_REFLECTION_WORK_DIR}/project_data/src/include"

    COMMAND
        "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${LRE_REFLECTION_WORK_DIR}/project_data/src/src"

    COMMAND
        "${CMAKE_COMMAND}" -E copy_if_different
        "${LRE_ENGINE_SCHEMA_FILE}"
        "${LRE_REFLECTION_WORK_DIR}/project_data/manifests/engine_defaults.meta_lynxschema"

    COMMAND
        $<TARGET_FILE:LYNX_RE_COMPONENTS_REFLECTION_TOOL>
        --lynx-folder "${LRE_REFLECTION_WORK_DIR}"

    COMMAND
        "${CMAKE_COMMAND}" -E copy_if_different
        "${LRE_REFLECTION_WORK_DIR}/project_data/manifests/components.meta_lynxschema"
        "${LRE_PROJECT_SCHEMA}"

    COMMAND
        "${CMAKE_COMMAND}" -E copy_if_different
        "${LRE_REFLECTION_WORK_DIR}/project_data/manifests/generated_serializers.inl"
        "${LRE_PROJECT_SERIALIZERS}"

    COMMAND
        "${CMAKE_COMMAND}" -E touch
        "${LRE_PROJECT_REFLECTION_STAMP}"

    DEPENDS
        ${LRE_REFLECTION_DEPENDENCIES}

    COMMENT
        "Generating project reflection"

    VERBATIM
)

add_custom_target(
    lre_game_reflection
    DEPENDS
        "${LRE_PROJECT_REFLECTION_STAMP}"
)

# ----------------------------------------------------------------------
# Compilación de assets del proyecto
# ----------------------------------------------------------------------

file(
    GLOB_RECURSE
    LRE_PROJECT_DATA_INPUTS
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    "${CMAKE_CURRENT_SOURCE_DIR}/project_data/*"
)

list(
    FILTER
    LRE_PROJECT_DATA_INPUTS
    EXCLUDE
    REGEX "/\\.DS_Store$"
)

set(LRE_PROJECT_ASSET_DEPENDENCIES
    ${LRE_PROJECT_DATA_INPUTS}
    "${LRE_PROJECT_REFLECTION_STAMP}"
    LYNX_RE_FILE_BINARIZER
    LYNX_RE_SCENE_BINARIZER
)

if(TARGET lre_engine_assets)
    list(
        APPEND
        LRE_PROJECT_ASSET_DEPENDENCIES
        lre_engine_assets
    )
endif()

add_custom_command(
    OUTPUT
        "${LRE_PROJECT_ASSET_STAMP}"

    COMMAND
        "${CMAKE_COMMAND}" -E rm -rf
        "${LRE_PROJECT_ASSET_WORK_DIR}"

    COMMAND
        "${CMAKE_COMMAND}" -E rm -rf
        "${LRE_PROJECT_BINARY_ASSET_DIR}"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${LRE_PROJECT_ASSET_WORK_DIR}"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${LRE_PROJECT_BINARY_ASSET_DIR}"

    COMMAND
        "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/project_data"
        "${LRE_PROJECT_ASSET_WORK_DIR}/project_data"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${LRE_PROJECT_ASSET_WORK_DIR}/project_data/manifests"

    COMMAND
        "${CMAKE_COMMAND}" -E copy_if_different
        "${LRE_PROJECT_SCHEMA}"
        "${LRE_PROJECT_ASSET_WORK_DIR}/project_data/manifests/components.meta_lynxschema"

    COMMAND
        $<TARGET_FILE:LYNX_RE_FILE_BINARIZER>
        --mode project
        --source-assets
            "${LRE_PROJECT_ASSET_WORK_DIR}/project_data/assets"
        --source-manifest
            "${LRE_PROJECT_ASSET_WORK_DIR}/project_data/manifests/assets.meta_lynxmanifest"
        --predefined-assets
            "${LRE_ENGINE_BINARY_ASSET_DIR}"
        --predefined-manifest
            "${LRE_ENGINE_DEFAULT_ASSET_MANIFEST}"
        --output
            "${LRE_PROJECT_BINARY_ASSET_DIR}"
        --compile-target
            "${LRE_ASSET_TARGET_NAME}"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${LRE_PROJECT_ASSET_WORK_DIR}/.lynx_bin_data/scenes"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${LRE_PROJECT_ASSET_WORK_DIR}/.lynx_bin_data/prefabs"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${LRE_PROJECT_ASSET_WORK_DIR}/.lynx_bin_data/manifests"

    COMMAND
        $<TARGET_FILE:LYNX_RE_SCENE_BINARIZER>
        --lynx-folder
            "${LRE_PROJECT_ASSET_WORK_DIR}"

    COMMAND
        "${CMAKE_COMMAND}" -E copy_directory
        "${LRE_PROJECT_ASSET_WORK_DIR}/.lynx_bin_data/scenes"
        "${LRE_PROJECT_BINARY_ASSET_DIR}/scenes"

    COMMAND
        "${CMAKE_COMMAND}" -E copy_directory
        "${LRE_PROJECT_ASSET_WORK_DIR}/.lynx_bin_data/prefabs"
        "${LRE_PROJECT_BINARY_ASSET_DIR}/prefabs"

    COMMAND
        "${CMAKE_COMMAND}" -E copy_directory
        "${LRE_PROJECT_ASSET_WORK_DIR}/.lynx_bin_data/manifests"
        "${LRE_PROJECT_BINARY_ASSET_DIR}/manifests"

    COMMAND
        "${CMAKE_COMMAND}" -E touch
        "${LRE_PROJECT_ASSET_STAMP}"

    DEPENDS
        ${LRE_PROJECT_ASSET_DEPENDENCIES}

    COMMENT
        "Compiling project assets and scenes"

    COMMAND_EXPAND_LISTS
    VERBATIM
)

add_custom_target(
    lre_game_assets
    DEPENDS
        "${LRE_PROJECT_ASSET_STAMP}"
)

add_dependencies(
    lre_game_assets
    lre_game_reflection
)

# ----------------------------------------------------------------------
# Ejecutable
# ----------------------------------------------------------------------

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(LRE_GAME_ENTRY_POINT "${CMAKE_CURRENT_SOURCE_DIR}/platform/iOS/ios_entry.mm")
elseif(CMAKE_SYSTEM_NAME STREQUAL "tvOS")
    set(LRE_GAME_ENTRY_POINT "${CMAKE_CURRENT_SOURCE_DIR}/platform/tvOS/tvos_entry.mm")
else()
    set(
        LRE_GAME_ENTRY_POINT
        "${CMAKE_CURRENT_SOURCE_DIR}/platform/desktop/main.cpp"
    )
endif()

if(APPLE)
    set(LRE_GAME_ICON)

    if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS"
       AND NOT CMAKE_SYSTEM_NAME STREQUAL "tvOS")
        set(LRE_GAME_ICON "${CMAKE_CURRENT_SOURCE_DIR}/platform/macOS/icon/AppIcon.icns")
        set_source_files_properties("${LRE_GAME_ICON}" PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
        )
    endif()

    add_executable(
        lre_game_project
        MACOSX_BUNDLE
        ${LRE_GAME_SOURCES}
        ${LRE_GAME_HEADERS}
        "${LRE_GAME_ENTRY_POINT}"
        "${LRE_GAME_ICON}"
        "${LRE_PROJECT_RUNTIME_HEADER}"
    )
else()
    add_executable(
        lre_game_project
        ${LRE_GAME_SOURCES}
        ${LRE_GAME_HEADERS}
        "${CMAKE_CURRENT_SOURCE_DIR}/platform/desktop/main.cpp"
        "${LRE_PROJECT_RUNTIME_HEADER}"
    )
endif()

target_include_directories(
    lre_game_project
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${LRE_PROJECT_GENERATED_INCLUDE_DIR}"
)

target_link_libraries(
    lre_game_project
    PRIVATE
        Lynx::Engine
        Lynx::BuildConfig
)

target_compile_features(
    lre_game_project
    PRIVATE
        cxx_std_23
)

add_dependencies(
    lre_game_project
    lre_game_assets
)

# ----------------------------------------------------------------------
# Bundle de iOS
# ----------------------------------------------------------------------

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(LRE_IOS_ASSET_CATALOG "${CMAKE_CURRENT_SOURCE_DIR}/platform/iOS/icon/Assets.xcassets")
    set(LRE_IOS_ICON_COMPOSER "${CMAKE_CURRENT_SOURCE_DIR}/platform/iOS/icon/AppIcon.icon")

    set_source_files_properties("${LRE_IOS_ASSET_CATALOG}" PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
        XCODE_EXPLICIT_FILE_TYPE "folder.assetcatalog"
    )
    set_source_files_properties("${LRE_IOS_ICON_COMPOSER}" PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
        XCODE_EXPLICIT_FILE_TYPE "folder.iconcomposer.icon"
    )
    target_sources(lre_game_project PRIVATE
        "${LRE_IOS_ASSET_CATALOG}"
        "${LRE_IOS_ICON_COMPOSER}"
    )
    target_link_libraries(lre_game_project PRIVATE
        "-framework Foundation"
        "-framework UIKit"
        "-framework QuartzCore"
        "-framework Metal"
        "-framework MetalKit"
        "-framework GameController"
    )
    target_link_options(lre_game_project PRIVATE
        "LINKER:-all_load"
    )
    target_compile_options(lre_game_project PRIVATE "-fno-objc-arc")

    set_target_properties(lre_game_project PROPERTIES
        OUTPUT_NAME "${LRE_GAME_NAME}"
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/platform/iOS/Info.plist"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${LRE_GAME_IDENTIFIER}"
        MACOSX_BUNDLE_BUNDLE_NAME "${LRE_GAME_NAME}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${LRE_GAME_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${LRE_GAME_VERSION}"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${LRE_GAME_IDENTIFIER}"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
        XCODE_ATTRIBUTE_SKIP_INSTALL "NO"
        XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
    )

    add_custom_command(TARGET lre_game_project POST_BUILD
        COMMAND "${CMAKE_COMMAND}"
            "-DLRE_ASSET_SOURCE=${LRE_PROJECT_BINARY_ASSET_DIR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/copy_ios_assets.cmake"
        COMMENT "Assembling iOS game assets"
        VERBATIM
    )
endif()

# ----------------------------------------------------------------------
# Bundle de tvOS
# ----------------------------------------------------------------------

if(CMAKE_SYSTEM_NAME STREQUAL "tvOS")
    set(LRE_TVOS_ASSET_CATALOG "${CMAKE_CURRENT_SOURCE_DIR}/platform/tvOS/icon/Assets.xcassets")

    set_source_files_properties("${LRE_TVOS_ASSET_CATALOG}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources" XCODE_EXPLICIT_FILE_TYPE "folder.assetcatalog")
    target_sources(lre_game_project PRIVATE "${LRE_TVOS_ASSET_CATALOG}")

    target_link_libraries(lre_game_project PRIVATE
        "-framework Foundation"
        "-framework UIKit"
        "-framework TVUIKit"
        "-framework QuartzCore"
        "-framework Metal"
        "-framework MetalKit"
        "-framework GameController"
    )

    target_link_options(lre_game_project PRIVATE
        "LINKER:-all_load"
    )    
    target_compile_options(lre_game_project PRIVATE "-fno-objc-arc")
    set_target_properties(lre_game_project PROPERTIES 
        OUTPUT_NAME "${LRE_GAME_NAME}"
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/platform/tvOS/Info.plist"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${LRE_GAME_IDENTIFIER}"
        MACOSX_BUNDLE_BUNDLE_NAME "${LRE_GAME_NAME}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${LRE_GAME_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${LRE_GAME_VERSION}"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${LRE_GAME_IDENTIFIER}"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "3"
        XCODE_ATTRIBUTE_SKIP_INSTALL "NO"
        XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
    )

    add_custom_command(TARGET lre_game_project POST_BUILD
        COMMAND "${CMAKE_COMMAND}" 
        "-DLRE_ASSET_SOURCE=${LRE_PROJECT_BINARY_ASSET_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/copy_ios_assets.cmake"

        COMMENT "Assembling tvOS game assets"
        VERBATIM
    )
endif()

# ----------------------------------------------------------------------
# Bundle de macOS
# ----------------------------------------------------------------------

if(APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS"
         AND NOT CMAKE_SYSTEM_NAME STREQUAL "tvOS")

    set_target_properties(
        lre_game_project
        PROPERTIES
            OUTPUT_NAME
                "${LRE_GAME_NAME}"

            MACOSX_BUNDLE
                TRUE

            MACOSX_BUNDLE_INFO_PLIST
                "${CMAKE_CURRENT_SOURCE_DIR}/platform/macOS/Info.plist"

            MACOSX_BUNDLE_BUNDLE_NAME
                "${LRE_GAME_NAME}"

            MACOSX_BUNDLE_GUI_IDENTIFIER
                "${LRE_GAME_IDENTIFIER}"

            MACOSX_BUNDLE_ICON_FILE
                "AppIcon.icns"

            MACOSX_BUNDLE_SHORT_VERSION_STRING
                "${LRE_GAME_VERSION}"

            MACOSX_BUNDLE_BUNDLE_VERSION
                "${LRE_GAME_VERSION}"

            BUILD_RPATH
                "@executable_path/../Frameworks"

            INSTALL_RPATH
                "@executable_path/../Frameworks"

            MACOSX_RPATH
                TRUE

            XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS
                "@executable_path/../Frameworks"
    )

    add_custom_command(
        TARGET
            lre_game_project

        POST_BUILD

        COMMAND
            "${CMAKE_COMMAND}" -E make_directory
            "$<TARGET_BUNDLE_CONTENT_DIR:lre_game_project>/Resources"

        COMMAND
            "${CMAKE_COMMAND}" -E copy_directory
            "${LRE_PROJECT_BINARY_ASSET_DIR}"
            "$<TARGET_BUNDLE_CONTENT_DIR:lre_game_project>/Resources"

        COMMAND
            "${CMAKE_COMMAND}" -E make_directory
            "$<TARGET_BUNDLE_CONTENT_DIR:lre_game_project>/Frameworks"

        COMMAND
            "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:Lynx::Engine>"
            "$<TARGET_BUNDLE_CONTENT_DIR:lre_game_project>/Frameworks/$<TARGET_FILE_NAME:Lynx::Engine>"

        COMMENT
            "Assembling macOS game bundle"

        VERBATIM
    )
endif()
