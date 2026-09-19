add_library(mudflow-core STATIC
    src/project_config.cpp
    src/workflow.cpp
)

add_library(mudflow::core ALIAS mudflow-core)

target_include_directories(mudflow-core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(mudflow-core PUBLIC Qt6::Core)
