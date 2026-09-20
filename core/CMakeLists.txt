find_package(Qt6 6.11 REQUIRED COMPONENTS Core)

set(CMAKE_CXX_EXTENSIONS OFF)

add_library(mudflow_core STATIC
    src/git.cpp
    src/handoff.cpp
    src/ledger.cpp
    src/paths.cpp
    src/plan.cpp
    src/project_config.cpp
    src/rules.cpp
    src/workflow.cpp
)

add_library(mudflow::core ALIAS mudflow_core)

target_include_directories(mudflow_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(mudflow_core PUBLIC Qt6::Core)
target_compile_features(mudflow_core PUBLIC cxx_std_20)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
