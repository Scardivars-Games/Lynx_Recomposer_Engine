cmake_minimum_required(VERSION 3.20)
project(ilen-engine LANGUAGES CXX VERSION 0.0.1)

if(APPLE)
enable_language(OBJCXX)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(ExternalProject)

set(ILEN_MODULES_BUILD_DIR ${CMAKE_BINARY_DIR}/ILEN_MODULES)
set(ILEN_MODULES_EXECUTABLE_FOLDER ${ILEN_MODULES_BUILD_DIR}/executable)

set(NATIVE_TOOL_BYPRODUCT ${ILEN_MODULES_EXECUTABLE_FOLDER})

set(APP_IDENTIFIER "com.scardivargames.ilen-test")
set(APP_NAME "ILEN Test")
set(APP_VERSION "0.0.1")
set(APP_BUILD_NUMBER "1")
set(APP_COPYRIGHT "Copyright (c) 2025 Scardivar's Games. All rights reserved.")

option(ILEN_METAL_HUD "Muestra el HUD de Metal para debuggear")
option(ILEN_OPTIMIZED_BUILD_FLAGS "Activa flags de optimización agresivas. No recomendado para debuggear, o usar para apps que no sean juegos" OFF)

if(CMAKE_SYSTEM_NAME MATCHES "iOS|tvOS")
    set(ARCH_FLAGS -arch arm64 -mcpu=apple-a13)

elseif(ANDROID)
    if(CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
        set(ARCH_FLAGS -march=armv8-a)
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "armeabi-v7a")
        set(ARCH_FLAGS -march=armv7-a -mfpu=neon)
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64")
        set(ARCH_FLAGS -march=x86-64-v3)
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86")
        set(ARCH_FLAGS -march=i686)
    endif()

elseif(APPLE)
    if("arm64" IN_LIST CMAKE_OSX_ARCHITECTURES AND
       "x86_64" IN_LIST CMAKE_OSX_ARCHITECTURES)
        set(ARCH_FLAGS "")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
        set(ARCH_FLAGS -mcpu=apple-m1)
    else()
        set(ARCH_FLAGS -march=x86-64-v3)
    endif()

else()
    # Windows, Linux, BSD, etc.
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
        if(MSVC)
            set(ARCH_FLAGS /arch:arm64)
        else()
            set(ARCH_FLAGS -march=armv8-a)
        endif()
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(armv7|armhf|arm)$")
        set(ARCH_FLAGS -march=armv7-a -mfpu=neon)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$")
        if(MSVC)
            set(ARCH_FLAGS /arch:AVX2)
        else()
            set(ARCH_FLAGS -march=x86-64-v3)
        endif()
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(i[3-6]86|x86)$")
        set(ARCH_FLAGS -march=i686)
    else()
        message(WARNING "Arquitectura desconocida: ${CMAKE_SYSTEM_PROCESSOR}, sin ARCH_FLAGS")
        set(ARCH_FLAGS "")
    endif()
endif()

file(GLOB_RECURSE PROJECT_SOURCES src/main.cpp src/Common/*.cpp src/Examples/*.cpp)

if(NOT APPLE)
    add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
else()
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${PROJECT_SOURCES})
endif()

if(ILEN_OPTIMIZED_BUILD_FLAGS)
    if(CMAKE_BUILD_TYPE MATCHES "Release|RelWithDebInfo")
        message("Activando flags de optimización agresivas para build optimizada")
        target_compile_options(${PROJECT_NAME} PRIVATE
            -O3
            ${ARCH_FLAGS}
            -ffast-math
            -flto=thin
            -funroll-loops
            -fomit-frame-pointer
            -fstrict-vtable-pointers
            -DNDEBUG
        )

        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    else()
        message(WARNING "ILEN_OPTIMIZED_BUILD_FLAGS está activado, pero la configuración actual no es Release o RelWithDebInfo. Las flags de optimización agresivas solo se aplicarán a builds optimizadas.")
    endif()
endif()

#target_include_directories(${PROJECT_NAME} PRIVATE include)
add_library(ilen_global_includes INTERFACE)
target_include_directories(ilen_global_includes INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME} PRIVATE ilen_global_includes)

list(APPEND SHARED_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

#Threads support
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

if(APPLE)
    list(APPEND EXTERNAL_SYSTEM_SPECIFIC_COMMANDS
        -DCMAKE_TOOLCHAIN_FILE:FILEPATH=${CMAKE_SOURCE_DIR}/toolchains/macos.toolchain.cmake
        #-DCMAKE_OSX_ARCHITECTURES:STRING=${CMAKE_OSX_ARCHITECTURES}
        -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE:STRING="Automatic"
    )
elseif(WIN32)   
    list(APPEND EXTERNAL_SYSTEM_SPECIFIC_COMMANDS
        -DCMAKE_TOOLCHAIN_FILE:FILEPATH=${CMAKE_SOURCE_DIR}/toolchains/windows.toolchain.cmake
    )
elseif(LINUX)    
    list(APPEND EXTERNAL_SYSTEM_SPECIFIC_COMMANDS
        -DCMAKE_TOOLCHAIN_FILE:FILEPATH=${CMAKE_SOURCE_DIR}/toolchains/linux.toolchain.cmake
    )
endif()


ExternalProject_Add(
    ilen-modules
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/ilen-modules
    BINARY_DIR ${ILEN_MODULES_BUILD_DIR}/build

    CMAKE_CACHE_ARGS
        -DPARENT_INCLUDE_DIR:STRING=${SHARED_INCLUDE_DIR}
        -DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH=${ILEN_MODULES_EXECUTABLE_FOLDER}
        -DCMAKE_INSTALL_PREFIX:PATH=${ILEN_MODULES_BUILD_DIR}
        #-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -DBUILD_SHARED_LIBS:BOOL=OFF
        ${EXTERNAL_SYSTEM_SPECIFIC_COMMANDS}

    BUILD_BYPRODUCTS ${NATIVE_TOOL_BYPRODUCT}
    #BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR>
    BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR>

    LOG_CONFIGURE 1
    LOG_BUILD 1
    LOG_INSTALL 1
)

add_executable(ILEN_METAL_COMPILER IMPORTED GLOBAL)
set_property(
    TARGET ILEN_METAL_COMPILER PROPERTY IMPORTED_LOCATION ${NATIVE_TOOL_BYPRODUCT}/${CMAKE_CFG_INTDIR}/ILEN_METAL_COMPILER
)

add_executable(ILEN_FILE_BINARIZATION IMPORTED GLOBAL)
set_property(
        TARGET ILEN_FILE_BINARIZATION PROPERTY IMPORTED_LOCATION ${NATIVE_TOOL_BYPRODUCT}/${CMAKE_CFG_INTDIR}/ILEN_FILE_BINARIZATION
)

add_dependencies(ILEN_METAL_COMPILER ilen-modules)
add_dependencies(ILEN_FILE_BINARIZATION ilen-modules)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)


#Command used by all Platforms
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND $<TARGET_FILE:ILEN_FILE_BINARIZATION>
    ARGS --ilen-folder ${CMAKE_SOURCE_DIR}
    COMMENT "Organizing and binarizing all asset of the engine"
    VERBATIM
)


#Commands for the Apple Ecosystem
if(APPLE)
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        message("Configuration for iOS")
        file(GLOB_RECURSE NATIVE_SOURCES src/Platform/iOS/*.mm src/Platform/apple/*.mm src/Platform/apple/*.cpp src/Renderer/Metal/*.mm)
        set(NATIVE_HEADERS src/Platform/iOS src/Platform/apple src/Renderer/Metal)

        target_include_directories(${PROJECT_NAME} PRIVATE ${NATIVE_HEADERS})
        set(ASSET_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/iOS/icon/Assets.xcassets")
        set(APPLE_LIQUID_GLASS_APPICON "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/iOS/icon/AppIcon.icon")

        set_source_files_properties(${ASSET_FOLDER} PROPERTIES RESOURCE TRUE)
        set_source_files_properties(${APPLE_LIQUID_GLASS_APPICON} PROPERTIES RESOURCE TRUE)

        target_sources(${PROJECT_NAME} PRIVATE ${NATIVE_SOURCES} ${ASSET_FOLDER} ${APPLE_LIQUID_GLASS_APPICON})
        target_link_libraries(${PROJECT_NAME} PRIVATE 
            "-framework Foundation" 
            "-framework UIKit"
        )
        set_target_properties(${PROJECT_NAME} PROPERTIES        
            XCODE_ATTRIBUTE_SKIP_INSTALL "NO"
            XCODE_ATTRIBUTE_INSTALL_PATH "$(LOCAL_APPS_DIR)"
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/iOS/Info.plist"
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.scardivargames.ilen-test"
            MACOSX_BUNDLE_IDENTIFIER ${APP_IDENTIFIER}
            MACOSX_BUNDLE_NAME ${APP_NAME}
            MACOSX_BUNDLE_SHORT_VERSION_STRING ${APP_VERSION}
            MACOSX_BUNDLE_VERSION ${APP_BUILD_NUMBER}
            MACOSX_BUNDLE_COPYRIGHT ${APP_COPYRIGHT}
            XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
            XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "3QYB6BMV6T"
            XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
            XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_ASSET_PATHS "${ASSET_FOLDER}"
            XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
            )

            target_compile_options(${PROJECT_NAME} PRIVATE "-fno-objc-arc")

    if(PLATFORM STREQUAL "SIMULATOR")
        set(ILEN_METAL_SDK "iphonesimulator")
    else()
        set(ILEN_METAL_SDK "iphoneos")
    endif()

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND $<TARGET_FILE:ILEN_METAL_COMPILER>
            ARGS --ilen-folder ${CMAKE_SOURCE_DIR} --sdk ${ILEN_METAL_SDK} $<$<CONFIG:Debug>:--add-debug>
            COMMENT "Compilando shader de metal para ser usados en iOS"
            VERBATIM
        )

        #iOS need also assets folder in the bundle
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/.ilen-bin-data" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/"
        )
    elseif(CMAKE_SYSTEM_NAME STREQUAL "tvOS")
        message("Configuration for tvOS")
        
        # Asumiendo una estructura de carpetas similar a iOS pero para tvOS
        file(GLOB_RECURSE NATIVE_SOURCES src/Platform/tvOS/*.mm src/Platform/apple/*.mm src/Renderer/Metal/*.mm src/Platform/apple/*.cpp)
        set(NATIVE_HEADERS src/Platform/tvOS src/Platform/apple src/Renderer/Metal)

        target_include_directories(${PROJECT_NAME} PRIVATE ${NATIVE_HEADERS})
        
        # Ajusta estas rutas a tus carpetas de iconos de tvOS (suelen ser capas LCR en lugar de imágenes planas)
        set(ASSET_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/tvOS/icon/Assets.xcassets")
        
        set_source_files_properties(${ASSET_FOLDER} PROPERTIES RESOURCE TRUE)

        target_sources(${PROJECT_NAME} PRIVATE ${NATIVE_SOURCES} ${ASSET_FOLDER})
        
        target_link_libraries(${PROJECT_NAME} PRIVATE 
            "-framework Foundation" 
            "-framework UIKit"
            "-framework TVUIKit"
        )

        set_target_properties(${PROJECT_NAME} PROPERTIES        
            XCODE_ATTRIBUTE_SKIP_INSTALL "NO"
            XCODE_ATTRIBUTE_INSTALL_PATH "$(LOCAL_APPS_DIR)"
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/tvOS/Info.plist"
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.scardivargames.ilen-test"
            MACOSX_BUNDLE_IDENTIFIER ${APP_IDENTIFIER}
            MACOSX_BUNDLE_NAME ${APP_NAME}
            MACOSX_BUNDLE_SHORT_VERSION_STRING ${APP_VERSION}
            MACOSX_BUNDLE_VERSION ${APP_BUILD_NUMBER}
            MACOSX_BUNDLE_COPYRIGHT ${APP_COPYRIGHT}
            XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
            XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "3QYB6BMV6T"
            XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
            XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_ASSET_PATHS "${ASSET_FOLDER}"
            # "3" es el identificador para Apple TV
            XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "3"
        )


        target_compile_options(${PROJECT_NAME} PRIVATE "-fno-objc-arc")

        if(PLATFORM STREQUAL "SIMULATOR")
            set(ILEN_METAL_SDK "appletvsimulator")
        else()
            set(ILEN_METAL_SDK "appletvos")
        endif()

        # Comando para shaders de Metal ajustado al SDK de tvOS
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND $<TARGET_FILE:ILEN_METAL_COMPILER>
            ARGS --ilen-folder ${CMAKE_SOURCE_DIR} --sdk ${ILEN_METAL_SDK} $<$<CONFIG:Debug>:--add-debug>
            COMMENT "Compilando shader de metal para ser usados en tvOS"
            VERBATIM
        )

        # Copia de assets al bundle
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/.ilen-bin-data" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/"
        )
    else()
        message("Configuration for macOS")

        file(GLOB_RECURSE NATIVE_SOURCES src/Platform/macOS/*.mm src/Platform/apple/*.mm src/Renderer/Metal/*.mm src/Platform/apple/*.cpp)
        set(NATIVE_HEADERS src/Platform/macOS src/Platform/apple src/Renderer/Metal)

        target_include_directories(${PROJECT_NAME} PRIVATE ${NATIVE_HEADERS})
        target_sources(${PROJECT_NAME} PRIVATE ${NATIVE_SOURCES})

        target_link_libraries(${PROJECT_NAME} PRIVATE
            "-framework Cocoa"
            )

        set_target_properties(${PROJECT_NAME} PROPERTIES      
            BUNDLE TRUE  
            XCODE_ATTRIBUTE_SKIP_INSTALL "NO"
            XCODE_ATTRIBUTE_INSTALL_PATH "$(LOCAL_APPS_DIR)"
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/macOS/Info.plist"
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.scardivargames.ilen-test"
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.scardivargames.ilen-test"
            MACOSX_BUNDLE_IDENTIFIER ${APP_IDENTIFIER}
            MACOSX_BUNDLE_NAME ${APP_NAME}
            MACOSX_BUNDLE_SHORT_VERSION_STRING ${APP_VERSION}
            MACOSX_BUNDLE_VERSION ${APP_BUILD_NUMBER}
            MACOSX_BUNDLE_COPYRIGHT ${APP_COPYRIGHT}
            MACOSX_BUNDLE_BUNDLE_NAME ${APP_NAME}
            )

        if(NOT EXISTS ${CMAKE_SOURCE_DIR}/.ilen-bin-data)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/.ilen-bin-data"
            )
        endif()

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND $<TARGET_FILE:ILEN_METAL_COMPILER>
            ARGS --ilen-folder ${CMAKE_SOURCE_DIR} --sdk macosx $<$<CONFIG:Debug>:--add-debug>
            COMMENT "Compilando shader de metal para ser usados en MacOS"
            VERBATIM
        )

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/.ilen-bin-data" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/src/Platform/macOS/icon" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/"
        )
    endif()
    #common libs in Apple
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework MetalKit" 
        "-framework Metal"
        "-framework QuartzCore"
    )

    #HUD de Metal para debuggear
    if(ILEN_METAL_HUD)
        add_custom_target(run COMMAND ${CMAKE_COMMAND} -E env MTL_HUD_ENABLED=1
        $<TARGET_FILE:${PROJECT_NAME}> DEPENDS ${PROJECT_NAME} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}>)
    endif()
elseif(LINUX)
    message("Configuring for Linux")

    file(GLOB_RECURSE NATIVE_SOURCES src/Platform/linux/*.cpp src/Renderer/Vulkan/*.cpp)
    set(NATIVE_HEADERS src/Platform/linux src/Renderer/Vulkan)
    target_include_directories(${PROJECT_NAME} PRIVATE ${NATIVE_HEADERS})
    target_sources(${PROJECT_NAME} PRIVATE ${NATIVE_SOURCES})

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(XCB REQUIRED xcb)
    pkg_check_modules(XCB_WM REQUIRED xcb-ewmh xcb-icccm)

    target_include_directories(${PROJECT_NAME} PRIVATE ${XCB_INCLUDE_DIRS} ${XCB_EWMH_INCLUDE_DIRS} ${XCB_ICCCM_INCLUDE_DIRS})
    target_link_directories(${PROJECT_NAME} PRIVATE ${XCB_LIBRARY_DIRS} ${XCB_EWMH_LIBRARY_DIRS} ${XCB_ICCCM_LIBRARY_DIRS})
    #target_link_libraries(${PROJECT_NAME} PRIVATE ${XCB_LIBRARIES} ${XCB_EWMH_LIBRARIES} ${XCB_ICCCM_LIBRARIES})
    target_link_libraries(${PROJECT_NAME} PRIVATE xcb xcb-ewmh xcb-icccm)
    target_compile_options(${PROJECT_NAME} PRIVATE ${XCB_CFLAGS_OTHER})

    set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/${PROJECT_NAME})

    # Ejecutable y DLLs/.so siempre dentro de build/IlusionsEngine/
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR} CACHE PATH " " FORCE)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR} CACHE PATH " " FORCE)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR} CACHE PATH " " FORCE)

    find_package(Vulkan REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::Vulkan)

    #iOS need also assets folder in the bundle
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/.ilen-bin-data" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/Assets/"
    )
endif()

add_dependencies(${PROJECT_NAME} ILEN_METAL_COMPILER)
add_dependencies(${PROJECT_NAME} ILEN_FILE_BINARIZATION)

target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:ILEN_DEBUG>)

if(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "ILEN_MOBILE=1;ILEN_IOS=1")
elseif(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "tvOS")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "ILEN_MOBILE=1;ILEN_TVOS=1")
elseif(APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "tvOS")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "ILEN_DESKTOP=1;ILEN_MACOS=1;ILEN_SOURCES_FOLDER=\"${CMAKE_SOURCE_DIR}\"")
elseif(LINUX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE "ILEN_DESKTOP=1;ILEN_LINUX=1;VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1")
elseif(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE "ILEN_DESKTOP=1;ILEN_WINDOWS=1")
endif()


#DEFINITIONS PARA PRUEBAS EN EL MOTOR
target_compile_definitions(${PROJECT_NAME} PRIVATE "IGNORE_RENDERING=0") #Ignora el sistema de renderizado. Util para probar las ventanas sin nada más de por medio
target_compile_definitions(${PROJECT_NAME} PRIVATE "TEST_ERROR_WINDOW_AT_START=0") #Lanza una error window al empezar para revisar que se vea correctamente
target_compile_definitions(${PROJECT_NAME} PRIVATE "ILEN_ONLY_ECS=0") #Desactiva el sistema de scripting del motor, forzando a ser solamente ECS.
target_compile_definitions(${PROJECT_NAME} PRIVATE "ILEN_SINGLECORE_TEST=0") #Fuerza al sistema de Jobs tener 1 solo hilo disponible
target_compile_definitions(${PROJECT_NAME} PRIVATE "ILEN_DISABLE_BASIC_LOGGIN=0") #Desactiva las llamadas de logging de la build (las llamadas de Warning y Error sigan funcionando.)
target_compile_definitions(${PROJECT_NAME} PRIVATE "ILEN_DISABLE_TIME_LOGGING=1") #Desactiva las llamadas de logging de la build (las llamadas de Warning y Error sigan funcionando.)