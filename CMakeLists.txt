cmake_minimum_required(VERSION 4.4)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(RunmarkVersion)

project(Runmark VERSION ${RUNMARK_VERSION} LANGUAGES CXX)

option(RUNMARK_BUILD_CLI "Build the Runmark CLI" ON)
option(RUNMARK_BUILD_UI "Build the Runmark UI" ON)
option(RUNMARK_BUNDLE_RUNTIME "Bundle macOS CLI runtime for distribution" OFF)
option(RUNMARK_MERCE_NOTIFICATIONS "Build Merce's notification and dialog module" OFF)

include(CTest)

include(RunmarkWarnings)

find_package(Qt6 6.11 REQUIRED COMPONENTS Core)
set(CMAKE_CXX_EXTENSIONS OFF)

# Bagimlilik yonu: application -> infrastructure -> domain. Ters yon yok.
add_subdirectory(libs/domain)
add_subdirectory(libs/infrastructure)
add_subdirectory(libs/application)
if(RUNMARK_BUILD_CLI)
    add_subdirectory(apps/cli)
endif()
# The UI is optional on purpose: a CLI-only build must not require Qt Quick.
if(RUNMARK_BUILD_UI)
    add_subdirectory(libs/ui-shell)
    add_subdirectory(apps/desktop)
endif()

include(RunmarkPackaging)
