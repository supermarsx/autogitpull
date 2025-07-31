#ifndef AUTOGITPULL_VERSION_HPP
#define AUTOGITPULL_VERSION_HPP

/* ------------------------------------------------------------------
   Public numeric version macros (safe for .rc files)               */
#define AUTOGITPULL_VERSION_MAJOR 0
#define AUTOGITPULL_VERSION_MINOR 0
#define AUTOGITPULL_VERSION_PATCH 0

/*
 * Rolling release tag injected by the CI workflow.
 * Example format: "2025.07.31-1".
 */
#define AUTOGITPULL_VERSION_STR "rolling"
#define AUTOGITPULL_VERSION_RC                                                                     \
    AUTOGITPULL_VERSION_MAJOR, AUTOGITPULL_VERSION_MINOR, AUTOGITPULL_VERSION_PATCH, 0
/* ------------------------------------------------------------------ */

/* Everything below is **C++-only**.  Keep it out of windres runs.   */
#ifndef RC_INVOKED
/* Human-friendly version string for the C++ codebase */
constexpr const char* AUTOGITPULL_VERSION = AUTOGITPULL_VERSION_STR;
#endif /* RC_INVOKED */

#endif /* AUTOGITPULL_VERSION_HPP */
