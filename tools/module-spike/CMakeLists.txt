cmake_minimum_required(VERSION 4.4)
project(RunmarkModuleSpike LANGUAGES CXX)

include(CTest)
find_package(Qt6 6.11 REQUIRED COMPONENTS Core Qml)

add_library(module_domain STATIC)
target_sources(module_domain PUBLIC FILE_SET CXX_MODULES FILES domain.cppm)
target_compile_features(module_domain PUBLIC cxx_std_23)
set_target_properties(module_domain PROPERTIES CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES ON)

add_executable(module_bridge_test bridge.h bridge.cpp main.cpp)
target_link_libraries(module_bridge_test PRIVATE module_domain Qt6::Core Qt6::Qml)
set_target_properties(module_bridge_test PROPERTIES
    AUTOMOC ON
    CXX_EXTENSIONS OFF
    CXX_SCAN_FOR_MODULES ON
)
add_test(NAME module_qml_bridge COMMAND module_bridge_test)
