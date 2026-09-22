set(CMAKE_CXX_EXTENSIONS OFF)

configure_file(
    ${PROJECT_SOURCE_DIR}/cmake/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/runmark/version.h
    @ONLY
)

add_executable(rmk
    src/json.cpp
    src/main.cpp
)

target_include_directories(rmk PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include)
target_link_libraries(rmk PRIVATE runmark::application)
set_target_properties(rmk PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)

include(GNUInstallDirs)
install(TARGETS rmk RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
