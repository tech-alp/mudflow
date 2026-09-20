set(CMAKE_CXX_EXTENSIONS OFF)

configure_file(
    ${PROJECT_SOURCE_DIR}/cmake/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/mudflow/version.h
    @ONLY
)

add_executable(mudflow
    src/main.cpp
)

target_include_directories(mudflow PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include)
target_link_libraries(mudflow PRIVATE mudflow::core)
set_target_properties(mudflow PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)

include(GNUInstallDirs)
install(TARGETS mudflow RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
