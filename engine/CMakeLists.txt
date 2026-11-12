cmake_minimum_required(VERSION 4.0)

project(LynxRecomposerEngine VERSION 0.1.0 LANGUAGES C CXX)

if(APPLE)
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#Engine options

option(LRE_ONLY_ECS "Use LRE exclusively through ECS" OFF)
option(LRE_BUILD_TESTS "Build LRE tests" OFF)
option(LRE_BUILD_PHYSICS_BENCHMARKS "Build physics benchmarks" OFF)
option(LRE_BUILD_ENGINE_ASSETS "Create the engine asset target" ON)
option(LRE_ENGINE_STATIC "Build the engine as a static library" OFF)
set(LRE_ENGINE_XCFRAMEWORK "" CACHE PATH "Prebuilt LynxRecomposer.xcframework used by an iOS game build")

option(LRE_ENABLE_PHYSICS_DEBUG "Enable physics debug rendering" ON)
option(LRE_ENABLE_PHYSICS_STATS "Enable physics statistics" ON)
option(LRE_IGNORE_RENDERING "Disable rendering" OFF)
option(LRE_TEST_ERROR_WINDOW_AT_START "Show an error window at startup" OFF)
option(LRE_SINGLECORE_TEST "Force the job system to one thread" OFF)
option(LRE_DISABLE_BASIC_LOGGING "Disable basic logging" OFF)
option(LRE_DISABLE_TIME_LOGGING "Disable automatic time logging" ON)
option(
    LYNX_OPTIMIZED_BUILD_FLAGS
    "Enable aggressive optimization flags for Release and RelWithDebInfo builds"
    OFF
)

#Configuracion compartida con el juego

add_library(lre_build_config INTERFACE)
add_library(Lynx::BuildConfig ALIAS lre_build_config)

target_compile_features(lre_build_config INTERFACE cxx_std_23)
target_compile_definitions(lre_build_config INTERFACE
    LYNX_ONLY_ECS=$<BOOL:${LRE_ONLY_ECS}>
    LYNX_ENABLE_PHYSICS_DEBUG=$<BOOL:${LRE_ENABLE_PHYSICS_DEBUG}>
    LYNX_ENABLE_PHYSICS_STATS=$<BOOL:${LRE_ENABLE_PHYSICS_STATS}>

    IGNORE_RENDERING=$<BOOL:${LRE_IGNORE_RENDERING}>
    TEST_ERROR_WINDOW_AT_START=$<BOOL:${LRE_TEST_ERROR_WINDOW_AT_START}>
    LYNX_SINGLECORE_TEST=$<BOOL:${LRE_SINGLECORE_TEST}>
    LYNX_DISABLE_BASIC_LOGGIN=$<BOOL:${LRE_DISABLE_BASIC_LOGGING}>
    LYNX_DISABLE_TIME_LOGGING=$<BOOL:${LRE_DISABLE_TIME_LOGGING}>

    $<$<CONFIG:Debug>:LYNX_DEBUG=1>
)

# Optimizaciones agresivas opcionales para las configuraciones optimizadas.
# Se aplican mediante Lynx::BuildConfig para que alcancen tanto al motor como
# al proyecto que lo consume, incluso con generadores multiconfiguracion.
if(LYNX_OPTIMIZED_BUILD_FLAGS)
    set(LRE_OPTIMIZED_BUILD_CONFIG
        "$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>"
    )

    if(MSVC)
        target_compile_options(lre_build_config INTERFACE
            "$<${LRE_OPTIMIZED_BUILD_CONFIG}:/O2>"
            "$<${LRE_OPTIMIZED_BUILD_CONFIG}:/fp:fast>"
        )

        if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$")
            target_compile_options(lre_build_config INTERFACE
                "$<${LRE_OPTIMIZED_BUILD_CONFIG}:/arch:AVX2>"
            )
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
            target_compile_options(lre_build_config INTERFACE
                "$<${LRE_OPTIMIZED_BUILD_CONFIG}:/arch:arm64>"
            )
        endif()
    else()
        target_compile_options(lre_build_config INTERFACE
            "$<${LRE_OPTIMIZED_BUILD_CONFIG}:-O3>"
            "$<${LRE_OPTIMIZED_BUILD_CONFIG}:-ffast-math>"
            "$<${LRE_OPTIMIZED_BUILD_CONFIG}:-funroll-loops>"
            "$<${LRE_OPTIMIZED_BUILD_CONFIG}:-fomit-frame-pointer>"
            "$<$<AND:${LRE_OPTIMIZED_BUILD_CONFIG},$<CXX_COMPILER_ID:Clang,AppleClang>>:-fstrict-vtable-pointers>"
        )

        if(NOT LRE_ENGINE_STATIC)
            target_compile_options(lre_build_config INTERFACE
                "$<$<AND:${LRE_OPTIMIZED_BUILD_CONFIG},$<CXX_COMPILER_ID:Clang,AppleClang>>:-flto=thin>"
                "$<$<AND:${LRE_OPTIMIZED_BUILD_CONFIG},$<CXX_COMPILER_ID:GNU>>:-flto=auto>"
            )
        endif()

        if(CMAKE_SYSTEM_NAME MATCHES "iOS|tvOS" AND NOT PLATFORM STREQUAL "SIMULATOR")
            target_compile_options(lre_build_config INTERFACE
                "$<${LRE_OPTIMIZED_BUILD_CONFIG}:-mcpu=apple-a13>"
            )
        elseif(APPLE)
            if(NOT ("arm64" IN_LIST CMAKE_OSX_ARCHITECTURES AND
                    "x86_64" IN_LIST CMAKE_OSX_ARCHITECTURES))
                if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
                    target_compile_options(lre_build_config INTERFACE
                        "$<${LRE_OPTIMIZED_BUILD_CONFIG}:-mcpu=apple-m1>"
                    )
                elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$")
                    target_compile_options(lre_build_config INTERFACE
                        "$<${LRE_OPTIMIZED_BUILD_CONFIG}:-march=x86-64-v3>"
                    )
                endif()
            endif()
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
            target_compile_options(lre_build_config INTERFACE
                "$<${LRE_OPTIMIZED_BUILD_CONFIG}:-march=armv8-a>"
            )
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(armv7|armhf|arm)$")
            target_compile_options(lre_build_config INTERFACE
                "$<${LRE_OPTIMIZED_BUILD_CONFIG}:-march=armv7-a>"
                "$<${LRE_OPTIMIZED_BUILD_CONFIG}:-mfpu=neon>"
            )
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$")
            target_compile_options(lre_build_config INTERFACE
                "$<${LRE_OPTIMIZED_BUILD_CONFIG}:-march=x86-64-v3>"
            )
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(i[3-6]86|x86)$")
            target_compile_options(lre_build_config INTERFACE
                "$<${LRE_OPTIMIZED_BUILD_CONFIG}:-march=i686>"
            )
        else()
            message(WARNING
                "LYNX_OPTIMIZED_BUILD_FLAGS: arquitectura desconocida "
                "(${CMAKE_SYSTEM_PROCESSOR}); no se agregaran flags de CPU"
            )
        endif()
    endif()

    target_compile_definitions(lre_build_config INTERFACE
        "$<${LRE_OPTIMIZED_BUILD_CONFIG}:NDEBUG>"
    )

    if(NOT LRE_ENGINE_STATIC)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
    endif()

    message(STATUS
        "LYNX_OPTIMIZED_BUILD_FLAGS habilitado para Release y RelWithDebInfo"
    )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_compile_definitions(lre_build_config INTERFACE LYNX_MOBILE=1 LYNX_IOS=1)
elseif(CMAKE_SYSTEM_NAME STREQUAL "tvOS")
    target_compile_definitions(lre_build_config INTERFACE LYNX_MOBILE=1 LYNX_TVOS=1)
elseif(APPLE)
    target_compile_definitions(lre_build_config INTERFACE LYNX_DESKTOP=1 LYNX_MACOS=1)
elseif(LINUX)
    target_compile_definitions(lre_build_config INTERFACE LYNX_DESKTOP=1 LYNX_LINUX=1 VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1)
elseif(WIN32)
    target_compile_definitions(lre_build_config INTERFACE LYNX_DESKTOP=2 LYNX_WINDOWS=1 VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1)
endif()


#Fuentes comunes

file(GLOB_RECURSE LRE_COMMON_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/Common/*.cpp")
set(LRE_PLATFORM_SOURCES)
set(LRE_RENDERER_SOURCES)
set(LRE_PRIVATE_INCLUDE_DIRS)

#Fuentes por plataforma

if(APPLE)
    file(GLOB_RECURSE LRE_APPLE_COMMON_SOURCES
        CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/apple/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/apple/*.mm"
    )

    file(GLOB_RECURSE LRE_METAL_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/Metal/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/Metal/*.mm"
    )

    list(APPEND LRE_PLATFORM_SOURCES ${LRE_APPLE_COMMON_SOURCES})
    list(APPEND LRE_RENDERER_SOURCES ${LRE_METAL_SOURCES})
    list(APPEND LRE_PRIVATE_INCLUDE_DIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/apple" "${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/Metal"
    )

    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        file(GLOB_RECURSE LRE_TARGET_PLATFORM_SOURCES CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/iOS/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/iOS/*.mm"
        )
        list(APPEND LRE_PRIVATE_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/iOS")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "tvOS")
        file(GLOB_RECURSE LRE_TARGET_PLATFORM_SOURCES CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/tvOS/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/tvOS/*.mm"
        )
        list(APPEND LRE_PRIVATE_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/tvOS")
    else()
        file(GLOB_RECURSE LRE_TARGET_PLATFORM_SOURCES CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/macOS/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/macOS/*.mm"
        )
        list(APPEND LRE_PRIVATE_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/macOS")
    endif()
    list(APPEND LRE_PLATFORM_SOURCES ${LRE_TARGET_PLATFORM_SOURCES})

elseif(LINUX)
    file(GLOB_RECURSE LRE_LINUX_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/linux/*.cpp"
    )
    file(GLOB_RECURSE LRE_VULKAN_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/Vulkan/*.cpp"
    )

    list(APPEND LRE_PLATFORM_SOURCES ${LRE_LINUX_SOURCES})
    list(APPEND LRE_RENDERER_SOURCES ${LRE_VULKAN_SOURCES})
    list(APPEND LRE_PRIVATE_INCLUDE_DIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/linux"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/Vulkan"
    )
endif()

# Creación o importación de la biblioteca del engine
if(LRE_ENGINE_STATIC)
    set(LRE_ENGINE_LIBRARY_TYPE STATIC)
else()
    set(LRE_ENGINE_LIBRARY_TYPE SHARED)
endif()

if(LRE_ENGINE_XCFRAMEWORK)
    if(NOT CMAKE_SYSTEM_NAME MATCHES "^(iOS|tvOS)$")
        message(FATAL_ERROR "LRE_ENGINE_XCFRAMEWORK is only supported for iOS and tvOS builds")
    endif()
    if(NOT EXISTS "${LRE_ENGINE_XCFRAMEWORK}/Info.plist")
        message(FATAL_ERROR "Invalid XCFramework: ${LRE_ENGINE_XCFRAMEWORK}")
    endif()

    add_library(lre_engine INTERFACE)
    target_link_libraries(lre_engine INTERFACE
        Lynx::BuildConfig
        "${LRE_ENGINE_XCFRAMEWORK}"
    )
else()
    add_library(lre_engine ${LRE_ENGINE_LIBRARY_TYPE}
        ${LRE_COMMON_SOURCES}
        ${LRE_PLATFORM_SOURCES}
        ${LRE_RENDERER_SOURCES}
    )
endif()
add_library(Lynx::Engine ALIAS lre_engine)

if(NOT LRE_ENGINE_XCFRAMEWORK)
    set_target_properties(lre_engine PROPERTIES 
        OUTPUT_NAME "LynxRecomposer"
        #VERSION "${PROJECT_VERSION}"
        #SOVERSION $"{PROJECT_VERSION_MAJOR}"
        #Solucion inicial para exportar la API de windows
        WINDOWS_EXPORT_ALL_SYMBOLS TRUE
        EXPORT_NAME Engine
    )

    target_include_directories(lre_engine
        PUBLIC
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
            "$<INSTALL_INTERFACE:include>"
        PRIVATE
            ${LRE_PRIVATE_INCLUDE_DIRS}
    )

    find_package(Threads REQUIRED)

    target_link_libraries(lre_engine PUBLIC Lynx::BuildConfig Threads::Threads)

    #Dependencias por sistema
    if(APPLE)
        target_link_libraries(lre_engine PUBLIC
            "-framework MetalKit"
            "-framework Metal"
            "-framework QuartzCore"
            "-framework GameController"
        )

        if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
            target_link_libraries(lre_engine PUBLIC
                "-framework Foundation"
                "-framework UIKit"
            )
        elseif(CMAKE_SYSTEM_NAME STREQUAL "tvOS")
            target_link_libraries(lre_engine PUBLIC
                "-framework Foundation"
                "-framework UIKit"
                "-framework TVUIKit"
            )
        else()
            target_link_libraries(lre_engine PUBLIC "-framework Cocoa")
        endif()

        target_compile_options(lre_engine PRIVATE "-fno-objc-arc")

    elseif(LINUX)
        find_package(PkgConfig REQUIRED)
        find_package(Vulkan REQUIRED)

        pkg_check_modules(LRE_XCB REQUIRED IMPORTED_TARGET xcb xcb-ewmh xcb-icccm)
        target_link_libraries(lre_engine PUBLIC Vulkan::Vulkan PRIVATE PkgConfig::LRE_XCB)
    endif()
endif()

#Compilacion de los assets propios del motor

if(LRE_BUILD_ENGINE_ASSETS)
    foreach(LRE_REQUIRED_TOOL LYNX_RE_FILE_BINARIZER LYNX_RE_COMPONENTS_REFLECTION_TOOL)
        if(NOT TARGET ${LRE_REQUIRED_TOOL})
            message(FATAL_ERROR "LRE_BUILD_ENGINE_ASSETS requiere el target ${LRE_REQUIRED_TOOL}")
        endif()
    endforeach()
    
    if(APPLE AND NOT TARGET LYNX_RE_METAL_COMPILER)
        message(FATAL_ERROR "Los assets de Apple requieren LYNX_RE_METAL_COMPILER")
    endif()

    if(NOT LRE_ENGINE_BINARY_ASSET_DIR)
        set(LRE_ENGINE_BINARY_ASSET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/generated_assets" CACHE PATH "Assets compilados de Lynx Recomposer Engine")
    endif()

    if(NOT LRE_ENGINE_SCHEMA_FILE)
        set(LRE_ENGINE_SCHEMA_FILE "${CMAKE_CURRENT_BINARY_DIR}/generated_schema/engine_defaults.meta_lynxschema" CACHE PATH "Esquema de componentes de Lynx Recomposer Engine")
    endif()

    set(LRE_ENGINE_DEFAULT_ASSET_MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/project_data/manifests/assets_defaults.meta_lynxmanifest")
    set(LRE_ENGINE_ASSET_STAMP "${LRE_ENGINE_BINARY_ASSET_DIR}/.build_complete")
    set(LRE_ENGINE_ASSET_WORK_ROOT "${CMAKE_CURRENT_BINARY_DIR}/engine_asset_work")

    file(GLOB_RECURSE LRE_ENGINE_REFLECTION_INPUTS CONFIGURE_DEPENDS LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/include/lynx/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/include/lynx/*.hpp")
    file(GLOB_RECURSE LRE_ENGINE_ASSET_INPUTS CONFIGURE_DEPENDS LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/project_data/assets/*")
    list(APPEND LRE_ENGINE_ASSET_INPUTS ${LRE_ENGINE_DEFAULT_ASSET_MANIFEST})
    list(FILTER LRE_ENGINE_ASSET_INPUTS EXCLUDE REGEX "/\\.DS_Store$")

    set(LRE_ENGINE_ASSET_DEPENDENCIES ${LRE_ENGINE_ASSET_INPUTS} ${LRE_ENGINE_REFLECTION_INPUTS} LYNX_RE_FILE_BINARIZER LYNX_RE_COMPONENTS_REFLECTION_TOOL)
    set(LRE_ENGINE_ASSET_COMMANDS
    
        COMMAND
            "${CMAKE_COMMAND}" -E rm -rf
            "${LRE_ENGINE_ASSET_WORK_ROOT}"
        
        COMMAND
            "${CMAKE_COMMAND}" -E make_directory
            "${LRE_ENGINE_ASSET_WORK_ROOT}/include"

        COMMAND
            "${CMAKE_COMMAND}" -E make_directory
            "${LRE_ENGINE_ASSET_WORK_ROOT}/project_data/manifests"

        COMMAND
            "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/include/lynx"
            "${LRE_ENGINE_ASSET_WORK_ROOT}/include/lynx"
        
        COMMAND
            $<TARGET_FILE:LYNX_RE_COMPONENTS_REFLECTION_TOOL>
            --engine-mode
            --lynx-folder "${LRE_ENGINE_ASSET_WORK_ROOT}"
        
        COMMAND
            "${CMAKE_COMMAND}" -E make_directory
            "$<PATH:GET_PARENT_PATH,${LRE_ENGINE_SCHEMA_FILE}>"

        COMMAND
            "${CMAKE_COMMAND}" -E copy_if_different
            "${LRE_ENGINE_ASSET_WORK_ROOT}/project_data/manifests/engine_defaults.meta_lynxschema"
            "${LRE_ENGINE_SCHEMA_FILE}"

        COMMAND
            $<TARGET_FILE:LYNX_RE_FILE_BINARIZER>
            --mode engine
            --source-assets "${CMAKE_CURRENT_SOURCE_DIR}/project_data/assets"
            --source-manifest "${LRE_ENGINE_DEFAULT_ASSET_MANIFEST}"
            --output "${LRE_ENGINE_BINARY_ASSET_DIR}"
            --compile-target "${LRE_ASSET_TARGET_NAME}"
    )

    if(APPLE)
        list(APPEND LRE_ENGINE_ASSET_DEPENDENCIES LYNX_RE_METAL_COMPILER)

        if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
            if(PLATFORM STREQUAL "SIMULATOR")
                set(LRE_METAL_SDK "iphonesimulator")
            else()
                set(LRE_METAL_SDK "iphoneos")
            endif()
        elseif(CMAKE_SYSTEM_NAME STREQUAL "tvOS")
            if(PLATFORM STREQUAL "SIMULATOR")
                set(LRE_METAL_SDK "appletvsimulator")
            else()
                set(LRE_METAL_SDK "appletvos")
            endif()
        else()
            set(LRE_METAL_SDK "macosx")
        endif()

        list(APPEND LRE_ENGINE_ASSET_COMMANDS
        
            COMMAND
                "${CMAKE_COMMAND}" -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/project_data/assets"
                "${LRE_ENGINE_ASSET_WORK_ROOT}/project_data/assets"

            COMMAND
                $<TARGET_FILE:LYNX_RE_METAL_COMPILER>
                --lynx-folder "${LRE_ENGINE_ASSET_WORK_ROOT}"
                --sdk "${LRE_METAL_SDK}"
                $<$<CONFIG:Debug>:--add-debug>

            COMMAND
                "${CMAKE_COMMAND}" -E copy_directory
                "${LRE_ENGINE_ASSET_WORK_ROOT}/.lynx_bin_data/shader_code"
                "${LRE_ENGINE_BINARY_ASSET_DIR}/shader_code"
        )
    endif()

    add_custom_command(
        OUTPUT
            "${LRE_ENGINE_ASSET_STAMP}"
            "${LRE_ENGINE_SCHEMA_FILE}"

        ${LRE_ENGINE_ASSET_COMMANDS}

        COMMAND
            "${CMAKE_COMMAND}" -E touch
            "${LRE_ENGINE_ASSET_STAMP}"
        
        DEPENDS
            ${LRE_ENGINE_ASSET_DEPENDENCIES}
        
        COMMENT
            "Compiling Lynx Recomposer Engine assets"
        
        COMMAND_EXPAND_LISTS
        VERBATIM
    )

    add_custom_target(lre_engine_assets DEPENDS "${LRE_ENGINE_ASSET_STAMP}" "${LRE_ENGINE_SCHEMA_FILE}")
endif()

#pruebas

if(LRE_BUILD_TESTS OR LRE_BUILD_PHYSICS_BENCHMARKS)
    add_executable(
        lre_physics_broadphase_benchmark
        tests/physics/broad_phase_2d_benchmark.cpp
        src/Common/physics/broad_phase_2d.cpp
        src/Common/maths/AABB.cpp
    )

    target_link_libraries(
        lre_physics_broadphase_benchmark
        PRIVATE
            Lynx::BuildConfig
    )

    target_include_directories(
        lre_physics_broadphase_benchmark
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
endif()


#INSTALACION DE LA BIBLIOTECA

if(NOT LRE_ENGINE_XCFRAMEWORK)
    install(TARGETS lre_engine lre_build_config 
        EXPORT LynxRecomposerEngineTargets
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/" DESTINATION include)
    
    include(CMakePackageConfigHelpers)

    set_target_properties(lre_build_config PROPERTIES EXPORT_NAME BuildConfig)
    set(lre_package_dir "lib/cmake/LynxRecomposerEngine")

    install(EXPORT LynxRecomposerEngineTargets 
        FILE LynxRecomposerEngineTargets.cmake
        NAMESPACE Lynx::
        DESTINATION "${lre_package_dir}"
    )

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LynxRecomposerEngineConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/LynxRecomposerEngineConfig.cmake"
        INSTALL_DESTINATION "${lre_package_dir}"
    )

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/LynxRecomposerEngineConfig.cmake"
        DESTINATION "${lre_package_dir}"
    )

    if(LRE_BUILD_ENGINE_ASSETS)
        install(FILES "${LRE_ENGINE_SCHEMA_FILE}" DESTINATION "share/lynx/schemas")
        install(FILES "${LRE_ENGINE_DEFAULT_ASSET_MANIFEST}" DESTINATION "share/lynx/manifests")
        install(DIRECTORY "${LRE_ENGINE_BINARY_ASSET_DIR}/"
            DESTINATION "share/lynx/engine_assets/${LRE_ASSET_TARGET_NAME}"
            PATTERN ".build_complete" EXCLUDE
        )
    endif()
endif()
