add_executable(config_io_test config_io_test.cpp)
target_link_libraries(config_io_test PRIVATE runmark::infrastructure Qt6::Core)
add_test(NAME config_io COMMAND config_io_test)

add_executable(transcript_test transcript_test.cpp)
target_link_libraries(transcript_test PRIVATE runmark::infrastructure Qt6::Core)
add_test(NAME transcript COMMAND transcript_test ${CMAKE_CURRENT_SOURCE_DIR}/transcripts)
