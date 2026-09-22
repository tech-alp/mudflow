cmake_minimum_required(VERSION 3.30)
project(RunmarkTheme LANGUAGES CXX)
find_package(Qt6 6.11 REQUIRED COMPONENTS Core Gui)
if(NOT EXISTS "${MERCE_SOURCE}/Theme/BrandDerivation.cpp")
    message(FATAL_ERROR "Set MERCE_SOURCE to the pinned Merce v1.2.0 checkout")
endif()
include("${MERCE_SOURCE}/cmake/McuSubset.cmake")
add_executable(runmark-theme main.cpp
    "${MERCE_SOURCE}/Theme/BrandDerivation.cpp"
    "${MERCE_SOURCE}/Theme/ThemeValidator.cpp")
target_compile_features(runmark-theme PRIVATE cxx_std_17)
target_include_directories(runmark-theme PRIVATE "${MERCE_SOURCE}/Theme")
target_link_libraries(runmark-theme PRIVATE Qt6::Gui MerceMcuSubset)
