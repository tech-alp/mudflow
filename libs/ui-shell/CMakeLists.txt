# The QML bridge. Depends on the application layer, never the other way round;
# apps/cli must not reach any of this (TC-012).
find_package(Qt6 6.11 REQUIRED COMPONENTS Gui Quick Qml)

# Defined by Qt6Qml, so they can only be set once that module is found.
# QTP0001: ':/qt/qml/' is the resource prefix for QML modules -- the engine
#   searches nowhere else, so the OLD default of '/' makes the module
#   unfindable at runtime with no build-time error.
# QTP0004: generate qmldir files for QML subdirectories. None today; set NEW
#   so adding one later does not silently produce an unusable module.
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

qt_add_library(runmark_ui_shell STATIC)
# Target-scoped, not global: a directory-wide AUTOMOC would leak into whatever
# else gets added here later (TC-011).
set_target_properties(runmark_ui_shell PROPERTIES AUTOMOC ON)
add_library(runmark::ui_shell ALIAS runmark_ui_shell)

# qt_add_qml_module warns unless the output directory ends in the module's
# target path; without it qmllint and friends cannot find the module.
set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml)

qt_add_qml_module(runmark_ui_shell
    URI Runmark.Shell
    VERSION 1.0
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
