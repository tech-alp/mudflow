add_executable(mudflow
    src/main.cpp
)

target_link_libraries(mudflow PRIVATE mudflow::core Qt6::Core)
