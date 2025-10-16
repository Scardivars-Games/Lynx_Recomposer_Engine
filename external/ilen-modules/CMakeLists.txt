cmake_minimum_required(VERSION 3.30)
project(ILEN_MODULES LANGUAGES CXX VERSION 0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ILEN_METAL_COMPILER modules/ilen_metal_compiler.cpp)
add_executable(ILEN_FILE_BINARIZATION modules/ilen_file_binarization.cpp)

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/executable)
#
#foreach(config ${CMAKE_CONFIGURATION_TYPES})
#    string(TOUPPER ${config} config_upper)
#    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config_upper} ${CMAKE_BINARY_DIR}/executable)
#endforeach()

install(TARGETS ILEN_METAL_COMPILER DESTINATION executable/$<CONFIG>)