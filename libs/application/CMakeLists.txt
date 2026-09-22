# Orchestration: the inspect/status/start/finish/resume flows.
add_library(runmark_application STATIC
    src/workflow.cpp
)
add_library(runmark::application ALIAS runmark_application)

target_include_directories(runmark_application
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(runmark_application PUBLIC runmark::infrastructure)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
