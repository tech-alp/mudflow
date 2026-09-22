# Saf kurallar ve veri. Qt6::Core disinda bagimlilik yok; ozellikle
# infrastructure'a bagimli DEGIL — bagimlilik yonu tek taraflidir (TC-009).
add_library(runmark_domain STATIC
    src/project_config.cpp
    src/rules.cpp
)
add_library(runmark::domain ALIAS runmark_domain)

target_include_directories(runmark_domain
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(runmark_domain PUBLIC Qt6::Core)
target_compile_features(runmark_domain PUBLIC cxx_std_20)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
