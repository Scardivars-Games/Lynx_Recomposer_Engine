set(CMAKE_SYSTEM_NAME Darwin)

execute_process(
    COMMAND xcrun --sdk macosx --show-sdk-path
    OUTPUT_VARIABLE CMAKE_OSX_SYSROOT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0" CACHE STRING "macOS deploy target for macOS, and internal tools")
