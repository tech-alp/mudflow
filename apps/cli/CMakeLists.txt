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

if(RUNMARK_BUNDLE_RUNTIME)
    if(NOT APPLE OR NOT CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
        message(FATAL_ERROR "Runtime packaging currently supports macOS arm64 only")
    endif()
    get_target_property(qt_core_location Qt6::Core IMPORTED_LOCATION_RELEASE)
    if(NOT qt_core_location)
        get_target_property(qt_core_location Qt6::Core IMPORTED_LOCATION)
    endif()
    get_filename_component(qt_core_dir "${qt_core_location}" DIRECTORY)
    configure_file(${PROJECT_SOURCE_DIR}/cmake/DeployCli.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/DeployCli.cmake @ONLY)
    install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/DeployCli.cmake)
    install(FILES ${PROJECT_SOURCE_DIR}/tools/release/NOTICE.md DESTINATION share/doc/runmark)
    if(EXISTS "${PROJECT_SOURCE_DIR}/LICENSE")
        install(FILES ${PROJECT_SOURCE_DIR}/LICENSE DESTINATION share/doc/runmark)
    endif()
    if(IS_DIRECTORY "${PROJECT_SOURCE_DIR}/THIRD_PARTY_NOTICES")
        install(DIRECTORY ${PROJECT_SOURCE_DIR}/THIRD_PARTY_NOTICES DESTINATION share/doc/runmark)
    endif()
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
