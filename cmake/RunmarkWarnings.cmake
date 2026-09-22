# One warning set, linked by every target we compile. Kept as an INTERFACE
# target rather than global flags so a dependency added later does not inherit
# them (TC-011).
#
# The flags come from cforgo's CforgoWarnings module. cforgo itself is not a
# dependency here: it lives in a private GitLab project while runmark is
# public, so requiring it would leave anyone without that access unable to
# build, and CI unable to run without a credential. Four flags are cheaper
# than that coupling. If cforgo is ever published, this file becomes a call
# to cforgo_create_options_target().
include_guard(GLOBAL)

add_library(runmark_warnings INTERFACE)
add_library(runmark::warnings ALIAS runmark_warnings)

if(MSVC)
    target_compile_options(runmark_warnings INTERFACE /W4 /we4716)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(runmark_warnings INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Werror=return-type)
endif()

# An in-source build scatters generated files through the tree and makes
# "delete the build directory" mean "lose work".
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Runmark must be configured out of source: use a separate build directory.")
endif()
