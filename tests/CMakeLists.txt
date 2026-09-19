add_executable(project_config_test project_config_test.cpp)
target_link_libraries(project_config_test PRIVATE mudflow::core Qt6::Core)
add_test(NAME project_config COMMAND project_config_test)

add_executable(workflow_test workflow_test.cpp)
target_link_libraries(workflow_test PRIVATE mudflow::core Qt6::Core)
add_test(NAME workflow COMMAND workflow_test)

add_executable(cli_contract_test cli_contract_test.cpp)
target_link_libraries(cli_contract_test PRIVATE Qt6::Core)
add_dependencies(cli_contract_test mudflow)
add_test(NAME cli_contract COMMAND cli_contract_test $<TARGET_FILE:mudflow>)
