cmake_minimum_required(VERSION 3.20)
project(ilen-engine LANGUAGES CXX OBJCXX VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(APP_IDENTIFIER "com.scardivargames.ilen-test")
set(APP_NAME "ILEN Test")
set(APP_VERSION "0.0.1")
set(APP_BUILD_NUMBER "1")

file(GLOB_RECURSE PROJECT_SOURCES src/*.cpp)

#if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
#    add_library(${PROJECT_NAME} STATIC ${PROJECT_SOURCES})
#else()
#    add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
#endif()

if(NOT APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
else()
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${PROJECT_SOURCES})
endif()

target_include_directories(${PROJECT_NAME} PRIVATE include)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

if(APPLE)
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        message("Configuration for iOS")
        file(GLOB_RECURSE NATIVE_SOURCES src/native/iOS/*.mm src/native/apple/*.mm)
        target_sources(${PROJECT_NAME} PRIVATE ${NATIVE_SOURCES})
        target_link_libraries(${PROJECT_NAME} PRIVATE 
            "-framework Foundation" 
            "-framework UIKit"
            "-framework QuartzCore"
        )
        set_target_properties(${PROJECT_NAME} PROPERTIES        
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/native/iOS/Info.plist"
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.scardivargames.ilen-test"
            MACOSX_BUNDLE_IDENTIFIER ${APP_IDENTIFIER}
            MACOSX_BUNDLE_NAME ${APP_NAME}
            MACOSX_BUNDLE_SHORT_VERSION_STRING ${APP_VERSION}
            MACOSX_BUNDLE_VERSION ${APP_BUILD_NUMBER}
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
            )

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/"
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