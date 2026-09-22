# Pure rules and data. No dependency beyond Qt6::Core and, in particular, NOT
# on infrastructure -- the dependency direction is one-way (TC-009).
add_library(runmark_domain STATIC
    src/project_config.cpp
    src/rules.cpp
)
add_library(runmark::domain ALIAS runmark_domain)

target_include_directories(runmark_domain
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(runmark_domain PUBLIC runmark::warnings Qt6::Core)
target_sources(runmark_domain PUBLIC FILE_SET CXX_MODULES FILES src/domain.cppm)
target_compile_features(runmark_domain PUBLIC cxx_std_23)
set_property(TARGET runmark_domain PROPERTY CXX_SCAN_FOR_MODULES ON)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
