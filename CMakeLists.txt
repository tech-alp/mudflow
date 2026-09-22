cmake_minimum_required(VERSION 4.4)

set(RUNMARK_VERSION "0.3.0" CACHE STRING "Release version injected by semantic-release")
if(NOT RUNMARK_VERSION MATCHES "^(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)$")
    message(FATAL_ERROR "RUNMARK_VERSION must be a stable x.y.z version")
endif()
project(Runmark VERSION ${RUNMARK_VERSION} LANGUAGES CXX)

option(RUNMARK_BUILD_CLI "Build the Runmark CLI" ON)
option(RUNMARK_BUILD_UI "Build the Runmark UI" OFF)
option(RUNMARK_BUNDLE_RUNTIME "Bundle macOS CLI runtime for distribution" OFF)

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
# The UI is optional on purpose: a CLI-only build must not require Qt Quick.
if(RUNMARK_BUILD_UI)
    add_subdirectory(libs/ui-shell)
    add_subdirectory(apps/desktop)
endif()

set(CPACK_GENERATOR TGZ)
set(CPACK_PACKAGE_NAME runmark)
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "runmark-${PROJECT_VERSION}-macos-arm64")
include(CPack)
