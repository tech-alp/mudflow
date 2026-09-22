find_package(Qt6 6.11 REQUIRED COMPONENTS Gui Quick LabsStyleKit)

qt_add_executable(runmark-desktop src/main.cpp)
# A static QML module registers its types through its plugin target; without
# it the engine reports "No module named Runmark.Shell".
target_link_libraries(runmark-desktop PRIVATE runmark::ui_shell runmark_ui_shellplugin Qt6::Quick
    MerceThemeplugin MerceFoundationplugin MerceStyleplugin MerceControlsplugin)

# Merce 1.2 accepts external profile sources only as filesystem paths.
foreach(profile_file themes/index.json profiles/desktop.json)
    configure_file("${PROJECT_SOURCE_DIR}/libs/ui-shell/theme/${profile_file}"
        "${CMAKE_CURRENT_BINARY_DIR}/theme/${profile_file}" COPYONLY)
endforeach()
install(DIRECTORY "${PROJECT_SOURCE_DIR}/libs/ui-shell/theme/"
    DESTINATION "${CMAKE_INSTALL_BINDIR}/theme")

qt_import_qml_plugins(runmark-desktop)

install(TARGETS runmark-desktop RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(BUILD_TESTING)
    add_test(NAME desktop_smoke COMMAND $<TARGET_FILE:runmark-desktop>
        -platform offscreen --project ${PROJECT_SOURCE_DIR}/.runmark/project.json)
    set_tests_properties(desktop_smoke PROPERTIES
        ENVIRONMENT "RUNMARK_SMOKE=1;QT_QPA_PLATFORM=offscreen"
        TIMEOUT 30
        PASS_REGULAR_EXPRESSION "smoke: project=.* findings=[0-9]+ error=none profile=desktop control=32"
        FAIL_REGULAR_EXPRESSION "QQmlApplicationEngine failed;is not a type;module .* is not installed;TypeError;ReferenceError;theme initialization failed;profile check failed")
endif()
