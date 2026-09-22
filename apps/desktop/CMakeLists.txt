find_package(Qt6 6.11 REQUIRED COMPONENTS Gui Quick LabsStyleKit)

qt_add_executable(runmark-desktop src/main.cpp)
# A static QML module registers its types through its plugin target; without
# it the engine reports "No module named Runmark.Shell".
target_link_libraries(runmark-desktop PRIVATE runmark::ui_shell runmark_ui_shellplugin Qt6::Quick
    MerceThemeplugin MerceFoundationplugin MerceStyleplugin MerceControlsplugin)

# Merce 1.2 accepts external profile sources only as filesystem paths.
foreach(profile_file themes/index.json themes/runmark.light.json themes/runmark.dark.json profiles/desktop.json)
    configure_file("${PROJECT_SOURCE_DIR}/libs/ui-shell/theme/${profile_file}"
        "${CMAKE_CURRENT_BINARY_DIR}/theme/${profile_file}" COPYONLY)
endforeach()
install(DIRECTORY "${PROJECT_SOURCE_DIR}/libs/ui-shell/theme/"
    DESTINATION "${CMAKE_INSTALL_BINDIR}/theme")

qt_import_qml_plugins(runmark-desktop)

install(TARGETS runmark-desktop RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(BUILD_TESTING)
    find_package(Qt6 6.11 REQUIRED COMPONENTS Test)
    qt_add_executable(desktop_test ${PROJECT_SOURCE_DIR}/libs/ui-shell/tests/tst_desktop.cpp)
    set_target_properties(desktop_test PROPERTIES AUTOMOC ON)
    target_link_libraries(desktop_test PRIVATE runmark::ui_shell runmark_ui_shellplugin
        MerceThemeplugin MerceFoundationplugin MerceStyleplugin MerceControlsplugin Qt6::Test)
    target_compile_definitions(desktop_test PRIVATE
        DESKTOP_THEME_INDEX="${CMAKE_CURRENT_BINARY_DIR}/theme/themes/index.json"
        DESKTOP_CAPTURE_DIR="${CMAKE_BINARY_DIR}/Testing/desktop-captures")
    qt_import_qml_plugins(desktop_test)
    add_test(NAME desktop_project_workflow COMMAND desktop_test)
    set_tests_properties(desktop_project_workflow PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software" TIMEOUT 30)
    add_test(NAME desktop_smoke COMMAND $<TARGET_FILE:runmark-desktop>
        -platform offscreen --project ${PROJECT_SOURCE_DIR}/.runmark/project.json)
    set_tests_properties(desktop_smoke PROPERTIES
        ENVIRONMENT "RUNMARK_SMOKE=1;QT_QPA_PLATFORM=offscreen"
        TIMEOUT 30
        PASS_REGULAR_EXPRESSION "smoke: project=.* findings=[0-9]+ error=none profile=desktop control=32"
        FAIL_REGULAR_EXPRESSION "QQmlApplicationEngine failed;is not a type;module .* is not installed;TypeError;ReferenceError;theme initialization failed;profile check failed")
endif()
