cmake_minimum_required(VERSION 3.20)
project(ilen-engine LANGUAGES CXX VERSION 0.0.1)

if(APPLE)
enable_language(OBJCXX)
endif()

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

file(GLOB_RECURSE PROJECT_SOURCES src/main.cpp src/Common/*.cpp src/Examples/*.cpp)

if(NOT APPLE)
    add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
else()
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${PROJECT_SOURCES})
endif()

#target_include_directories(${PROJECT_NAME} PRIVATE include)
add_library(ilen_global_includes INTERFACE)
target_include_directories(ilen_global_includes INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME} PRIVATE ilen_global_includes)

list(APPEND SHARED_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(APPLE)
    list(APPEND EXTERNAL_SYSTEM_SPECIFIC_COMMANDS
        -DCMAKE_TOOLCHAIN_FILE:FILEPATH=${CMAKE_SOURCE_DIR}/toolchains/macos.toolchain.cmake
        -DCMAKE_OSX_ARCHITECTURES:STRING=${CMAKE_OSX_ARCHITECTURES}
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
        file(GLOB_RECURSE NATIVE_SOURCES src/Platform/iOS/*.mm src/Platform/apple/*.mm src/Renderer/Metal/*.mm)
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
            "-framework QuartzCore"
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

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND $<TARGET_FILE:ILEN_METAL_COMPILER>
            ARGS --ilen-folder ${CMAKE_SOURCE_DIR} --sdk iphoneos $<$<CONFIG:Debug>:--add-debug>
            COMMENT "Compilando shader de metal para ser usados en iOS"
            VERBATIM
        )

        #iOS need also assets folder in the bundle
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/.ilen-bin-data" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/"
        )
    else()
        message("Configuration for macOS")

        file(GLOB_RECURSE NATIVE_SOURCES src/Platform/macOS/*.mm src/Platform/apple/*.mm src/Renderer/Metal/*.mm)
        set(NATIVE_HEADERS src/Platform/macOS src/Platform/apple src/Renderer/Metal)

        target_include_directories(${PROJECT_NAME} PRIVATE ${NATIVE_HEADERS})
        target_sources(${PROJECT_NAME} PRIVATE ${NATIVE_SOURCES})
        target_link_libraries(${PROJECT_NAME} PRIVATE "-framework Cocoa")
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
    )
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

if(APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "ILEN_DESKTOP=1;ILEN_MACOS=1;ILEN_SOURCES_FOLDER=\"${CMAKE_SOURCE_DIR}\"")
elseif(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "ILEN_MOBILE=1;ILEN_IOS=1")
elseif(LINUX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE "ILEN_DESKTOP=1;ILEN_LINUX=1;VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1")
elseif(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE "ILEN_DESKTOP=1;ILEN_WINDOWS=1")
endif()


#DEFINITIONS PARA PRUEBAS EN EL MOTOR
target_compile_definitions(${PROJECT_NAME} PRIVATE "IGNORE_RENDERING=1") #Ignora el sistema de renderizado. Util para probar las ventanas sin nada más de por medio
target_compile_definitions(${PROJECT_NAME} PRIVATE "TEST_ERROR_WINDOW_AT_START=0") #Lanza una error window al empezar para revisar que se vea correctamente
