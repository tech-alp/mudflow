# The outside world: git, filesystem, JSON/JSONL, ledger and handoff I/O.
# Uses the domain; knows nothing of the application layer.
add_library(runmark_infrastructure STATIC
    src/config_io.cpp
    src/git.cpp
    src/handoff.cpp
    src/ledger.cpp
    src/paths.cpp
    src/plan.cpp
)
add_library(runmark::infrastructure ALIAS runmark_infrastructure)

# src/ is PUBLIC because the application layer includes these internal
# headers. It is not an exported API, only visible to the layer above.
target_include_directories(runmark_infrastructure
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)
target_link_libraries(runmark_infrastructure PUBLIC runmark::warnings runmark::domain)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
