# Domain purity is a check, not a claim. TC-009 forbids I/O and TC-012 forbids
# QObject; both hold only if breaking them shows up here, the moment it happens.
file(GLOB_RECURSE sources "${DIR}/src/*.cpp" "${DIR}/src/*.h" "${DIR}/include/*.h")
set(forbidden "QFile|QProcess|QDir|QTextStream|QDateTime::current|Q_OBJECT|Q_GADGET")
set(violations "")
foreach(file IN LISTS sources)
    file(READ "${file}" text)
    # Comments do not count: a comment explaining the ban is not a violation.
    string(REGEX REPLACE "//[^\n]*" "" text "${text}")
    if(text MATCHES "${forbidden}")
        list(APPEND violations "${file}")
    endif()
endforeach()
if(violations)
    message(FATAL_ERROR "domain purity broken (${forbidden}): ${violations}")
endif()
message(STATUS "domain is pure: no ${forbidden}")
