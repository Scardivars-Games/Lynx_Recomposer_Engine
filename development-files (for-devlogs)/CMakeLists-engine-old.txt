# =============================================================================
# Fuentes comunes del motor
# =============================================================================

file(GLOB_RECURSE 
    ILEN_ENGINE_COMMON_SOURCES 
    CONFIGURE_DEPENDS 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Common/*.cpp"
)

set(ILEN_ENGINE_PLATFORM_SOURCES)
set(ILEN_ENGINE_RENDERER_SOURCES)
set(ILEN_ENGINE_PRIVATE_INCLUDE_DIRS)

if(APPLE)
    file(GLOB_RECURSE 
        ILEN_ENGINE_APPLE_SOURCES
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/apple/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/apple/*.mm"
    )
    file(GLOB_RECURSE 
        ILEN_ENGINE_METAL_SOURCES
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/Metal/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/Metal/*.mm"
    )
    list(APPEND ILEN_ENGINE_PLATFORM_SOURCES ${ILEN_ENGINE_APPLE_SOURCES})
    list(APPEND ILEN_ENGINE_RENDERER_SOURCES ${ILEN_ENGINE_METAL_SOURCES})
    list(APPEND ILEN_ENGINE_PRIVATE_INCLUDE_DIRS 
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/apple"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/Metal"
    )

    if(IOS)
        file(GLOB_RECURSE
            ILEN_ENGINE_IOS_SOURCES
            CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/iOS/*.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/iOS/*.mm"
        )
        list(APPEND ILEN_ENGINE_PLATFORM_SOURCES ${ILEN_ENGINE_IOS_SOURCES})
        list(APPEND ILEN_ENGINE_PRIVATE_INCLUDE_DIRS 
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/iOS"
        )
    elseif(TVOS)
        file(GLOB_RECURSE
            ILEN_ENGINE_TVOS_SOURCES
            CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/tvOS/*.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/tvOS/*.mm"
        )
        list(APPEND ILEN_ENGINE_PLATFORM_SOURCES ${ILEN_ENGINE_TVOS_SOURCES})
        list(APPEND ILEN_ENGINE_PRIVATE_INCLUDE_DIRS 
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/tvOS"
        )
    else()
        file(GLOB_RECURSE
            ILEN_ENGINE_MACOS_SOURCES
            CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/macOS/*.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/macOS/*.mm"
        )
        list(APPEND ILEN_ENGINE_PLATFORM_SOURCES ${ILEN_ENGINE_MACOS_SOURCES})
        list(APPEND ILEN_ENGINE_PRIVATE_INCLUDE_DIRS 
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/macOS"
        )
    endif()
elseif(LINUX)
    file(GLOB_RECURSE ILEN_ENGINE_LINUX_SOURCES
        CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/linux/*.cpp"
    )
    file(GLOB_RECURSE ILEN_ENGINE_VULKAN_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/Vulkan/*.cpp"    
    )
    list(APPEND ILEN_ENGINE_PLATFORM_SOURCES ${ILEN_ENGINE_LINUX_SOURCES})
    list(APPEND ILEN_ENGINE_RENDERER_SOURCES ${ILEN_ENGINE_VULKAN_SOURCES})
    list(APPEND ILEN_ENGINE_PRIVATE_INCLUDE_DIRS 
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/linux"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/Vulkan"
    )
endif()

# =============================================================================
# Biblioteca del motor
# =============================================================================

add_library(ilen_engine STATIC)
add_library(ilen::engine ALIAS ilen_engine)

target_sources(ilen_engine PRIVATE 
    ${ILEN_ENGINE_COMMON_SOURCES} 
    ${ILEN_ENGINE_RENDERER_SOURCES} 
    ${ILEN_ENGINE_PLATFORM_SOURCES}
    "${ILEN_GENERATED_SERIALIZERS}"
)

target_include_directories(
    ilen_engine
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE ${ILEN_ENGINE_PRIVATE_INCLUDE_DIRS}
    #Estas 2 abajo son temporales hasta que se ajuste
    "${ILEN_GENERATED_REFLECTION_ROOT}/project_data"
    "${ILEN_PROJECT_ROOT}/include"
)

find_package(Threads REQUIRED)

target_link_libraries(
    ilen_engine PUBLIC ilen::build_config Threads::Threads
)

add_dependencies(ilen_engine ilen_reflection_codegen)

# =============================================================================
# Dependencias de plataforma
# =============================================================================

if(APPLE)
    target_link_libraries(ilen_engine PUBLIC
        "-framework MetalKit"
        "-framework Metal"
        "-framework QuartzCore"
        "-framework GameController"
    )

    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        target_link_libraries(ilen_engine PUBLIC
            "-framework Foundation"
            "-framework UIKit"
        )
        target_compile_options(ilen_engine PRIVATE "-fno-objc-arc")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "tvOS")
        target_link_libraries(ilen_engine PUBLIC
            "-framework Foundation"
            "-framework UIKit"
            "-framework TVUIKit"
        )
        target_compile_options(ilen_engine PRIVATE "-fno-objc-arc")
    else()
        target_link_libraries(ilen_engine PUBLIC "-framework Cocoa")
        target_compile_options(ilen_engine PRIVATE "-fno-objc-arc")
    endif()
elseif(LINUX)
    find_package(PkgConfig REQUIRED)
    find_package(Vulkan REQUIRED)
    pkg_check_modules(ILEN_XCB REQUIRED IMPORTED_TARGET xcb xcb-ewmh xcb-icccm)

    target_link_libraries(ilen_engine PUBLIC Vulkan::Vulkan PRIVATE PkgConfig::ILEN_XCB)
endif()

# =============================================================================
# Pruebas aisladas del motor
# =============================================================================

if(ILEN_BUILD_TESTS OR ILEN_BUILD_PHYSICS_BENCHMARK)
    add_executable(
        ilen-physics-broadphase-benchmark

        "${CMAKE_CURRENT_SOURCE_DIR}/tests/physics/broad_phase_2d_benchmark.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Common/physics/broad_phase_2d.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Common/maths/AABB.cpp"
    )

    target_link_libraries(
        ilen-physics-broadphase-benchmark
        PRIVATE
            ilen::build_config
    )

    target_include_directories(
        ilen-physics-broadphase-benchmark
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )

    add_executable(
        ilen-physics-swept-sat-test

        "${CMAKE_CURRENT_SOURCE_DIR}/tests/physics/swept_sat_2d_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Common/physics/narrow_phase_2d.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Common/maths/AABB.cpp"
    )

    target_link_libraries(
        ilen-physics-swept-sat-test
        PRIVATE
            ilen::build_config
    )

    target_include_directories(
        ilen-physics-swept-sat-test
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )

    add_executable(
        ilen-physics-collider-proxy-test

        "${CMAKE_CURRENT_SOURCE_DIR}/tests/physics/collider_proxy_builder_2d_test.cpp"
    )

    target_link_libraries(
        ilen-physics-collider-proxy-test
        PRIVATE
            ilen::build_config
    )

    target_include_directories(
        ilen-physics-collider-proxy-test
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
endif()

if(ILEN_BUILD_TESTS)
    add_executable(
        ilen-command-buffer-concurrency-test

        "${CMAKE_CURRENT_SOURCE_DIR}/tests/command_buffer_concurrency.cpp"
    )

    target_link_libraries(
        ilen-command-buffer-concurrency-test
        PRIVATE
            ilen::build_config
            Threads::Threads
    )

    target_include_directories(
        ilen-command-buffer-concurrency-test
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
endif()

# =============================================================================
# Assets binarios predefinidos
# =============================================================================

file(GLOB_RECURSE ILEN_ENGINE_ASSET_INPUTS CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    "${ILEN_ENGINE_DATA_DIR}/assets/*"
    "${ILEN_ENGINE_DATA_DIR}/manifests/assets_defaults.meta_ilenmanifest"
)

list(FILTER ILEN_ENGINE_ASSET_INPUTS EXCLUDE REGEX "/\\.DS_Store$")
set(ILEN_ENGINE_ASSET_WORK_ROOT "${ILEN_GENERATED_ROOT}/engine_asset_compiler")
set(
    ILEN_ENGINE_DEFAULT_MANIFEST
    "${ILEN_ENGINE_DATA_DIR}/manifests/assets_defaults.meta_ilenmanifest"
)

set(
    ILEN_ENGINE_ASSET_COMMANDS

    COMMAND
        "${CMAKE_COMMAND}" -E rm -rf
        "${ILEN_ENGINE_ASSET_WORK_ROOT}"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${ILEN_ENGINE_ASSET_WORK_ROOT}/project_data/assets"

    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${ILEN_ENGINE_ASSET_WORK_ROOT}/include/ilen/render"

    COMMAND
        "${CMAKE_COMMAND}" -E copy_directory
        "${ILEN_ENGINE_DATA_DIR}/assets"
        "${ILEN_ENGINE_ASSET_WORK_ROOT}/project_data/assets"

    COMMAND
        "${CMAKE_COMMAND}" -E copy_if_different
        "${ILEN_ENGINE_ROOT}/include/ilen/render/render_constants.h"
        "${ILEN_ENGINE_ASSET_WORK_ROOT}/include/ilen/render/render_constants.h"

    COMMAND
        $<TARGET_FILE:ILEN_FILE_BINARIZATION>
        --mode engine
        --source-assets "${ILEN_ENGINE_DATA_DIR}/assets"
        --source-manifest "${ILEN_ENGINE_DEFAULT_MANIFEST}"
        --output "${ILEN_ENGINE_BINARY_ASSET_DIR}"
        --compile-target "${ILEN_TARGET_NAME}"
)

if(APPLE)
    list(
        APPEND
        ILEN_ENGINE_ASSET_COMMANDS

        COMMAND
            $<TARGET_FILE:ILEN_METAL_COMPILER>
            --ilen-folder "${ILEN_ENGINE_ASSET_WORK_ROOT}"
            --sdk "${ILEN_METAL_SDK}"
            $<$<CONFIG:Debug>:--add-debug>

        COMMAND
            "${CMAKE_COMMAND}" -E copy_directory
            "${ILEN_ENGINE_ASSET_WORK_ROOT}/.ilen_bin_data/shader-code"
            "${ILEN_ENGINE_BINARY_ASSET_DIR}/shader-code"
    )
endif()

add_custom_command(
    OUTPUT
        "${ILEN_ENGINE_ASSET_STAMP}"

    ${ILEN_ENGINE_ASSET_COMMANDS}

    COMMAND
        "${CMAKE_COMMAND}" -E touch
        "${ILEN_ENGINE_ASSET_STAMP}"

    DEPENDS
        ${ILEN_ENGINE_ASSET_INPUTS}
        ILEN_FILE_BINARIZATION
        $<$<BOOL:${APPLE}>:ILEN_METAL_COMPILER>

    COMMENT
        "Compiling Ilusions Engine predefined assets"

    COMMAND_EXPAND_LISTS
    VERBATIM
)

add_custom_target(
    ilen_engine_assets
    DEPENDS
        "${ILEN_ENGINE_ASSET_STAMP}"
)