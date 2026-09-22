cmake_minimum_required(VERSION 4.4)

project(Runmark VERSION 0.3.0 LANGUAGES CXX)

option(RUNMARK_BUILD_CLI "Build the Runmark CLI" ON)
option(RUNMARK_BUILD_UI "Build the Runmark UI" OFF)

include(CTest)

add_subdirectory(core)
if(RUNMARK_BUILD_CLI)
    add_subdirectory(cli)
endif()

include(CPack)
