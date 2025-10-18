cmake_minimum_required(VERSION 3.20)
project(ilen-engine LANGUAGES CXX OBJCXX VERSION 0.0.1)

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

file(GLOB_RECURSE PROJECT_SOURCES src/*.cpp)

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

ExternalProject_Add(
    ilen-modules
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/ilen-modules
    BINARY_DIR ${ILEN_MODULES_BUILD_DIR}/build

    CMAKE_CACHE_ARGS
        -DPARENT_INCLUDE_DIR:STRING=${SHARED_INCLUDE_DIR}
        -DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH=${ILEN_MODULES_EXECUTABLE_FOLDER}
        -DCMAKE_INSTALL_PREFIX:PATH=${ILEN_MODULES_BUILD_DIR}
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -DCMAKE_TOOLCHAIN_FILE:FILEPATH=${CMAKE_SOURCE_DIR}/toolchains/macos.toolchain.cmake

    BUILD_BYPRODUCTS ${NATIVE_TOOL_BYPRODUCT}
    #BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR>
    BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR>

    LOG_CONFIGURE 1
    LOG_BUILD 1
    LOG_INSTALL 1
)

add_executable(ILEN_METAL_COMPILER IMPORTED GLOBAL)
set_property(
    TARGET ILEN_METAL_COMPILER PROPERTY IMPORTED_LOCATION ${NATIVE_TOOL_BYPRODUCT}/ILEN_METAL_COMPILER
)

add_executable(ILEN_FILE_BINARIZATION IMPORTED GLOBAL)
set_property(
        TARGET ILEN_FILE_BINARIZATION PROPERTY IMPORTED_LOCATION ${NATIVE_TOOL_BYPRODUCT}/ILEN_FILE_BINARIZATION
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
        file(GLOB_RECURSE NATIVE_SOURCES src/native/iOS/*.mm src/native/apple/*.mm)
        set(ASSET_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/src/native/iOS/icon/Assets.xcassets")
        set(APPLE_LIQUID_GLASS_APPICON "${CMAKE_CURRENT_SOURCE_DIR}/src/native/iOS/icon/AppIcon.icon")

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
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/native/iOS/Info.plist"
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
            ARGS --ilen-folder ${CMAKE_SOURCE_DIR} --sdk iphoneos
            COMMENT "Compilando shader de metal para ser usados en iOS"
            VERBATIM
        )

        #iOS need also assets folder in the bundle
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/.ilen-bin-data" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/"
        )
    else()
        message("Configuration for macOS")

        file(GLOB_RECURSE NATIVE_SOURCES src/native/macOS/*.mm src/native/apple/*.mm)
        target_sources(${PROJECT_NAME} PRIVATE ${NATIVE_SOURCES})
        target_link_libraries(${PROJECT_NAME} PRIVATE "-framework Cocoa")
        set_target_properties(${PROJECT_NAME} PROPERTIES      
            BUNDLE TRUE  
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/native/macOS/Info.plist"
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
            ARGS --ilen-folder ${CMAKE_SOURCE_DIR} --sdk macosx
            COMMENT "Compilando shader de metal para ser usados en MacOS"
            VERBATIM
        )

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/.ilen-bin-data" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/src/native/macOS/icon" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/"
        )
    endif()
    #common libs in Apple
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework MetalKit" 
        "-framework Metal"
    )
endif()

add_dependencies(${PROJECT_NAME} ILEN_METAL_COMPILER)
add_dependencies(${PROJECT_NAME} ILEN_FILE_BINARIZATION)

target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:ILEN_DEBUG>)

if(APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "ILEN_DESKTOP=1;ILEN_MACOS=1;ILEN_SOURCES_FOLDER=\"${CMAKE_SOURCE_DIR}\"")
elseif(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "ILEN_MOBILE=1;ILEN_IOS=1")
endif()