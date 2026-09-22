# Third-party dependencies (TC-011). Only pulled in when the UI is built: a
# CLI-only configure must not touch the network.
include_guard(GLOBAL)

include(FetchContent)

# Merce is the desktop design system (ADR-017). Embedded, not installed:
# its QML modules travel inside the binary instead of as separate plugins
# beside it, which is why cart/app embeds it the same way.
block(SCOPE_FOR VARIABLES)
    # Merce declares its QML modules with qt_add_qml_module and no explicit
    # STATIC/SHARED, so they follow BUILD_SHARED_LIBS. Static keeps the QML
    # inside the executable: no import path, no plugin directory at runtime.
    # Honoured only because QTP0003 is NEW (set by qt_standard_project_setup).
    set(BUILD_SHARED_LIBS OFF)
    set(MERCE_BUILD_NOTIFICATIONS ${RUNMARK_MERCE_NOTIFICATIONS})
    set(MERCE_ENABLE_FONTAWESOME OFF)

    FetchContent_Declare(Merce
        GIT_REPOSITORY https://github.com/tech-alp/Merce.git
        # The commit, not the tag object: v1.2.0 is annotated, so ls-remote gives
        # 994b509 for the tag and 2e587eb for what it points at. Pinning the
        # tag object would still resolve, but the commit is what we reviewed.
        GIT_TAG 2e587eb9f5cfc2f3456508646e22d151f45f0b55   # v1.2.0
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(Merce)
endblock()

FetchContent_GetProperties(Merce)
