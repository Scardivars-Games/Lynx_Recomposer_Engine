set(CMAKE_SYSTEM_NAME Darwin)

execute_process(
    COMMAND xcrun --sdk macosx --show-sdk-path
    OUTPUT_VARIABLE CMAKE_OSX_SYSROOT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "macOS deploy target for macOS, and internal tools")

execute_process(
    COMMAND xcrun --sdk macosx -f clang
    OUTPUT_VARIABLE CMAKE_C_COMPILER
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND xcrun --sdk macosx -f clang++
    OUTPUT_VARIABLE CMAKE_CXX_COMPILER
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
