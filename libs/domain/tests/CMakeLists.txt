add_executable(rules_test rules_test.cpp)
target_link_libraries(rules_test PRIVATE runmark::domain Qt6::Core)
add_test(NAME rules COMMAND rules_test)

add_test(NAME domain_purity COMMAND ${CMAKE_COMMAND}
    -DDIR=${CMAKE_CURRENT_SOURCE_DIR}/.. -P ${CMAKE_CURRENT_SOURCE_DIR}/purity.cmake)
