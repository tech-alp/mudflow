cmake_minimum_required(VERSION 4.4)

project(Runmark VERSION 0.3.0 LANGUAGES CXX)

option(RUNMARK_BUILD_CLI "Build the Runmark CLI" ON)
option(RUNMARK_BUILD_UI "Build the Runmark UI" OFF)

include(CTest)

find_package(Qt6 6.11 REQUIRED COMPONENTS Core)
set(CMAKE_CXX_EXTENSIONS OFF)

# Bagimlilik yonu: application -> infrastructure -> domain. Ters yon yok.
add_subdirectory(libs/domain)
add_subdirectory(libs/infrastructure)
add_subdirectory(libs/application)
if(RUNMARK_BUILD_CLI)
    add_subdirectory(apps/cli)
endif()

include(CPack)
