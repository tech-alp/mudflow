# TC-012: apps/cli must never link Qt Quick, QML or GUI. Declaring the boundary
# in CMake is not enough -- a transitive PUBLIC link would pull them in without
# anyone editing this file, so the shipped binary is what gets inspected.
if(NOT APPLE)
    message(STATUS "no_gui_dependency: skipped, only implemented for otool")
    return()
endif()
execute_process(COMMAND otool -L "${BINARY}" OUTPUT_VARIABLE linked RESULT_VARIABLE failed)
if(failed)
    message(FATAL_ERROR "cannot inspect ${BINARY}")
endif()
foreach(framework QtQuick QtQml QtGui QtWidgets)
    if(linked MATCHES "${framework}\\.framework")
        message(FATAL_ERROR "apps/cli links ${framework}; the CLI must stay headless")
    endif()
endforeach()
message(STATUS "no_gui_dependency: rmk links no GUI framework")
