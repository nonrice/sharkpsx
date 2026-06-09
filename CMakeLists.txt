cmake_minimum_required(VERSION 3.10)

project(sharkpsx
    VERSION 0.0.0
    LANGUAGES C CXX
)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(src)
