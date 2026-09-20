cmake_minimum_required(VERSION 3.21)

project(Mudflow VERSION 0.2.0 LANGUAGES CXX)

option(MUDFLOW_BUILD_CLI "Build the Mudflow CLI" ON)
option(MUDFLOW_BUILD_UI "Build the Mudflow UI" OFF)

include(CTest)

add_subdirectory(core)
if(MUDFLOW_BUILD_CLI)
    add_subdirectory(cli)
endif()

include(CPack)
