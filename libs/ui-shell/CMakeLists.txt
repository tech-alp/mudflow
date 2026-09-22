# The QML bridge. Depends on the application layer, never the other way round;
# apps/cli must not reach any of this (TC-012).
find_package(Qt6 6.11 REQUIRED COMPONENTS Gui Quick Qml)

qt_add_library(runmark_ui_shell STATIC)
# Target-scoped, not global: a directory-wide AUTOMOC would leak into whatever
# else gets added here later (TC-011).
set_target_properties(runmark_ui_shell PROPERTIES AUTOMOC ON)
add_library(runmark::ui_shell ALIAS runmark_ui_shell)

qt_add_qml_module(runmark_ui_shell
    URI Runmark.Shell
    VERSION 1.0
    # Without qt_standard_project_setup() the default resource prefix is "/",
    # but the engine only searches qrc:/qt/qml. Set it here rather than calling
    # the directory-wide helper, which would also flip AUTOMOC for everything
    # added later (TC-011).
    RESOURCE_PREFIX "/qt/qml"
    SOURCES
        src/FindingModel.cpp
        src/FindingModel.h
        src/StatusViewModel.cpp
        src/StatusViewModel.h
    QML_FILES
        Shell.qml
)

target_include_directories(runmark_ui_shell PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)
target_link_libraries(runmark_ui_shell PUBLIC runmark::application Qt6::Quick Qt6::Qml)
# qt_add_library turns module scanning off; StatusViewModel.cpp imports
# runmark.domain through workflow.h, so turn it back on for this target.
set_property(TARGET runmark_ui_shell PROPERTY CXX_SCAN_FOR_MODULES ON)
