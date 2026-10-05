# =============================================================================
# Metadatos de la aplicación
# =============================================================================

set(ILEN_GAME_TARGET "Lemons_Conspiracy")
set(ILEN_APP_IDENTIFIER "com.scardivargames.ilen_test")
set(ILEN_APP_NAME "Lemon's Conspiracy")
set(ILEN_APP_VERSION "0.0.1")
set(ILEN_APP_BUILD_NUMBER "1")
set(ILEN_APP_COPYRIGHT "Copyright (c) 2026 Scardivar's Games. All rights reserved.")
set(ILEN_APPLE_DEVELOPMENT_TEAM "3QYB6BMV6T" CACHE STRING "Apple development team used for code signing")

# =============================================================================
# Código del usuario
#
# OBJECT evita que el linker elimine los registradores estáticos de scripts.
# =============================================================================

file(
    GLOB_RECURSE
    ILEN_PROJECT_SOURCES
    CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.mm"
)

add_library(
    ilen_project_code
    OBJECT
)

target_sources(
    ilen_project_code
    PRIVATE
        ${ILEN_PROJECT_SOURCES}
)

target_include_directories(
    ilen_project_code

    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(
    ilen_project_code

    PUBLIC
        ilen::engine
        ilen::build_config
)

# =============================================================================
# Punto de entrada según la plataforma
# =============================================================================

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(
        ILEN_PROJECT_ENTRY_POINT
        "${CMAKE_CURRENT_SOURCE_DIR}/platform/iOS/ios_entry.mm"
    )

elseif(CMAKE_SYSTEM_NAME STREQUAL "tvOS")
    set(
        ILEN_PROJECT_ENTRY_POINT
        "${CMAKE_CURRENT_SOURCE_DIR}/platform/tvOS/tvos_entry.mm"
    )

else()
    set(
        ILEN_PROJECT_ENTRY_POINT
        "${CMAKE_CURRENT_SOURCE_DIR}/platform/desktop/main.cpp"
    )
endif()

if(APPLE)
    add_executable(
        ${ILEN_GAME_TARGET}
        MACOSX_BUNDLE
            "${ILEN_PROJECT_ENTRY_POINT}"
            $<TARGET_OBJECTS:ilen_project_code>
    )

else()
    add_executable(
        ${ILEN_GAME_TARGET}
            "${ILEN_PROJECT_ENTRY_POINT}"
            $<TARGET_OBJECTS:ilen_project_code>
    )
endif()

target_include_directories(
    ${ILEN_GAME_TARGET}

    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(
    ${ILEN_GAME_TARGET}

    PRIVATE
        ilen::engine
        ilen::build_config
)

# =============================================================================
# Configuración específica de Apple
# =============================================================================

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(
        ILEN_IOS_ASSET_CATALOG
        "${CMAKE_CURRENT_SOURCE_DIR}/platform/iOS/icon/Assets.xcassets"
    )

    set(
        ILEN_IOS_ICON_COMPOSER
        "${CMAKE_CURRENT_SOURCE_DIR}/platform/iOS/icon/AppIcon.icon"
    )

    set_source_files_properties(
        "${ILEN_IOS_ASSET_CATALOG}"
        PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
            XCODE_EXPLICIT_FILE_TYPE "folder.assetcatalog"
    )

    set_source_files_properties(
        "${ILEN_IOS_ICON_COMPOSER}"
        PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
            XCODE_EXPLICIT_FILE_TYPE "folder.iconcomposer.icon"
    )

    target_sources(
        ${ILEN_GAME_TARGET}
        PRIVATE
            "${ILEN_IOS_ASSET_CATALOG}"
            "${ILEN_IOS_ICON_COMPOSER}"
    )

    target_include_directories(
        ${ILEN_GAME_TARGET}
        PRIVATE
            "${ILEN_ENGINE_ROOT}/src/Platform/iOS"
    )

    target_link_libraries(
        ${ILEN_GAME_TARGET}
        PRIVATE
            "-framework Foundation"
            "-framework UIKit"
    )

    target_compile_options(
        ${ILEN_GAME_TARGET}
        PRIVATE
            "-fno-objc-arc"
    )

    set_target_properties(
        ${ILEN_GAME_TARGET}
        PROPERTIES
            MACOSX_BUNDLE TRUE

            MACOSX_BUNDLE_INFO_PLIST
                "${CMAKE_CURRENT_SOURCE_DIR}/platform/iOS/Info.plist"

            MACOSX_BUNDLE_GUI_IDENTIFIER
                "${ILEN_APP_IDENTIFIER}"

            MACOSX_BUNDLE_IDENTIFIER
                "${ILEN_APP_IDENTIFIER}"

            MACOSX_BUNDLE_NAME
                "${ILEN_APP_NAME}"

            MACOSX_BUNDLE_SHORT_VERSION_STRING
                "${ILEN_APP_VERSION}"

            MACOSX_BUNDLE_VERSION
                "${ILEN_APP_BUILD_NUMBER}"

            MACOSX_BUNDLE_COPYRIGHT
                "${ILEN_APP_COPYRIGHT}"

            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER
                "${ILEN_APP_IDENTIFIER}"

            XCODE_ATTRIBUTE_CODE_SIGN_STYLE
                "Automatic"

            XCODE_ATTRIBUTE_DEVELOPMENT_TEAM
                "${ILEN_APPLE_DEVELOPMENT_TEAM}"

            XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME
                "AppIcon"

            XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_ASSET_PATHS
                "${ILEN_IOS_ASSET_CATALOG}"

            XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY
                "1,2"

            XCODE_ATTRIBUTE_SKIP_INSTALL
                "NO"

            XCODE_ATTRIBUTE_INSTALL_PATH
                "$(LOCAL_APPS_DIR)"
    )

elseif(CMAKE_SYSTEM_NAME STREQUAL "tvOS")
    set(
        ILEN_TVOS_ASSET_CATALOG
        "${CMAKE_CURRENT_SOURCE_DIR}/platform/tvOS/icon/Assets.xcassets"
    )

    set_source_files_properties(
        "${ILEN_TVOS_ASSET_CATALOG}"
        PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
            XCODE_EXPLICIT_FILE_TYPE "folder.assetcatalog"
    )

    target_sources(
        ${ILEN_GAME_TARGET}
        PRIVATE
            "${ILEN_TVOS_ASSET_CATALOG}"
    )

    target_include_directories(
        ${ILEN_GAME_TARGET}
        PRIVATE
            "${ILEN_ENGINE_ROOT}/src/Platform/tvOS"
    )

    target_link_libraries(
        ${ILEN_GAME_TARGET}
        PRIVATE
            "-framework Foundation"
            "-framework UIKit"
            "-framework TVUIKit"
    )

    target_compile_options(
        ${ILEN_GAME_TARGET}
        PRIVATE
            "-fno-objc-arc"
    )

    set_target_properties(
        ${ILEN_GAME_TARGET}
        PROPERTIES
            MACOSX_BUNDLE TRUE

            MACOSX_BUNDLE_INFO_PLIST
                "${CMAKE_CURRENT_SOURCE_DIR}/platform/tvOS/Info.plist"

            MACOSX_BUNDLE_GUI_IDENTIFIER
                "${ILEN_APP_IDENTIFIER}"

            MACOSX_BUNDLE_IDENTIFIER
                "${ILEN_APP_IDENTIFIER}"

            MACOSX_BUNDLE_NAME
                "${ILEN_APP_NAME}"

            MACOSX_BUNDLE_SHORT_VERSION_STRING
                "${ILEN_APP_VERSION}"

            MACOSX_BUNDLE_VERSION
                "${ILEN_APP_BUILD_NUMBER}"

            MACOSX_BUNDLE_COPYRIGHT
                "${ILEN_APP_COPYRIGHT}"

            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER
                "${ILEN_APP_IDENTIFIER}"

            XCODE_ATTRIBUTE_CODE_SIGN_STYLE
                "Automatic"

            XCODE_ATTRIBUTE_DEVELOPMENT_TEAM
                "${ILEN_APPLE_DEVELOPMENT_TEAM}"

            XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME
                "AppIcon"

            XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_ASSET_PATHS
                "${ILEN_TVOS_ASSET_CATALOG}"

            XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY
                "3"

            XCODE_ATTRIBUTE_SKIP_INSTALL
                "NO"

            XCODE_ATTRIBUTE_INSTALL_PATH
                "$(LOCAL_APPS_DIR)"
    )

elseif(APPLE)
    set(
        ILEN_MACOS_ICON
        "${CMAKE_CURRENT_SOURCE_DIR}/platform/macOS/icon/AppIcon.icns"
    )

    set(
        ILEN_MACOS_ASSET_CATALOG
        "${CMAKE_CURRENT_SOURCE_DIR}/platform/macOS/icon/Assets.car"
    )

    set_source_files_properties(
        "${ILEN_MACOS_ICON}"
        "${ILEN_MACOS_ASSET_CATALOG}"
        PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
    )

    target_sources(
        ${ILEN_GAME_TARGET}
        PRIVATE
            "${ILEN_MACOS_ICON}"
            "${ILEN_MACOS_ASSET_CATALOG}"
    )

    target_link_libraries(
        ${ILEN_GAME_TARGET}
        PRIVATE
            "-framework Cocoa"
    )

    set_target_properties(
        ${ILEN_GAME_TARGET}
        PROPERTIES
            BUNDLE TRUE
            MACOSX_BUNDLE TRUE

            MACOSX_BUNDLE_INFO_PLIST
                "${CMAKE_CURRENT_SOURCE_DIR}/platform/macOS/Info.plist"

            MACOSX_BUNDLE_GUI_IDENTIFIER
                "${ILEN_APP_IDENTIFIER}"

            MACOSX_BUNDLE_IDENTIFIER
                "${ILEN_APP_IDENTIFIER}"

            MACOSX_BUNDLE_NAME
                "${ILEN_APP_NAME}"

            MACOSX_BUNDLE_BUNDLE_NAME
                "${ILEN_APP_NAME}"

            MACOSX_BUNDLE_SHORT_VERSION_STRING
                "${ILEN_APP_VERSION}"

            MACOSX_BUNDLE_VERSION
                "${ILEN_APP_BUILD_NUMBER}"

            MACOSX_BUNDLE_COPYRIGHT
                "${ILEN_APP_COPYRIGHT}"

            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER
                "${ILEN_APP_IDENTIFIER}"

            XCODE_ATTRIBUTE_SKIP_INSTALL
                "NO"

            XCODE_ATTRIBUTE_INSTALL_PATH
                "$(LOCAL_APPS_DIR)"
    )

elseif(LINUX)
    set_target_properties(
        ${ILEN_GAME_TARGET}
        PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY
                "${CMAKE_BINARY_DIR}/${ILEN_GAME_TARGET}"
    )
endif()
# =============================================================================
# Assets finales del videojuego
# =============================================================================

set(
    ILEN_PROJECT_ASSET_MANIFEST
    "${ILEN_PROJECT_DATA_DIR}/manifests/assets.meta_ilenmanifest"
)

set(
    ILEN_ENGINE_DEFAULT_ASSET_MANIFEST
    "${ILEN_ENGINE_DATA_DIR}/manifests/assets_defaults.meta_ilenmanifest"
)

file(
    GLOB_RECURSE
    ILEN_PROJECT_ASSET_INPUTS
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    "${ILEN_PROJECT_DATA_DIR}/assets/*"
)

list(
    FILTER
    ILEN_PROJECT_ASSET_INPUTS
    EXCLUDE
    REGEX "/\\.DS_Store$"
)

string(
    JOIN
    "\n"
    ILEN_PROJECT_ASSET_INVENTORY_CONTENT
    ${ILEN_PROJECT_ASSET_INPUTS}
)

set(
    ILEN_PROJECT_ASSET_INVENTORY_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/ilen_project_asset_inventory.txt"
)

file(
    GENERATE
    OUTPUT
        "${ILEN_PROJECT_ASSET_INVENTORY_FILE}"
    CONTENT
        "${ILEN_PROJECT_ASSET_INVENTORY_CONTENT}\n"
)

set(
    ILEN_PROJECT_ASSET_DEPENDENCIES
    ${ILEN_PROJECT_ASSET_INPUTS}
    "${ILEN_PROJECT_ASSET_INVENTORY_FILE}"
    "${ILEN_ENGINE_ASSET_STAMP}"
    "${ILEN_ENGINE_DEFAULT_ASSET_MANIFEST}"
    ILEN_FILE_BINARIZATION
)

if(EXISTS "${ILEN_PROJECT_ASSET_MANIFEST}")
    list(
        APPEND
        ILEN_PROJECT_ASSET_DEPENDENCIES
        "${ILEN_PROJECT_ASSET_MANIFEST}"
    )
endif()

set(
    ILEN_PROJECT_SCENE_WORK_ROOT
    "${ILEN_GENERATED_ROOT}/project_scene_compiler"
)

file(
    GLOB_RECURSE
    ILEN_PROJECT_SCENE_INPUTS
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    "${ILEN_PROJECT_DATA_DIR}/scenes/*"
    "${ILEN_PROJECT_DATA_DIR}/assets/prefabs/*"
)

list(
    FILTER
    ILEN_PROJECT_SCENE_INPUTS
    EXCLUDE
    REGEX "/\\.DS_Store$"
)

list(
    APPEND
    ILEN_PROJECT_ASSET_DEPENDENCIES
    ${ILEN_PROJECT_SCENE_INPUTS}
    "${ILEN_GENERATED_COMPONENT_SCHEMA}"
    ILEN_SCENE_BINARIZER
)
add_custom_command(
    OUTPUT
        "${ILEN_GAME_ASSET_STAMP}"

    # -------------------------------------------------------------------------
    # Preparar únicamente el layout temporal requerido por la tool de escenas.
    # -------------------------------------------------------------------------

    COMMAND
        "${CMAKE_COMMAND}" -E rm -rf
        "${ILEN_PROJECT_SCENE_WORK_ROOT}"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${ILEN_PROJECT_SCENE_WORK_ROOT}/project_data/assets/prefabs"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${ILEN_PROJECT_SCENE_WORK_ROOT}/project_data/scenes"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${ILEN_PROJECT_SCENE_WORK_ROOT}/project_data/manifests"

    COMMAND
        "${CMAKE_COMMAND}" -E copy_directory
        "${ILEN_PROJECT_DATA_DIR}/scenes"
        "${ILEN_PROJECT_SCENE_WORK_ROOT}/project_data/scenes"

    COMMAND
        "${CMAKE_COMMAND}" -E copy_directory
        "${ILEN_PROJECT_DATA_DIR}/assets/prefabs"
        "${ILEN_PROJECT_SCENE_WORK_ROOT}/project_data/assets/prefabs"

    # Copia cualquier scenes.meta_ilenmanifest anterior para conservar IDs.
    COMMAND
        "${CMAKE_COMMAND}" -E copy_directory
        "${ILEN_PROJECT_DATA_DIR}/manifests"
        "${ILEN_PROJECT_SCENE_WORK_ROOT}/project_data/manifests"

    COMMAND
        "${CMAKE_COMMAND}" -E copy_if_different
        "${ILEN_GENERATED_COMPONENT_SCHEMA}"
        "${ILEN_PROJECT_SCENE_WORK_ROOT}/project_data/manifests/components.meta_ilenschema"

    # -------------------------------------------------------------------------
    # Generar assets finales: predefinidos + proyecto + manifiesto único.
    # -------------------------------------------------------------------------

    COMMAND
        $<TARGET_FILE:ILEN_FILE_BINARIZATION>
        --mode project
        --source-assets "${ILEN_PROJECT_DATA_DIR}/assets"
        --source-manifest "${ILEN_PROJECT_ASSET_MANIFEST}"
        --predefined-assets "${ILEN_ENGINE_BINARY_ASSET_DIR}"
        --predefined-manifest "${ILEN_ENGINE_DEFAULT_ASSET_MANIFEST}"
        --output "${ILEN_GAME_BINARY_DATA_DIR}"
        --compile-target "${ILEN_TARGET_NAME}"

    # -------------------------------------------------------------------------
    # Generar escenas y prefabs.
    # -------------------------------------------------------------------------

    COMMAND
        $<TARGET_FILE:ILEN_SCENE_BINARIZER>
        --ilen-folder "${ILEN_PROJECT_SCENE_WORK_ROOT}"

    COMMAND
        "${CMAKE_COMMAND}" -E copy_directory
        "${ILEN_PROJECT_SCENE_WORK_ROOT}/.ilen_bin_data"
        "${ILEN_GAME_BINARY_DATA_DIR}"

    # Persistir el manifiesto editable de escenas en project_data real.
    COMMAND
        "${CMAKE_COMMAND}" -E copy_if_different
        "${ILEN_PROJECT_SCENE_WORK_ROOT}/project_data/manifests/scenes.meta_ilenmanifest"
        "${ILEN_PROJECT_DATA_DIR}/manifests/scenes.meta_ilenmanifest"

    COMMAND
        "${CMAKE_COMMAND}" -E touch
        "${ILEN_GAME_ASSET_STAMP}"

    DEPENDS
        ${ILEN_PROJECT_ASSET_DEPENDENCIES}

    COMMENT
        "Compiling project assets, scenes and predefined engine assets"

    VERBATIM
)

add_custom_target(
    ilen_game_assets
    ALL
    DEPENDS
        "${ILEN_GAME_ASSET_STAMP}"
)

add_dependencies(
    ilen_game_assets
    ilen_engine_assets
)

add_dependencies(
    ${ILEN_GAME_TARGET}
    ilen_game_assets
)

add_custom_target(
    ilen_clean_asset_cache

    COMMAND
        "${CMAKE_COMMAND}" -E rm -rf
        "${ILEN_ENGINE_BINARY_ASSET_DIR}"

    COMMAND
        "${CMAKE_COMMAND}" -E rm -rf
        "${ILEN_GAME_BINARY_DATA_DIR}"

    COMMENT
        "Removing generated engine and game assets"

    VERBATIM
)

# =============================================================================
# Copiar el paquete final al ejecutable o bundle
# =============================================================================

if(CMAKE_SYSTEM_NAME STREQUAL "iOS" OR
   CMAKE_SYSTEM_NAME STREQUAL "tvOS")

    add_custom_command(
        TARGET
            ${ILEN_GAME_TARGET}

        POST_BUILD

        COMMAND
            "${CMAKE_COMMAND}" -E make_directory
            "$<TARGET_FILE_DIR:${ILEN_GAME_TARGET}>/assets"

        COMMAND
            "${CMAKE_COMMAND}" -E copy_directory
            "${ILEN_GAME_BINARY_DATA_DIR}"
            "$<TARGET_FILE_DIR:${ILEN_GAME_TARGET}>/assets"

        COMMENT
            "Copying merged assets into the mobile application bundle"

        VERBATIM
    )

elseif(APPLE)
    add_custom_command(
        TARGET
            ${ILEN_GAME_TARGET}

        POST_BUILD

        COMMAND
            "${CMAKE_COMMAND}" -E make_directory
            "$<TARGET_FILE_DIR:${ILEN_GAME_TARGET}>/../Resources"

        COMMAND
            "${CMAKE_COMMAND}" -E copy_directory
            "${ILEN_GAME_BINARY_DATA_DIR}"
            "$<TARGET_FILE_DIR:${ILEN_GAME_TARGET}>/../Resources"

        COMMENT
            "Copying merged assets into the macOS application bundle"

        VERBATIM
    )

elseif(LINUX)
    add_custom_command(
        TARGET
            ${ILEN_GAME_TARGET}

        POST_BUILD

        COMMAND
            "${CMAKE_COMMAND}" -E make_directory
            "$<TARGET_FILE_DIR:${ILEN_GAME_TARGET}>/Assets"

        COMMAND
            "${CMAKE_COMMAND}" -E copy_directory
            "${ILEN_GAME_BINARY_DATA_DIR}"
            "$<TARGET_FILE_DIR:${ILEN_GAME_TARGET}>/Assets"

        COMMENT
            "Copying merged assets next to the Linux executable"

        VERBATIM
    )

elseif(WIN32)
    add_custom_command(
        TARGET
            ${ILEN_GAME_TARGET}

        POST_BUILD

        COMMAND
            "${CMAKE_COMMAND}" -E make_directory
            "$<TARGET_FILE_DIR:${ILEN_GAME_TARGET}>/Assets"

        COMMAND
            "${CMAKE_COMMAND}" -E copy_directory
            "${ILEN_GAME_BINARY_DATA_DIR}"
            "$<TARGET_FILE_DIR:${ILEN_GAME_TARGET}>/Assets"

        COMMENT
            "Copying merged assets next to the Windows executable"

        VERBATIM
    )
endif()

# =============================================================================
# Ejecución de macOS con Metal HUD
# =============================================================================

if(
    ILEN_METAL_HUD
    AND APPLE
    AND CMAKE_SYSTEM_NAME STREQUAL "Darwin"
)
    add_custom_target(
        run

        COMMAND
            "${CMAKE_COMMAND}" -E env
            MTL_HUD_ENABLED=1
            $<TARGET_FILE:${ILEN_GAME_TARGET}>

        DEPENDS
            ${ILEN_GAME_TARGET}

        WORKING_DIRECTORY
            "${ILEN_PROJECT_ROOT}"

        USES_TERMINAL
    )
endif()