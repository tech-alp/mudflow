add_executable(project_config_test project_config_test.cpp)
target_link_libraries(project_config_test PRIVATE runmark::core Qt6::Core)
add_test(NAME project_config COMMAND project_config_test)

add_executable(workflow_test workflow_test.cpp)
target_link_libraries(workflow_test PRIVATE runmark::core Qt6::Core)
add_test(NAME workflow COMMAND workflow_test)

add_executable(state_exclude_test state_exclude_test.cpp)
target_link_libraries(state_exclude_test PRIVATE runmark::core Qt6::Core)
add_test(NAME state_exclude COMMAND state_exclude_test)

add_executable(base_ref_test base_ref_test.cpp)
target_link_libraries(base_ref_test PRIVATE runmark::core Qt6::Core)
add_test(NAME base_ref COMMAND base_ref_test)

add_executable(preserved_worktree_test preserved_worktree_test.cpp)
target_link_libraries(preserved_worktree_test PRIVATE runmark::core Qt6::Core)
add_test(NAME preserved_worktree COMMAND preserved_worktree_test)

add_executable(plan_rules_test plan_rules_test.cpp)
target_link_libraries(plan_rules_test PRIVATE runmark::core Qt6::Core)
add_test(NAME plan_rules COMMAND plan_rules_test)

add_executable(rules_test rules_test.cpp)
target_link_libraries(rules_test PRIVATE runmark::core Qt6::Core)
add_test(NAME rules COMMAND rules_test)
