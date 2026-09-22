# Dis dunya: git, dosya sistemi, JSON/JSONL, ledger ve handoff I/O.
# Domain'i kullanir; application'i bilmez.
add_library(runmark_infrastructure STATIC
    src/config_io.cpp
    src/git.cpp
    src/handoff.cpp
    src/ledger.cpp
    src/paths.cpp
    src/plan.cpp
)
add_library(runmark::infrastructure ALIAS runmark_infrastructure)

# src/ PUBLIC: application bu ic basliklari include eder. Disa acilan API
# degil, ayni agacin ust katmani icin gorunur.
target_include_directories(runmark_infrastructure
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)
target_link_libraries(runmark_infrastructure PUBLIC runmark::domain)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
