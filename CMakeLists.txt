cmake_minimum_required(VERSION 3.21)

project(mudflow LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.11 REQUIRED COMPONENTS Core)

enable_testing()

add_subdirectory(mudflow-core)
add_subdirectory(mudflow-cli)
add_subdirectory(tests)
