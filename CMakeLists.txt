cmake_minimum_required(VERSION 3.20)
project(autogitpull LANGUAGES C CXX)

# Ensure all static libs are PIC-friendly on ELF platforms
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(BUILD_SHARED_LIBS OFF)
include(CTest)
if(MSVC)
    # Link the static runtime to satisfy "static everything" on Windows
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
# Ensure zlib static archive is built with PIC
if(_autogitpull_zlib_target)
  set_property(TARGET ${_autogitpull_zlib_target} PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckLinkerFlag)

function(_filter_supported_flags _lang _out_var)
  set(_filtered "")
  foreach(_flag IN LISTS ARGN)
    string(STRIP "${_flag}" _flag)
    if(_flag STREQUAL "")
      continue()
    endif()
    set(_ok FALSE)
    if(_lang STREQUAL "C")
      check_c_compiler_flag("${_flag}" _ok)
    elseif(_lang STREQUAL "CXX")
      check_cxx_compiler_flag("${_flag}" _ok)
    elseif(_lang STREQUAL "LD")
      # Probe linker flag using C as the language context
      check_linker_flag(C "${_flag}" _ok)
    else()
      message(FATAL_ERROR "Unknown language for flag filtering: ${_lang}")
    endif()
    if(_ok)
      list(APPEND _filtered "${_flag}")
    endif()
    unset(_ok CACHE)
  endforeach()
  set(${_out_var} "${_filtered}" PARENT_SCOPE)
endfunction()

# --- BEGIN: flag normalization (treat env flags sanely) ---
if(DEFINED ENV{CFLAGS})
  separate_arguments(_ENV_CFLAGS NATIVE_COMMAND "$ENV{CFLAGS}")
  _filter_supported_flags(C _ENV_CFLAGS_OK ${_ENV_CFLAGS})
  add_compile_options($<$<COMPILE_LANGUAGE:C>:${_ENV_CFLAGS_OK}>)
endif()
if(DEFINED ENV{CXXFLAGS})
  separate_arguments(_ENV_CXXFLAGS NATIVE_COMMAND "$ENV{CXXFLAGS}")
  _filter_supported_flags(CXX _ENV_CXXFLAGS_OK ${_ENV_CXXFLAGS})
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:${_ENV_CXXFLAGS_OK}>)
endif()
if(DEFINED ENV{LDFLAGS})
  separate_arguments(_ENV_LDFLAGS NATIVE_COMMAND "$ENV{LDFLAGS}")
  _filter_supported_flags(LD _ENV_LDFLAGS_OK ${_ENV_LDFLAGS})
  add_link_options(${_ENV_LDFLAGS_OK})
endif()

# --- END: flag normalization ---

# Make option() honor cache values in subprojects
cmake_policy(SET CMP0077 NEW)

# (moved) POSIX feature macro scrubbing is scoped to zlib only

# Compiler warning tuning: add only if the compiler supports them
function(_add_supported_warning_flags _lang)
  foreach(_f IN LISTS ARGN)
    if(_lang STREQUAL "C")
      check_c_compiler_flag("${_f}" _ok)
      if(_ok)
        add_compile_options($<$<COMPILE_LANGUAGE:C>:${_f}>)
      endif()
    elseif(_lang STREQUAL "CXX")
      check_cxx_compiler_flag("${_f}" _ok)
      if(_ok)
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:${_f}>)
      endif()
    endif()
    unset(_ok CACHE)
  endforeach()
endfunction()

# Suppress noisy warnings when supported by the active compiler
# GCC supports -Wno-maybe-uninitialized; Clang does not. Probe first.
_add_supported_warning_flags(C    "-Wno-maybe-uninitialized" "-Wno-missing-declarations")
_add_supported_warning_flags(CXX  "-Wno-maybe-uninitialized" "-Wno-missing-declarations")

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wno-documentation-deprecated-sync>)
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-documentation-deprecated-sync>)
endif()

include(FetchContent)

# Prefer letting libgit2 build with its bundled zlib to avoid
# clashes and system libz.a (non-PIC) on Linux.
set(AUTOGITPULL_EMBED_ZLIB ON)
set(_autogitpull_use_system_zlib_default ON)
if(WIN32)
    set(_autogitpull_use_system_zlib_default OFF)
endif()
option(AUTOGITPULL_USE_SYSTEM_ZLIB "Use a pre-installed zlib instead of the bundled static build" ${_autogitpull_use_system_zlib_default})
unset(_autogitpull_use_system_zlib_default)

set(_autogitpull_app_zlib_default OFF)
if(WIN32)
    set(_autogitpull_app_zlib_default ON)
endif()
option(AUTOGITPULL_APP_BUNDLED_ZLIB "Fetch and build a bundled zlib for autogitpull features" ${_autogitpull_app_zlib_default})
unset(_autogitpull_app_zlib_default)

set(_autogitpull_reuse_libgit2_zlib OFF)
if(WIN32 AND AUTOGITPULL_APP_BUNDLED_ZLIB)
    set(_autogitpull_reuse_libgit2_zlib ON)
endif()

if(AUTOGITPULL_USE_SYSTEM_ZLIB)
    set(ZLIB_USE_STATIC_LIBS OFF)
else()
    set(ZLIB_USE_STATIC_LIBS ON)
endif()

if(AUTOGITPULL_USE_SYSTEM_LIBGIT2 AND NOT AUTOGITPULL_USE_SYSTEM_ZLIB)
    set(AUTOGITPULL_USE_SYSTEM_ZLIB ON CACHE BOOL "" FORCE)
endif()

set(_autogitpull_zlib_target "")
if(AUTOGITPULL_USE_SYSTEM_ZLIB)
    find_package(ZLIB QUIET)
    if(NOT ZLIB_FOUND)
        set(_autogitpull_zlib_search_paths)
        if(DEFINED CMAKE_OSX_SYSROOT)
            list(APPEND _autogitpull_zlib_search_paths "${CMAKE_OSX_SYSROOT}/usr/lib")
            list(APPEND _autogitpull_zlib_search_paths "${CMAKE_OSX_SYSROOT}/System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks")
        endif()
        list(APPEND _autogitpull_zlib_search_paths "/usr/lib")
        find_library(ZLIB_LIBRARY z NAMES z zlib PATHS ${_autogitpull_zlib_search_paths})
        if(ZLIB_LIBRARY)
            find_path(ZLIB_INCLUDE_DIR zlib.h
                PATHS
                    ${CMAKE_OSX_SYSROOT}/usr/include
                    ${CMAKE_OSX_SYSROOT}/usr/include/zlib
                    /usr/include
                    /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include
                    /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/zlib)
            if(ZLIB_INCLUDE_DIR)
                set(ZLIB_FOUND TRUE)
                set(ZLIB_LIBRARIES ${ZLIB_LIBRARY})
                set(ZLIB_INCLUDE_DIRS ${ZLIB_INCLUDE_DIR})
            endif()
        endif()
        unset(_autogitpull_zlib_search_paths)
    endif()
    if(NOT ZLIB_FOUND)
        message(FATAL_ERROR "System zlib not found; install zlib or disable AUTOGITPULL_USE_SYSTEM_ZLIB")
    endif()
    if(NOT WIN32 AND ZLIB_LIBRARY MATCHES "\\.a$")
        get_filename_component(_autogitpull_zlib_dir "${ZLIB_LIBRARY}" DIRECTORY)
        find_library(_autogitpull_zlib_shared
            NAMES z zlib
            PATHS "${_autogitpull_zlib_dir}"
            NO_DEFAULT_PATH)
        if(_autogitpull_zlib_shared AND NOT _autogitpull_zlib_shared MATCHES "\\.a$")
            set(ZLIB_LIBRARY "${_autogitpull_zlib_shared}" CACHE FILEPATH "" FORCE)
            set(ZLIB_LIBRARIES "${_autogitpull_zlib_shared}" CACHE STRING "" FORCE)
        endif()
        unset(_autogitpull_zlib_dir)
        unset(_autogitpull_zlib_shared)
    endif()
    if(NOT TARGET ZLIB::ZLIB)
        add_library(ZLIB::ZLIB UNKNOWN IMPORTED)
        set_target_properties(ZLIB::ZLIB PROPERTIES
            IMPORTED_LOCATION "${ZLIB_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${ZLIB_INCLUDE_DIR}")
    endif()
elseif(AUTOGITPULL_APP_BUNDLED_ZLIB AND NOT _autogitpull_reuse_libgit2_zlib)
    message(STATUS "Configuring bundled static zlib for autogitpull")
    FetchContent_Declare(
        autogitpull_zlib
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG v1.3.1
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_C_FLAGS=-w
    )
    set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(autogitpull_zlib)
    if(TARGET zlibstatic)
        set(_autogitpull_zlib_target zlibstatic)
        if(NOT TARGET ZLIB::ZLIB)
            add_library(ZLIB::ZLIB ALIAS zlibstatic)
        endif()
    elseif(TARGET zlib)
        get_target_property(_autogitpull_zlib_type zlib TYPE)
        set(_autogitpull_zlib_is_static FALSE)
        if(_autogitpull_zlib_type STREQUAL "STATIC_LIBRARY" OR _autogitpull_zlib_type STREQUAL "OBJECT_LIBRARY")
            set(_autogitpull_zlib_is_static TRUE)
        elseif(_autogitpull_zlib_type STREQUAL "UNKNOWN")
            get_target_property(_autogitpull_zlib_location zlib LOCATION)
            if(_autogitpull_zlib_location MATCHES "\.(a|lib)$")
                set(_autogitpull_zlib_is_static TRUE)
            endif()
            unset(_autogitpull_zlib_location)
        endif()
        if(_autogitpull_zlib_is_static AND _autogitpull_zlib_type STREQUAL "OBJECT_LIBRARY")
            add_library(autogitpull_zlib_static STATIC)
            target_sources(autogitpull_zlib_static PRIVATE $<TARGET_OBJECTS:zlib>)
            target_include_directories(autogitpull_zlib_static PUBLIC ${autogitpull_zlib_SOURCE_DIR})
            set_target_properties(autogitpull_zlib_static PROPERTIES LINKER_LANGUAGE C OUTPUT_NAME autogitpull_zlib)
            set(_autogitpull_zlib_target autogitpull_zlib_static)
            if(NOT TARGET ZLIB::ZLIB)
                add_library(ZLIB::ZLIB ALIAS autogitpull_zlib_static)
            endif()
        elseif(_autogitpull_zlib_is_static)
            set(_autogitpull_zlib_target zlib)
            if(NOT TARGET ZLIB::ZLIB)
                add_library(ZLIB::ZLIB ALIAS zlib)
            endif()
        else()
            get_target_property(_autogitpull_zlib_location zlib LOCATION)
            if(NOT TARGET ZLIB::ZLIB)
                add_library(ZLIB::ZLIB UNKNOWN IMPORTED)
            endif()
            set_target_properties(ZLIB::ZLIB PROPERTIES
                IMPORTED_LOCATION "${_autogitpull_zlib_location}"
                INTERFACE_INCLUDE_DIRECTORIES "${autogitpull_zlib_SOURCE_DIR}")
            unset(_autogitpull_zlib_location)
        endif()
        unset(_autogitpull_zlib_is_static)
    endif()
    if(NOT TARGET ZLIB::ZLIB)
        message(FATAL_ERROR "Bundled zlib did not produce ZLIB::ZLIB target")
    endif()
    if(_autogitpull_zlib_target)
        # Scope any feature-macro scrubbing to zlib only (avoid breaking feature tests elsewhere)
        target_compile_options(${_autogitpull_zlib_target} PRIVATE -U_POSIX_C_SOURCE -U_XOPEN_SOURCE)
        get_target_property(_autogitpull_zlib_include_dirs ${_autogitpull_zlib_target} INTERFACE_INCLUDE_DIRECTORIES)
        if(NOT _autogitpull_zlib_include_dirs)
            set(_autogitpull_zlib_include_dirs "${autogitpull_zlib_SOURCE_DIR}")
        endif()
        get_target_property(_autogitpull_zlib_output ${_autogitpull_zlib_target} OUTPUT_NAME)
        if(NOT _autogitpull_zlib_output)
            set(_autogitpull_zlib_output ${_autogitpull_zlib_target})
        endif()
        get_target_property(_autogitpull_zlib_archive_dir ${_autogitpull_zlib_target} ARCHIVE_OUTPUT_DIRECTORY)
        if(NOT _autogitpull_zlib_archive_dir)
            set(_autogitpull_zlib_archive_dir "${autogitpull_zlib_BINARY_DIR}")
        endif()
        get_target_property(_autogitpull_zlib_archive_dir_release ${_autogitpull_zlib_target} ARCHIVE_OUTPUT_DIRECTORY_RELEASE)
        if(_autogitpull_zlib_archive_dir_release)
            set(_autogitpull_zlib_release_dir ${_autogitpull_zlib_archive_dir_release})
        elseif(CMAKE_CONFIGURATION_TYPES)
            set(_autogitpull_zlib_release_dir "${autogitpull_zlib_BINARY_DIR}/Release")
        else()
            set(_autogitpull_zlib_release_dir ${_autogitpull_zlib_archive_dir})
        endif()
        get_target_property(_autogitpull_zlib_archive_dir_debug ${_autogitpull_zlib_target} ARCHIVE_OUTPUT_DIRECTORY_DEBUG)
        if(_autogitpull_zlib_archive_dir_debug)
            set(_autogitpull_zlib_debug_dir ${_autogitpull_zlib_archive_dir_debug})
        elseif(CMAKE_CONFIGURATION_TYPES)
            set(_autogitpull_zlib_debug_dir "${autogitpull_zlib_BINARY_DIR}/Debug")
        else()
            set(_autogitpull_zlib_debug_dir ${_autogitpull_zlib_archive_dir})
        endif()
        get_target_property(_autogitpull_zlib_debug_postfix ${_autogitpull_zlib_target} DEBUG_POSTFIX)
        if(NOT _autogitpull_zlib_debug_postfix)
            set(_autogitpull_zlib_debug_postfix "${CMAKE_DEBUG_POSTFIX}")
        endif()
        if(NOT _autogitpull_zlib_debug_postfix)
            set(_autogitpull_zlib_debug_postfix "")
        endif()
        set(_autogitpull_zlib_release_lib "${_autogitpull_zlib_release_dir}/${CMAKE_STATIC_LIBRARY_PREFIX}${_autogitpull_zlib_output}${CMAKE_STATIC_LIBRARY_SUFFIX}")
        set(_autogitpull_zlib_debug_lib "${_autogitpull_zlib_debug_dir}/${CMAKE_STATIC_LIBRARY_PREFIX}${_autogitpull_zlib_output}${_autogitpull_zlib_debug_postfix}${CMAKE_STATIC_LIBRARY_SUFFIX}")
        set(ZLIB_INCLUDE_DIR "${_autogitpull_zlib_include_dirs}" CACHE STRING "" FORCE)
        set(ZLIB_INCLUDE_DIRS "${_autogitpull_zlib_include_dirs}" CACHE STRING "" FORCE)
        if(CMAKE_CONFIGURATION_TYPES)
            set(ZLIB_LIBRARY_RELEASE "${_autogitpull_zlib_release_lib}" CACHE FILEPATH "" FORCE)
            if(_autogitpull_zlib_debug_postfix STREQUAL "")
                set(ZLIB_LIBRARY_DEBUG "${_autogitpull_zlib_release_lib}" CACHE FILEPATH "" FORCE)
                set(_autogitpull_zlib_multi_config "optimized;${ZLIB_LIBRARY_RELEASE};debug;${ZLIB_LIBRARY_RELEASE}")
            else()
                set(ZLIB_LIBRARY_DEBUG "${_autogitpull_zlib_debug_lib}" CACHE FILEPATH "" FORCE)
                set(_autogitpull_zlib_multi_config "optimized;${ZLIB_LIBRARY_RELEASE};debug;${ZLIB_LIBRARY_DEBUG}")
            endif()
            set(ZLIB_LIBRARIES "${_autogitpull_zlib_multi_config}" CACHE STRING "" FORCE)
            set(ZLIB_LIBRARY "${ZLIB_LIBRARIES}" CACHE STRING "" FORCE)
            unset(_autogitpull_zlib_multi_config)
        else()
            set(ZLIB_LIBRARY "${_autogitpull_zlib_release_lib}" CACHE STRING "" FORCE)
            set(ZLIB_LIBRARIES "${ZLIB_LIBRARY}" CACHE STRING "" FORCE)
        endif()
        set(ZLIB_FOUND TRUE CACHE BOOL "" FORCE)
    endif()
    if(_autogitpull_zlib_target)
        set(_autogitpull_zlib_primary_target "${_autogitpull_zlib_target}")
    endif()
elseif(_autogitpull_reuse_libgit2_zlib)
    message(STATUS "Reusing libgit2 bundled zlib for autogitpull targets on Windows")
else()
    message(STATUS "Skipping app-level zlib configuration; expecting external linkage")
endif()
if(TARGET ZLIB::ZLIB)
    get_property(_autogitpull_zlib_is_alias TARGET ZLIB::ZLIB PROPERTY ALIASED_TARGET SET)
    if(NOT _autogitpull_zlib_is_alias)
        set_property(TARGET ZLIB::ZLIB PROPERTY IMPORTED_LOCATION "${ZLIB_LIBRARY}")
    endif()
    unset(_autogitpull_zlib_is_alias)
endif()
unset(_autogitpull_zlib_target)
unset(_autogitpull_zlib_include_dirs)
unset(_autogitpull_zlib_output)
unset(_autogitpull_zlib_archive_dir)
unset(_autogitpull_zlib_archive_dir_release)
unset(_autogitpull_zlib_archive_dir_debug)
unset(_autogitpull_zlib_release_dir)
unset(_autogitpull_zlib_debug_dir)
unset(_autogitpull_zlib_debug_postfix)
unset(_autogitpull_zlib_release_lib)
unset(_autogitpull_zlib_debug_lib)


unset(_autogitpull_zlib_primary_target)

find_package(yaml-cpp CONFIG QUIET)
if(NOT yaml-cpp_FOUND)
    message(STATUS "yaml-cpp not found, fetching via FetchContent...")
    FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG 4fe2fb83fe77c3ebc49f56ef3a1fa24c77688d84
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        CMAKE_ARGS
            -DYAML_BUILD_SHARED_LIBS=OFF
            -DYAML_CPP_BUILD_TESTS=OFF
            -DYAML_CPP_BUILD_TOOLS=OFF
    )
    FetchContent_MakeAvailable(yaml-cpp)
endif()

find_package(nlohmann_json 3 CONFIG QUIET)
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found, fetching via FetchContent...")
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.12.0
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()




set(LIBGIT2_TARGET "")
set(LIBGIT2_DEPENDENCIES "")


option(AUTOGITPULL_USE_SYSTEM_LIBGIT2 "Use a pre-installed libgit2 instead of the bundled static build" OFF)
if(AUTOGITPULL_USE_SYSTEM_LIBGIT2)
    find_package(libgit2 CONFIG REQUIRED)
    if(TARGET libgit2::libgit2)
        set(LIBGIT2_TARGET libgit2::libgit2)
    elseif(TARGET libgit2::libgit2package)
        set(LIBGIT2_TARGET libgit2::libgit2package)
    else()
        message(FATAL_ERROR "libgit2 package found but no usable target exported")
    endif()

    set(_libgit2_is_static FALSE)
    if(LIBGIT2_TARGET)
        get_target_property(_libgit2_type ${LIBGIT2_TARGET} TYPE)
        if(_libgit2_type STREQUAL "STATIC_LIBRARY")
            set(_libgit2_is_static TRUE)
        else()
            get_target_property(_libgit2_location ${LIBGIT2_TARGET} IMPORTED_LOCATION_${CMAKE_BUILD_TYPE})
            if(NOT _libgit2_location)
                get_target_property(_libgit2_location ${LIBGIT2_TARGET} IMPORTED_LOCATION_RELEASE)
            endif()
            if(NOT _libgit2_location)
                get_target_property(_libgit2_location ${LIBGIT2_TARGET} IMPORTED_LOCATION)
            endif()
            if(_libgit2_location MATCHES "\.(a|lib)$")
                set(_libgit2_is_static TRUE)
            endif()
        endif()
    endif()
    if(NOT _libgit2_is_static)
        message(FATAL_ERROR "autogitpull requires a static libgit2; disable AUTOGITPULL_USE_SYSTEM_LIBGIT2 or provide a static archive")
    endif()
    set(ZLIB_USE_STATIC_LIBS TRUE)
    find_package(ZLIB REQUIRED)
endif()

if(NOT LIBGIT2_TARGET)
    set(_autogitpull_libgit2_args
        -DBUILD_SHARED_LIBS:BOOL=OFF
        -DBUILD_TESTS:BOOL=OFF
        -DBUILD_TESTING:BOOL=OFF
        -DBUILD_CLAR:BOOL=OFF
        -DBUILD_CLI:BOOL=OFF
        -DBUILD_EXAMPLES:BOOL=OFF
        -DBUILD_FUZZERS:BOOL=OFF
        -DBUILD_PROGRAMS:BOOL=OFF
        -DUSE_SSH:BOOL=OFF
        -DUSE_NTLMCLIENT:BOOL=OFF
        -DUSE_NTLM:BOOL=OFF
        -DENABLE_NTLM:BOOL=OFF
        -DNTLMCLIENT_FOUND:BOOL=OFF
        -DUSE_REGEX=builtin
        -DUSE_COMPRESSION=zlib
        -DLINK_WITH_STATIC_LIBRARIES:BOOL=ON
        -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON
        -DCMAKE_POLICY_DEFAULT_CMP0077:STRING=NEW
    )


    if(TARGET ZLIB::ZLIB)
        list(APPEND _autogitpull_libgit2_args
            -DUSE_BUNDLED_ZLIB:BOOL=OFF
            -DCMAKE_DISABLE_FIND_PACKAGE_ZLIB:BOOL=FALSE
            -DZLIB_FOUND:BOOL=ON
            -DZLIB_LIBRARY:STRING=${ZLIB_LIBRARY}
            -DZLIB_INCLUDE_DIR:PATH=${ZLIB_INCLUDE_DIR})
        if(DEFINED ZLIB_LIBRARY_RELEASE AND ZLIB_LIBRARY_RELEASE)
            list(APPEND _autogitpull_libgit2_args -DZLIB_LIBRARY_RELEASE:FILEPATH=${ZLIB_LIBRARY_RELEASE})
        endif()
        if(DEFINED ZLIB_LIBRARY_DEBUG AND ZLIB_LIBRARY_DEBUG)
            list(APPEND _autogitpull_libgit2_args -DZLIB_LIBRARY_DEBUG:FILEPATH=${ZLIB_LIBRARY_DEBUG})
        endif()
    else()
        list(APPEND _autogitpull_libgit2_args
            -DUSE_BUNDLED_ZLIB:BOOL=ON
            -DCMAKE_DISABLE_FIND_PACKAGE_ZLIB:BOOL=TRUE
            -DZLIB_FOUND:BOOL=OFF
            -DZLIB_LIBRARY:STRING=
            -DZLIB_INCLUDE_DIR:PATH=)
    endif()

    if(WIN32)
        # Work around MSVC/Windows struct stat nsec detection issues by disabling USE_NSEC.
        # This avoids: "GIT_USE_NSEC defined but unknown struct stat nanosecond type" during libgit2 build.
        list(APPEND _autogitpull_libgit2_args -DUSE_HTTPS=WinHTTP -DSTATIC_CRT=ON)
        list(APPEND LIBGIT2_DEPENDENCIES winhttp rpcrt4 crypt32 ole32 ws2_32 secur32 bcrypt)
        if(NOT TARGET ZLIB::ZLIB)
            set(USE_BUNDLED_ZLIB ON CACHE BOOL "" FORCE)
            set(CMAKE_DISABLE_FIND_PACKAGE_ZLIB TRUE CACHE BOOL "" FORCE)
            set(ZLIB_FOUND OFF CACHE BOOL "" FORCE)
            unset(ZLIB_LIBRARY CACHE)
            unset(ZLIB_LIBRARY_RELEASE CACHE)
            unset(ZLIB_LIBRARY_DEBUG CACHE)
            unset(ZLIB_LIBRARIES CACHE)
            unset(ZLIB_INCLUDE_DIR CACHE)
            unset(ZLIB_INCLUDE_DIRS CACHE)
        endif()
    elseif(APPLE)
        list(APPEND _autogitpull_libgit2_args -DUSE_HTTPS=SecureTransport)
    else()
        set(OPENSSL_USE_STATIC_LIBS TRUE)
        list(APPEND _autogitpull_libgit2_args -DUSE_HTTPS=OpenSSL -DOPENSSL_USE_STATIC_LIBS=ON)
        find_package(OpenSSL REQUIRED)
        list(APPEND LIBGIT2_DEPENDENCIES OpenSSL::SSL OpenSSL::Crypto)
    endif()

    # Prevent libgit2 from registering its own tests/examples/programs/CLI in our build
    # Use cache variables so the subproject's option() calls pick up our values.
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_CLAR OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_FUZZERS OFF CACHE BOOL "" FORCE)
    set(BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
    set(BUILD_CLI OFF CACHE BOOL "" FORCE)
    # Disable optional NTLM client to avoid missing ntlm_client_* symbols on Linux
    set(USE_NTLMCLIENT OFF CACHE BOOL "" FORCE)
    set(USE_NTLM OFF CACHE BOOL "" FORCE)
    set(ENABLE_NTLM OFF CACHE BOOL "" FORCE)
    set(NTLMCLIENT_FOUND OFF CACHE BOOL "" FORCE)
    # Force libgit2 to ignore any top-level zlib and use its bundled one
    # only when we have not already provisioned a ZLIB::ZLIB target. On
    # Windows we ship a standalone zlib for our binaries, and libgit2 should
    # reuse that archive instead of trying to add another `zlib` target.
    if(NOT TARGET ZLIB::ZLIB)
        set(ZLIB_FOUND OFF CACHE BOOL "" FORCE)
        unset(ZLIB_LIBRARY CACHE)
        unset(ZLIB_INCLUDE_DIR CACHE)
    endif()
    # Disable nanosecond mtimes at the subproject level for stability across toolchains
    set(USE_NSEC OFF CACHE BOOL "Disable nanosecond mtimes in libgit2" FORCE)
    FetchContent_Declare(
        autogitpull_libgit2
        GIT_REPOSITORY https://github.com/libgit2/libgit2.git
        GIT_TAG v1.9.1
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        CMAKE_ARGS ${_autogitpull_libgit2_args}
    )
    FetchContent_MakeAvailable(autogitpull_libgit2)

    if(DEFINED LIBGIT2_SYSTEM_LIBS)
        list(APPEND LIBGIT2_DEPENDENCIES ${LIBGIT2_SYSTEM_LIBS})
    endif()

    # Restore project-level testing setting; our tests are handled below
    set(BUILD_TESTING ON CACHE BOOL "" FORCE)

    set(_autogitpull_libgit2_build_target "")
    if(TARGET git2)
        set(_autogitpull_libgit2_build_target git2)
    elseif(TARGET libgit2)
        set(_autogitpull_libgit2_build_target libgit2)
    endif()

    if(NOT _autogitpull_libgit2_build_target)
        message(FATAL_ERROR "Failed to configure static libgit2")
    endif()

    set(LIBGIT2_TARGET ${_autogitpull_libgit2_build_target})
    if(DEFINED _autogitpull_zlib_primary_target AND _autogitpull_zlib_primary_target)
        if(TARGET ${_autogitpull_zlib_primary_target})
            add_dependencies(${_autogitpull_libgit2_build_target} ${_autogitpull_zlib_primary_target})
        endif()
    endif()
    if(NOT TARGET libgit2::libgit2)
        add_library(libgit2::libgit2 ALIAS ${_autogitpull_libgit2_build_target})
    endif()

    # Do not mutate libgit2's own targets with additional link interfaces;
    # we pass ZLIB paths via CMAKE_ARGS above so libgit2 wires itself
    # correctly. Avoid mixing keyword/plain signatures across subprojects.

    # Scope feature macros to libgit2; ensure no stray GIT_USE_NSEC leaks in
    foreach(_t libgit2 libgit2_static git2 ${_autogitpull_libgit2_build_target})
        if(TARGET ${_t})
            target_compile_definitions(${_t} PRIVATE _GNU_SOURCE _DARWIN_C_SOURCE)
            target_compile_options(${_t} PRIVATE -UGIT_USE_NSEC -UGIT_NTLM -UUSE_NTLMCLIENT)
            target_compile_definitions(${_t} PRIVATE
                GIT_USE_NSEC=0
                GIT_NTLM=0
                USE_NTLMCLIENT=0
                NTLMCLIENT_FOUND=0
                ENABLE_NTLM=0
                USE_NTLM=0)
        endif()
    endforeach()

    foreach(_libgit2_extra IN ITEMS util llhttp pcre xdiff)
        if(TARGET ${_libgit2_extra})
            list(APPEND LIBGIT2_DEPENDENCIES ${_libgit2_extra})
        endif()
    endforeach()

    if(NOT TARGET ZLIB::ZLIB)
        if(TARGET zlibstatic)
            add_library(ZLIB::ZLIB ALIAS zlibstatic)
        elseif(TARGET zlib)
            get_target_property(_autogitpull_zlib_type zlib TYPE)
            if(_autogitpull_zlib_type STREQUAL "OBJECT_LIBRARY")
                add_library(autogitpull_zlib STATIC)
                target_sources(autogitpull_zlib PRIVATE $<TARGET_OBJECTS:zlib>)
                target_include_directories(autogitpull_zlib PUBLIC ${autogitpull_libgit2_SOURCE_DIR}/deps/zlib)
                set_target_properties(autogitpull_zlib PROPERTIES LINKER_LANGUAGE C OUTPUT_NAME autogitpull_zlib)
                add_library(ZLIB::ZLIB ALIAS autogitpull_zlib)
            else()
                add_library(ZLIB::ZLIB ALIAS zlib)
            endif()
        endif()
    endif()
    if(NOT TARGET ZLIB::ZLIB)
        message(STATUS "libgit2 did not expose a zlib target; proceeding without top-level ZLIB::ZLIB")
    endif()
    if(_autogitpull_reuse_libgit2_zlib)
        if(TARGET zlibstatic)
            set(_autogitpull_reused_zlib_target zlibstatic)
        elseif(TARGET zlib)
            set(_autogitpull_reused_zlib_target zlib)
        endif()
        if(DEFINED _autogitpull_reused_zlib_target)
            get_target_property(_autogitpull_zlib_include_dirs ${_autogitpull_reused_zlib_target} INTERFACE_INCLUDE_DIRECTORIES)
            if(NOT _autogitpull_zlib_include_dirs)
                set(_autogitpull_zlib_include_dirs "${autogitpull_libgit2_SOURCE_DIR}/deps/zlib")
            endif()
            set(ZLIB_FOUND TRUE CACHE BOOL "" FORCE)
            set(ZLIB_LIBRARIES "ZLIB::ZLIB" CACHE STRING "" FORCE)
            set(ZLIB_LIBRARY "ZLIB::ZLIB" CACHE STRING "" FORCE)
            set(ZLIB_INCLUDE_DIR "${_autogitpull_zlib_include_dirs}" CACHE STRING "" FORCE)
            set(ZLIB_INCLUDE_DIRS "${_autogitpull_zlib_include_dirs}" CACHE STRING "" FORCE)
            unset(_autogitpull_zlib_include_dirs)
        else()
            message(FATAL_ERROR "Expected libgit2 to expose a zlib target, but none was found")
        endif()
        unset(_autogitpull_reused_zlib_target)
    endif()
    unset(_autogitpull_libgit2_build_target)
endif()
unset(OPENSSL_USE_STATIC_LIBS)
unset(_libgit2_is_static)
unset(_libgit2_type)
unset(_libgit2_location)
unset(_autogitpull_reuse_libgit2_zlib)

find_package(Threads REQUIRED)

if(APPLE)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
    find_library(CORESERVICES_FRAMEWORK CoreServices REQUIRED)
    find_library(SECURITY_FRAMEWORK Security)

    # FSEvents lives under CoreServices.framework; provide explicit hints for newer SDK layouts
    unset(_autogitpull_fsevents_hints)
    if(DEFINED CMAKE_OSX_SYSROOT)
        set(_autogitpull_sysroot_fsevents
            "${CMAKE_OSX_SYSROOT}/System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks")
        if(EXISTS "${_autogitpull_sysroot_fsevents}")
            list(APPEND _autogitpull_fsevents_hints "${_autogitpull_sysroot_fsevents}")
        endif()
    endif()
    if(EXISTS "/System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks")
        list(APPEND _autogitpull_fsevents_hints
            "/System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks")
    endif()
    if(_autogitpull_fsevents_hints)
        find_library(FSEVENTS_FRAMEWORK FSEvents
            HINTS ${_autogitpull_fsevents_hints}
            NO_DEFAULT_PATH)
    endif()
    # Fallback to standard search paths when explicit hints fail (older SDKs)
    find_library(FSEVENTS_FRAMEWORK FSEvents)
    unset(_autogitpull_fsevents_hints)
    unset(_autogitpull_sysroot_fsevents)

    if(NOT AUTOGITPULL_USE_SYSTEM_ZLIB)
        # Always link the macOS platform zlib to get gzip helpers for logging.
        find_library(AUTOGITPULL_APPLE_ZLIB z PATHS
            ${CMAKE_OSX_SYSROOT}/usr/lib
            /usr/lib
            NO_DEFAULT_PATH)
        if(NOT AUTOGITPULL_APPLE_ZLIB)
            find_library(AUTOGITPULL_APPLE_ZLIB z)
        endif()
        if(NOT AUTOGITPULL_APPLE_ZLIB)
            message(FATAL_ERROR "Failed to locate system zlib on macOS; install Command Line Tools or enable AUTOGITPULL_USE_SYSTEM_ZLIB")
        endif()
    endif()
endif()

add_library(autogitpull_lib STATIC
    src/git_utils.cpp
    src/logger.cpp
    src/resource_utils.cpp
    src/system_utils.cpp
    src/time_utils.cpp
    src/config_utils.cpp
    src/ignore_utils.cpp
    src/debug_utils.cpp
    src/help_text.cpp
    src/scanner.cpp
    src/file_watch.cpp
    src/ui_loop.cpp
    src/options.cpp
    src/parse_utils.cpp
    src/history_utils.cpp
    src/process_monitor.cpp
    src/cli_commands.cpp
    src/mutant_mode.cpp)
if(WIN32)
    target_sources(autogitpull_lib PRIVATE src/windows_service.cpp src/windows_commands.cpp src/lock_utils_windows.cpp src/linux_daemon.cpp)
elseif(APPLE)
    target_sources(autogitpull_lib PRIVATE src/macos_daemon.cpp src/linux_commands.cpp src/lock_utils_posix.cpp)
else()
    target_sources(autogitpull_lib PRIVATE src/linux_daemon.cpp src/linux_commands.cpp src/lock_utils_posix.cpp)
endif()
target_include_directories(autogitpull_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)
if(DEFINED LIBGIT2_INCLUDES)
    target_include_directories(autogitpull_lib PRIVATE ${LIBGIT2_INCLUDES})
endif()
if(DEFINED LIBGIT2_SYSTEM_INCLUDES)
    target_include_directories(autogitpull_lib SYSTEM PRIVATE ${LIBGIT2_SYSTEM_INCLUDES})
endif()
if(DEFINED LIBGIT2_DEPENDENCY_INCLUDES)
    target_include_directories(autogitpull_lib PRIVATE ${LIBGIT2_DEPENDENCY_INCLUDES})
endif()
target_link_libraries(autogitpull_lib
    PUBLIC ${LIBGIT2_TARGET} ${LIBGIT2_DEPENDENCIES} yaml-cpp::yaml-cpp nlohmann_json::nlohmann_json Threads::Threads)
if(TARGET ZLIB::ZLIB)
    target_link_libraries(autogitpull_lib PUBLIC ZLIB::ZLIB)
endif()
if(TARGET zlibstatic)
    target_link_libraries(autogitpull_lib PUBLIC zlibstatic)
elseif(TARGET zlib)
    target_link_libraries(autogitpull_lib PUBLIC zlib)
endif()
if(TARGET autogitpull_zlib)
    target_link_libraries(autogitpull_lib PUBLIC autogitpull_zlib)
endif()
if(APPLE AND AUTOGITPULL_APPLE_ZLIB)
    target_link_libraries(autogitpull_lib PUBLIC ${AUTOGITPULL_APPLE_ZLIB})
endif()
if(APPLE)
    target_link_libraries(autogitpull_lib PUBLIC
        ${COREFOUNDATION_FRAMEWORK}
        ${CORESERVICES_FRAMEWORK})
    if(SECURITY_FRAMEWORK)
        target_link_libraries(autogitpull_lib PUBLIC ${SECURITY_FRAMEWORK})
    else()
        # Fallback to explicit framework flag when path not found
        target_link_options(autogitpull_lib PUBLIC "-framework" "Security")
    endif()
    if(FSEVENTS_FRAMEWORK)
        target_link_libraries(autogitpull_lib PUBLIC ${FSEVENTS_FRAMEWORK})
    endif()
    # Older Apple toolchains require explicit linkage to the filesystem
    # library. Attempt to locate either libc++fs or libstdc++fs and link
    # it when available so that std::filesystem symbols resolve correctly
    # on macOS 64-bit builds.
    find_library(FILESYSTEM_LIB c++fs)
    if(NOT FILESYSTEM_LIB)
        find_library(FILESYSTEM_LIB stdc++fs)
    endif()
    if(FILESYSTEM_LIB)
        target_link_libraries(autogitpull_lib PUBLIC ${FILESYSTEM_LIB})
    endif()
endif()

add_executable(autogitpull
    src/autogitpull.cpp
    src/tui.cpp)
target_link_libraries(autogitpull PRIVATE autogitpull_lib ${LIBGIT2_TARGET})
set_property(TARGET autogitpull PROPERTY LINK_LIBRARY_DEPENDENCIES ON)
if(TARGET ZLIB::ZLIB)
    target_link_libraries(autogitpull PRIVATE ZLIB::ZLIB)
endif()
if(TARGET zlibstatic)
    target_link_libraries(autogitpull PRIVATE zlibstatic)
elseif(TARGET zlib)
    target_link_libraries(autogitpull PRIVATE zlib)
endif()
if(TARGET autogitpull_zlib)
    target_link_libraries(autogitpull PRIVATE autogitpull_zlib)
endif()
if(APPLE)
    target_link_libraries(autogitpull PRIVATE
        ${COREFOUNDATION_FRAMEWORK}
        ${CORESERVICES_FRAMEWORK})
    if(SECURITY_FRAMEWORK)
        target_link_libraries(autogitpull PRIVATE ${SECURITY_FRAMEWORK})
    else()
        target_link_options(autogitpull PRIVATE "-framework" "Security")
    endif()
    if(FSEVENTS_FRAMEWORK)
        target_link_libraries(autogitpull PRIVATE ${FSEVENTS_FRAMEWORK})
    endif()
    # macOS does not support fully static binaries
endif()

# Warnings configuration
option(AUTOGITPULL_SUPPRESS_ALL_WARNINGS "Silence compiler warnings for project targets" ON)
if(MSVC)
    if(AUTOGITPULL_SUPPRESS_ALL_WARNINGS)
        target_compile_options(autogitpull_lib PRIVATE /w)
        target_compile_options(autogitpull PRIVATE /w)
    else()
        target_compile_options(autogitpull_lib PRIVATE /W4)
        target_compile_options(autogitpull PRIVATE /W4)
    endif()
else()
    if(AUTOGITPULL_SUPPRESS_ALL_WARNINGS)
        target_compile_options(autogitpull_lib PRIVATE -w)
        target_compile_options(autogitpull PRIVATE -w)
    else()
        target_compile_options(autogitpull_lib PRIVATE -Wall -Wextra)
        target_compile_options(autogitpull PRIVATE -Wall -Wextra)
    endif()
endif()

enable_testing()
find_package(Catch2 3 QUIET)
if(NOT Catch2_FOUND)
    message(STATUS "Catch2 not found, fetching via FetchContent...")
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.2
        CMAKE_ARGS
            -DBUILD_SHARED_LIBS=OFF
            -DCATCH_BUILD_TESTING=OFF
            -DCATCH_BUILD_EXAMPLES=OFF
            -DCATCH_BUILD_SHARED_LIBS=OFF
            -DCATCH_BUILD_STATIC_LIBRARY=ON
            -DCATCH_INSTALL_DOCS=OFF
            -DCATCH_INSTALL_HELPERS=OFF
    )
    FetchContent_MakeAvailable(Catch2)
endif()
add_executable(autogitpull_tests
  tests/arg_parser_tests.cpp tests/utils_tests.cpp tests/options_tests.cpp tests/config_tests.cpp tests/repo_tests.cpp tests/process_tests.cpp tests/ui_output_tests.cpp tests/history_tests.cpp tests/ignore_utils_tests.cpp tests/timeout_tests.cpp tests/git_remote_tests.cpp tests/mutant_timeout_tests.cpp tests/windows_attach_tests.cpp tests/macos_daemon_tests.cpp tests/cli_commands_tests.cpp tests/resource_limit_tests.cpp tests/logger_tests.cpp tests/dry_run_tests.cpp src/autogitpull.cpp src/tui.cpp src/ignore_utils.cpp)
target_sources(autogitpull_tests PRIVATE tests/post_pull_hook_tests.cpp)
target_sources(autogitpull_tests PRIVATE tests/file_watch_tests.cpp)
target_include_directories(autogitpull_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(autogitpull_tests PRIVATE AUTOGITPULL_NO_MAIN)
target_link_libraries(autogitpull_tests PRIVATE Catch2::Catch2WithMain autogitpull_lib ${LIBGIT2_TARGET})
if(TARGET ZLIB::ZLIB)
    target_link_libraries(autogitpull_tests PRIVATE ZLIB::ZLIB)
endif()
if(TARGET zlibstatic)
    target_link_libraries(autogitpull_tests PRIVATE zlibstatic)
elseif(TARGET zlib)
    target_link_libraries(autogitpull_tests PRIVATE zlib)
endif()
if(TARGET autogitpull_zlib)
    target_link_libraries(autogitpull_tests PRIVATE autogitpull_zlib)
endif()
set_property(TARGET autogitpull_tests PROPERTY LINK_LIBRARY_DEPENDENCIES ON)
if(UNIX AND NOT APPLE)
    target_link_libraries(autogitpull_tests PRIVATE dl)
endif()
if(APPLE)
    target_link_libraries(autogitpull_tests PRIVATE
        ${COREFOUNDATION_FRAMEWORK}
        ${CORESERVICES_FRAMEWORK})
    if(SECURITY_FRAMEWORK)
        target_link_libraries(autogitpull_tests PRIVATE ${SECURITY_FRAMEWORK})
    else()
        target_link_options(autogitpull_tests PRIVATE "-framework" "Security")
    endif()
    if(FSEVENTS_FRAMEWORK)
        target_link_libraries(autogitpull_tests PRIVATE ${FSEVENTS_FRAMEWORK})
    endif()
endif()
set(_catch2_extra_search_paths)
if(DEFINED Catch2_SOURCE_DIR)
    list(APPEND _catch2_extra_search_paths "${Catch2_SOURCE_DIR}/extras")
endif()
if(DEFINED Catch2_DIR)
    list(APPEND _catch2_extra_search_paths
        "${Catch2_DIR}/../extras"
        "${Catch2_DIR}/../../share/Catch2"
        "${Catch2_DIR}/../../Catch2")
endif()
find_path(CATCH2_EXTRA_DIR Catch.cmake HINTS ${_catch2_extra_search_paths} NO_DEFAULT_PATH)
if(CATCH2_EXTRA_DIR)
    list(APPEND CMAKE_MODULE_PATH "${CATCH2_EXTRA_DIR}")
    include(Catch)
    catch_discover_tests(autogitpull_tests
        TEST_PREFIX autogitpull_tests::
        EXTRA_ARGS --durations=yes --skip-benchmarks)
else()
    add_test(NAME autogitpull_tests COMMAND autogitpull_tests)
endif()
unset(CATCH2_EXTRA_DIR)
unset(_catch2_extra_search_paths)
if(WIN32)
    add_custom_command(TARGET autogitpull_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Catch2::Catch2>
            $<TARGET_FILE_DIR:autogitpull_tests>)
    add_custom_command(TARGET autogitpull_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Catch2::Catch2WithMain>
            $<TARGET_FILE_DIR:autogitpull_tests>)
endif()

# Address sanitizer memory leak test
if(NOT MSVC)
    add_executable(memory_leak_test
        tests/memory_leak.cpp
        src/autogitpull.cpp
        src/scanner.cpp
        src/file_watch.cpp
        src/ui_loop.cpp
        src/git_utils.cpp
        src/logger.cpp
        src/resource_utils.cpp
        src/system_utils.cpp
        src/time_utils.cpp
        src/config_utils.cpp
        src/debug_utils.cpp
        src/help_text.cpp
        src/ignore_utils.cpp
        src/parse_utils.cpp
        src/history_utils.cpp
        src/tui.cpp)
    if(WIN32)
        target_sources(memory_leak_test PRIVATE src/windows_service.cpp src/windows_commands.cpp src/lock_utils_windows.cpp src/linux_daemon.cpp)
    elseif(APPLE)
        target_sources(memory_leak_test PRIVATE src/macos_daemon.cpp src/linux_commands.cpp src/lock_utils_posix.cpp)
    else()
        target_sources(memory_leak_test PRIVATE src/linux_daemon.cpp src/linux_commands.cpp src/lock_utils_posix.cpp)
    endif()
    target_include_directories(memory_leak_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_compile_definitions(memory_leak_test PRIVATE AUTOGITPULL_NO_MAIN)
    target_compile_options(memory_leak_test PRIVATE -fsanitize=address)
    target_link_options(memory_leak_test PRIVATE -fsanitize=address)
    target_link_libraries(memory_leak_test PRIVATE Catch2::Catch2WithMain autogitpull_lib ${LIBGIT2_TARGET} yaml-cpp::yaml-cpp nlohmann_json::nlohmann_json)
    if(TARGET ZLIB::ZLIB)
        target_link_libraries(memory_leak_test PRIVATE ZLIB::ZLIB)
    endif()
    if(TARGET zlibstatic)
        target_link_libraries(memory_leak_test PRIVATE zlibstatic)
    elseif(TARGET zlib)
        target_link_libraries(memory_leak_test PRIVATE zlib)
    endif()
    if(TARGET autogitpull_zlib)
        target_link_libraries(memory_leak_test PRIVATE autogitpull_zlib)
    endif()
    set_property(TARGET memory_leak_test PROPERTY LINK_LIBRARY_DEPENDENCIES ON)
    add_test(NAME memory_leak_test COMMAND memory_leak_test)
    if(APPLE)
        target_link_libraries(memory_leak_test PRIVATE
            ${COREFOUNDATION_FRAMEWORK}
            ${CORESERVICES_FRAMEWORK})
        if(SECURITY_FRAMEWORK)
            target_link_libraries(memory_leak_test PRIVATE ${SECURITY_FRAMEWORK})
        else()
            target_link_options(memory_leak_test PRIVATE "-framework" "Security")
        endif()
        if(FSEVENTS_FRAMEWORK)
            target_link_libraries(memory_leak_test PRIVATE ${FSEVENTS_FRAMEWORK})
        endif()
    endif()
endif()
