find_package(Qt6 6.11 REQUIRED COMPONENTS Core)

set(CMAKE_CXX_EXTENSIONS OFF)

add_library(runmark_core STATIC
    src/git.cpp
    src/handoff.cpp
    src/ledger.cpp
    src/paths.cpp
    src/plan.cpp
    src/project_config.cpp
    src/rules.cpp
    src/workflow.cpp
)

add_library(runmark::core ALIAS runmark_core)

target_include_directories(runmark_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(runmark_core PUBLIC Qt6::Core)
target_compile_features(runmark_core PUBLIC cxx_std_20)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
