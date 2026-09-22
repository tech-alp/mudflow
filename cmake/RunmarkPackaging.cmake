# CPack settings. Kept out of the root file so that reading it answers
# "what do we build", not "how do we ship it".
include_guard(GLOBAL)

set(CPACK_GENERATOR TGZ)
set(CPACK_PACKAGE_NAME runmark)
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "runmark-${PROJECT_VERSION}-macos-arm64")
include(CPack)
