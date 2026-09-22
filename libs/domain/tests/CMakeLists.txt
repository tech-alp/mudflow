add_executable(rules_test rules_test.cpp)
target_link_libraries(rules_test PRIVATE runmark::domain Qt6::Core)
add_test(NAME rules COMMAND rules_test)
