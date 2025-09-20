cmake_minimum_required(VERSION 3.20)
project(ilen-engine LANGUAGES CXX OBJCXX VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

add_subdirectory(external/assimp)

target_include_directories(${PROJECT_NAME} PRIVATE include)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)
target_link_libraries(${PROJECT_NAME} PRIVATE assimp)
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
        #iOS need also assets folder in the bundle
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/"
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

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/src/native/macOS/icon" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/"
        )
    endif()
    #common libs in Apple
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework MetalKit" 
        "-framework Metal"
    )
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:ILEN_DEBUG>)

if(APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "ILEN_DESKTOP=1;ILEN_MACOS=1;ILEN_SOURCES_FOLDER=\"${CMAKE_SOURCE_DIR}\"")
elseif(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "ILEN_MOBILE=1;ILEN_IOS=1")
endif()