# The QML bridge. Depends on the application layer, never the other way round;
# apps/cli must not reach any of this (TC-012).
find_package(Qt6 6.11 REQUIRED COMPONENTS Gui Quick Qml LabsStyleKit)

# Called here, not at the root. It is a macro: CMAKE_AUTOMOC/AUTOUIC and the
# Qt policies land in the calling directory scope and are inherited by every
# subdirectory added after it. At the root that would turn Qt's moc machinery
# on for domain, infrastructure, application and the CLI -- none of which may
# contain a Q_OBJECT (domain_purity, cli_no_gui). Measured: 16 autogen targets
# from the root versus 3 from here. REQUIRES pins the Qt policies, including
# QTP0001 (QML resource prefix :/qt/qml) and QTP0003 (BUILD_SHARED_LIBS,
# needed to embed Merce statically in RM-8).
qt_standard_project_setup(REQUIRES 6.11)

# Merce is fetched here, not at the root: a CLI-only configure must not reach
# the network (TC-011). It has to come AFTER qt_standard_project_setup, which
# sets QTP0003 -- that policy is what makes Merce's qt_add_qml_module targets
# honour BUILD_SHARED_LIBS and link statically.
include(RunmarkDependencies)

qt_add_library(runmark_ui_shell STATIC)
add_library(runmark::ui_shell ALIAS runmark_ui_shell)

# qt_add_qml_module warns unless the output directory ends in the module's
# target path; without it qmllint and friends cannot find the module.
set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml)

qt_add_qml_module(runmark_ui_shell
    URI Runmark.Shell
    VERSION 1.0
    IMPORT_PATH "${merce_BINARY_DIR}/qml"
    SOURCES
        src/FindingModel.cpp
        src/FindingModel.h
        src/StatusViewModel.cpp
        src/StatusViewModel.h
    QML_FILES
        Shell.qml
)

target_include_directories(runmark_ui_shell PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)
target_link_libraries(runmark_ui_shell PUBLIC runmark::application Qt6::Quick Qt6::Qml
    Merce::Theme Merce::Foundation Merce::Style Merce::Controls)
# qt_add_library turns module scanning off; StatusViewModel.cpp imports
# runmark.domain through workflow.h, so turn it back on for this target.
set_property(TARGET runmark_ui_shell PROPERTY CXX_SCAN_FOR_MODULES ON)
