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
# Paquete fusionado de assets
#
# FileManager actualmente solo reconoce un directorio de recursos.
#
# Por ello se crea este staging:
#
#   build/ilen-generated/game-assets/
#
# Orden:
#
#   1. Assets del motor.
#   2. Assets del proyecto.
#   3. Schemas generados.
#   4. Escenas.
#   5. Shaders.
#   6. Binarización general.
#
# El proyecto se copia después para permitir overrides intencionales.
# =============================================================================

set(
    ILEN_GAME_ASSET_STAGING_ROOT
    "${ILEN_GENERATED_ROOT}/game_assets"
)

set(
    ILEN_GAME_BINARY_DATA_DIR
    "${ILEN_GAME_ASSET_STAGING_ROOT}/.ilen-bin-data"
)

set(
    ILEN_GAME_ASSET_STAMP
    "${ILEN_GAME_BINARY_DATA_DIR}/.build_complete"
)

# =============================================================================
# Entradas que pueden invalidar el paquete de assets
# =============================================================================

file(
    GLOB_RECURSE
    ILEN_ENGINE_ASSET_INPUTS
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false

    "${ILEN_ENGINE_DATA_DIR}/assets/*"
    "${ILEN_ENGINE_DATA_DIR}/manifests/*"
)

file(
    GLOB_RECURSE
    ILEN_PROJECT_ASSET_INPUTS
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false

    "${ILEN_PROJECT_DATA_DIR}/assets/*"
    "${ILEN_PROJECT_DATA_DIR}/scenes/*"
    "${ILEN_PROJECT_DATA_DIR}/manifests/*"
)

# Los shaders Metal actuales solo incluyen este header.
set(
    ILEN_SHADER_HEADER_INPUTS
    "${ILEN_ENGINE_ROOT}/include/ilen/render/render_constants.h"
)

set(
    ILEN_ALL_ASSET_INPUTS
    ${ILEN_ENGINE_ASSET_INPUTS}
    ${ILEN_PROJECT_ASSET_INPUTS}
    ${ILEN_SHADER_HEADER_INPUTS}
)

# Evitar que modificaciones de Finder invaliden los assets.
list(
    FILTER
    ILEN_ALL_ASSET_INPUTS
    EXCLUDE
    REGEX "/\\.DS_Store$"
)

list(SORT ILEN_ALL_ASSET_INPUTS)

# Este inventario permite detectar assets añadidos o eliminados.
# file(GENERATE) no cambia el timestamp si el contenido sigue igual.
string(
    JOIN
    "\n"
    ILEN_ASSET_INVENTORY_CONTENT
    ${ILEN_ALL_ASSET_INPUTS}
)

set(
    ILEN_ASSET_INVENTORY_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/ilen_asset_inventory.txt"
)

file(
    GENERATE
    OUTPUT
        "${ILEN_ASSET_INVENTORY_FILE}"
    CONTENT
        "${ILEN_ASSET_INVENTORY_CONTENT}\n"
)

# =============================================================================
# Generación incremental
#
# Se elimina solamente la copia de las fuentes del staging.
# No se elimina .ilen-bin-data.
# =============================================================================

if(APPLE)
    add_custom_command(
        OUTPUT
            "${ILEN_GAME_ASSET_STAMP}"

        # Limpiar únicamente las entradas copiadas.
        # Los binarios pesados permanecen en .ilen-bin-data.
        COMMAND
            "${CMAKE_COMMAND}" -E rm -rf
            "${ILEN_GAME_ASSET_STAGING_ROOT}/include"

        COMMAND
            "${CMAKE_COMMAND}" -E rm -rf
            "${ILEN_GAME_ASSET_STAGING_ROOT}/project_data"

        COMMAND
            "${CMAKE_COMMAND}" -E make_directory
            "${ILEN_GAME_ASSET_STAGING_ROOT}/include/ilen/render"

        COMMAND
            "${CMAKE_COMMAND}" -E make_directory
            "${ILEN_GAME_ASSET_STAGING_ROOT}/project_data/assets"

        COMMAND
            "${CMAKE_COMMAND}" -E make_directory
            "${ILEN_GAME_ASSET_STAGING_ROOT}/project_data/scenes"

        COMMAND
            "${CMAKE_COMMAND}" -E make_directory
            "${ILEN_GAME_ASSET_STAGING_ROOT}/project_data/manifests"

        COMMAND
            "${CMAKE_COMMAND}" -E make_directory
            "${ILEN_GAME_BINARY_DATA_DIR}"

        # El compilador Metal actual busca:
        # <ilen-folder>/include/ilen/render/render_constants.h
        COMMAND
            "${CMAKE_COMMAND}" -E copy_if_different
            "${ILEN_ENGINE_ROOT}/include/ilen/render/render_constants.h"
            "${ILEN_GAME_ASSET_STAGING_ROOT}/include/ilen/render/render_constants.h"

        # Assets predeterminados del motor.
        COMMAND
            "${CMAKE_COMMAND}" -E copy_directory
            "${ILEN_ENGINE_DATA_DIR}/assets"
            "${ILEN_GAME_ASSET_STAGING_ROOT}/project_data/assets"

        COMMAND
            "${CMAKE_COMMAND}" -E copy_directory
            "${ILEN_ENGINE_DATA_DIR}/manifests"
            "${ILEN_GAME_ASSET_STAGING_ROOT}/project_data/manifests"

        # Assets y datos propios del proyecto.
        COMMAND
            "${CMAKE_COMMAND}" -E copy_directory
            "${ILEN_PROJECT_DATA_DIR}/assets"
            "${ILEN_GAME_ASSET_STAGING_ROOT}/project_data/assets"

        COMMAND
            "${CMAKE_COMMAND}" -E copy_directory
            "${ILEN_PROJECT_DATA_DIR}/scenes"
            "${ILEN_GAME_ASSET_STAGING_ROOT}/project_data/scenes"

        COMMAND
            "${CMAKE_COMMAND}" -E copy_directory
            "${ILEN_PROJECT_DATA_DIR}/manifests"
            "${ILEN_GAME_ASSET_STAGING_ROOT}/project_data/manifests"

        # Schemas generados en esta build.
        COMMAND
            "${CMAKE_COMMAND}" -E copy_if_different
            "${ILEN_GENERATED_ENGINE_SCHEMA}"
            "${ILEN_GAME_ASSET_STAGING_ROOT}/project_data/manifests/engine_defaults.meta_ilenschema"

        COMMAND
            "${CMAKE_COMMAND}" -E copy_if_different
            "${ILEN_GENERATED_COMPONENT_SCHEMA}"
            "${ILEN_GAME_ASSET_STAGING_ROOT}/project_data/manifests/components.meta_ilenschema"

        # Generación de escenas y prefabs.
        COMMAND
            $<TARGET_FILE:ILEN_SCENE_BINARIZER>
            --ilen-folder "${ILEN_GAME_ASSET_STAGING_ROOT}"

        # Compilación de Metal.
        COMMAND
            $<TARGET_FILE:ILEN_METAL_COMPILER>
            --ilen-folder "${ILEN_GAME_ASSET_STAGING_ROOT}"
            --sdk "${ILEN_METAL_SDK}"
            $<$<CONFIG:Debug>:--add-debug>

        # Binarización general.
        COMMAND
            $<TARGET_FILE:ILEN_FILE_BINARIZATION>
            --ilen-folder "${ILEN_GAME_ASSET_STAGING_ROOT}"
            --compile-target "${ILEN_TARGET_NAME}"

        # Solo se crea si todas las operaciones anteriores terminaron bien.
        COMMAND
            "${CMAKE_COMMAND}" -E touch
            "${ILEN_GAME_ASSET_STAMP}"

        DEPENDS
            ${ILEN_ALL_ASSET_INPUTS}
            "${ILEN_ASSET_INVENTORY_FILE}"
            "${ILEN_GENERATED_ENGINE_SCHEMA}"
            "${ILEN_GENERATED_COMPONENT_SCHEMA}"

            ILEN_SCENE_BINARIZER
            ILEN_METAL_COMPILER
            ILEN_FILE_BINARIZATION

        COMMENT
            "Updating merged engine and project asset package"

        COMMAND_EXPAND_LISTS
        VERBATIM
    )

else()
    add_custom_command(
        OUTPUT
            "${ILEN_GAME_ASSET_STAMP}"

        COMMAND
            "${CMAKE_COMMAND}" -E rm -rf
            "${ILEN_GAME_ASSET_STAGING_ROOT}/project_data"

        COMMAND
            "${CMAKE_COMMAND}" -E make_directory
            "${ILEN_GAME_ASSET_STAGING_ROOT}/project_data/assets"

        COMMAND
            "${CMAKE_COMMAND}" -E make_directory
            "${ILEN_GAME_ASSET_STAGING_ROOT}/project_data/scenes"

        COMMAND
            "${CMAKE_COMMAND}" -E make_directory
            "${ILEN_GAME_ASSET_STAGING_ROOT}/project_data/manifests"

        COMMAND
            "${CMAKE_COMMAND}" -E make_directory
            "${ILEN_GAME_BINARY_DATA_DIR}"

        COMMAND
            "${CMAKE_COMMAND}" -E copy_directory
            "${ILEN_ENGINE_DATA_DIR}/assets"
            "${ILEN_GAME_ASSET_STAGING_ROOT}/project_data/assets"

        COMMAND
            "${CMAKE_COMMAND}" -E copy_directory
            "${ILEN_ENGINE_DATA_DIR}/manifests"
            "${ILEN_GAME_ASSET_STAGING_ROOT}/project_data/manifests"

        COMMAND
            "${CMAKE_COMMAND}" -E copy_directory
            "${ILEN_PROJECT_DATA_DIR}/assets"
            "${ILEN_GAME_ASSET_STAGING_ROOT}/project_data/assets"

        COMMAND
            "${CMAKE_COMMAND}" -E copy_directory
            "${ILEN_PROJECT_DATA_DIR}/scenes"
            "${ILEN_GAME_ASSET_STAGING_ROOT}/project_data/scenes"

        COMMAND
            "${CMAKE_COMMAND}" -E copy_directory
            "${ILEN_PROJECT_DATA_DIR}/manifests"
            "${ILEN_GAME_ASSET_STAGING_ROOT}/project_data/manifests"

        COMMAND
            "${CMAKE_COMMAND}" -E copy_if_different
            "${ILEN_GENERATED_ENGINE_SCHEMA}"
            "${ILEN_GAME_ASSET_STAGING_ROOT}/project_data/manifests/engine_defaults.meta_ilenschema"

        COMMAND
            "${CMAKE_COMMAND}" -E copy_if_different
            "${ILEN_GENERATED_COMPONENT_SCHEMA}"
            "${ILEN_GAME_ASSET_STAGING_ROOT}/project_data/manifests/components.meta_ilenschema"

        COMMAND
            $<TARGET_FILE:ILEN_SCENE_BINARIZER>
            --ilen-folder "${ILEN_GAME_ASSET_STAGING_ROOT}"

        COMMAND
            $<TARGET_FILE:ILEN_FILE_BINARIZATION>
            --ilen-folder "${ILEN_GAME_ASSET_STAGING_ROOT}"
            --compile-target "${ILEN_TARGET_NAME}"

        COMMAND
            "${CMAKE_COMMAND}" -E touch
            "${ILEN_GAME_ASSET_STAMP}"

        DEPENDS
            ${ILEN_ALL_ASSET_INPUTS}
            "${ILEN_ASSET_INVENTORY_FILE}"
            "${ILEN_GENERATED_ENGINE_SCHEMA}"
            "${ILEN_GENERATED_COMPONENT_SCHEMA}"

            ILEN_SCENE_BINARIZER
            ILEN_FILE_BINARIZATION

        COMMENT
            "Updating merged engine and project asset package"

        VERBATIM
    )
endif()

# Este target pertenece a ALL, pero no tiene comandos propios.
# Solo solicita el sello. Si está actualizado, no hace absolutamente nada.
add_custom_target(
    ilen_game_assets
    ALL
    DEPENDS
        "${ILEN_GAME_ASSET_STAMP}"
)

add_dependencies(
    ${ILEN_GAME_TARGET}
    ilen_game_assets
)

# Limpieza explícita de la caché.
# Nunca se ejecuta automáticamente.
add_custom_target(
    ilen_clean_asset_cache

    COMMAND
        "${CMAKE_COMMAND}" -E rm -rf
        "${ILEN_GAME_ASSET_STAGING_ROOT}"

    COMMENT
        "Removing the complete generated asset cache"

    VERBATIM
)

add_dependencies(
    ${ILEN_GAME_TARGET}
    ilen_game_assets
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