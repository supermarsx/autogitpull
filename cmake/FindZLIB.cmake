# Autogitpull custom FindZLIB: if a ZLIB::ZLIB target already exists (from our
# bundled build), reuse it so dependent subprojects link by target instead of
# hard-coded archives. This avoids duplicate zlib builds on Windows and keeps
# multi-config generators happy.

if(TARGET ZLIB::ZLIB)
  get_target_property(_autogitpull_zlib_includes ZLIB::ZLIB INTERFACE_INCLUDE_DIRECTORIES)
  if(NOT _autogitpull_zlib_includes)
    set(_autogitpull_zlib_includes "")
  endif()
  set(ZLIB_INCLUDE_DIR "${_autogitpull_zlib_includes}")
  set(ZLIB_INCLUDE_DIRS "${_autogitpull_zlib_includes}")
  set(ZLIB_LIBRARY ZLIB::ZLIB)
  set(ZLIB_LIBRARIES ZLIB::ZLIB)
  if(NOT ZLIB_VERSION AND DEFINED AUTOGITPULL_ZLIB_VERSION)
    set(ZLIB_VERSION "${AUTOGITPULL_ZLIB_VERSION}")
  endif()
  set(ZLIB_FOUND TRUE)
  mark_as_advanced(ZLIB_INCLUDE_DIR ZLIB_INCLUDE_DIRS ZLIB_LIBRARY ZLIB_LIBRARIES)
  unset(_autogitpull_zlib_includes)
  return()
endif()

include("${CMAKE_ROOT}/Modules/FindZLIB.cmake")
