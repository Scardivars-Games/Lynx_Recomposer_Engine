# LinuxCrossToolchain.cmake

cmake_minimum_required(VERSION 3.18)
set(CMAKE_SYSTEM_NAME Linux)

if(NOT TARGET_ARCH)
    set(TARGET_ARCH native CACHE STRING "native | x86_64 | arm64")
endif()

string(TOLOWER "${TARGET_ARCH}" arch)
if(arch STREQUAL "native")
    message(STATUS "Native Linux build")
    return()
endif()

if(arch MATCHES "^(arm64|aarch64)$")
    set(TOOLCHAIN aarch64-linux-gnu)
    set(CMAKE_SYSTEM_PROCESSOR aarch64)
elseif(arch MATCHES "^(x86_64|amd64)$")
    set(TOOLCHAIN x86_64-linux-gnu)
    set(CMAKE_SYSTEM_PROCESSOR x86_64)
else()
    message(FATAL_ERROR "Unsupported TARGET_ARCH: ${TARGET_ARCH}")
endif()

message(STATUS "Cross-compiling for ${TOOLCHAIN}")

# ─────────────────────────────────────────────────────────────────
# This is the only thing that really matters for pkg-config on Ubuntu
# ─────────────────────────────────────────────────────────────────
#set(CMAKE_SYSROOT "/")                                      # important
set(CMAKE_FIND_ROOT_PATH "/usr/${TOOLCHAIN}")               # important

set(CMAKE_C_COMPILER   ${TOOLCHAIN}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN}-g++)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY  ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE  ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE  ONLY)

# Force the correct pkg-config even if the user has PKG_CONFIG set in his environment
set(CMAKE_PREFIX_PATH "/usr/${TOOLCHAIN}")                  # helps find .pc files
set(ENV{PKG_CONFIG_LIBDIR} "")                              # clear host paths
set(ENV{PKG_CONFIG_SYSROOT_DIR} "")                         # must be empty when SYSROOT="/"

# This line is the real killer – without it CMake will silently fall back to host pkg-config
set(PKG_CONFIG_EXECUTABLE "${TOOLCHAIN}-pkg-config" CACHE FILEPATH "pkg-config for target" FORCE)
