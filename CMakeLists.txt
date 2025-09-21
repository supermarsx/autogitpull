cmake_minimum_required(VERSION 3.20)
project(autogitpull LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(BUILD_SHARED_LIBS OFF)
if(MSVC)
    # Link the static runtime to satisfy "static everything" on Windows
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# --- BEGIN: flag normalization (treat env flags sanely) ---
# Treat env flags as proper CMake lists (avoid "all flags as one token")
if(DEFINED ENV{CFLAGS})
  separate_arguments(_ENV_CFLAGS NATIVE_COMMAND "$ENV{CFLAGS}")
  add_compile_options(${_ENV_CFLAGS})
endif()
if(DEFINED ENV{CXXFLAGS})
  separate_arguments(_ENV_CXXFLAGS NATIVE_COMMAND "$ENV{CXXFLAGS}")
  add_compile_options(${_ENV_CXXFLAGS})
endif()
if(DEFINED ENV{LDFLAGS})
  separate_arguments(_ENV_LDFLAGS NATIVE_COMMAND "$ENV{LDFLAGS}")
  add_link_options(${_ENV_LDFLAGS})
endif()

# GNU extensions help libgit2 detect struct stat nsec fields correctly
add_compile_definitions(_GNU_SOURCE)

# Emergency switch to disable nanosecond timestamps if needed
option(CODEX_FORCE_NO_NSEC "Force-disable nanosecond stat fields for libgit2" OFF)
if(CODEX_FORCE_NO_NSEC)
  add_compile_definitions(GIT_USE_NSEC=0)
endif()
# --- END: flag normalization ---

include(FetchContent)

set(ZLIB_USE_STATIC_LIBS TRUE)
option(AUTOGITPULL_USE_SYSTEM_ZLIB "Use a pre-installed zlib instead of the bundled static build" OFF)
if(AUTOGITPULL_USE_SYSTEM_LIBGIT2 AND NOT AUTOGITPULL_USE_SYSTEM_ZLIB)
    set(AUTOGITPULL_USE_SYSTEM_ZLIB ON CACHE BOOL "" FORCE)
endif()

set(_autogitpull_zlib_target "")
if(AUTOGITPULL_USE_SYSTEM_ZLIB)
    find_package(ZLIB REQUIRED)
    if(NOT TARGET ZLIB::ZLIB)
        message(FATAL_ERROR "System zlib must export the ZLIB::ZLIB target")
    endif()
else()
    message(STATUS "Configuring bundled static zlib...")
    FetchContent_Declare(
        autogitpull_zlib
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG v1.3.1
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(autogitpull_zlib)
    if(TARGET zlibstatic)
        set(_autogitpull_zlib_target zlibstatic)
        if(NOT TARGET ZLIB::ZLIB)
            add_library(ZLIB::ZLIB ALIAS zlibstatic)
        endif()
    elseif(TARGET zlib)
        set(_autogitpull_zlib_target zlib)
        if(NOT TARGET ZLIB::ZLIB)
            add_library(ZLIB::ZLIB ALIAS zlib)
        endif()
    endif()
    if(NOT TARGET ZLIB::ZLIB)
        message(FATAL_ERROR "Bundled zlib did not produce ZLIB::ZLIB target")
    endif()
    if(_autogitpull_zlib_target)
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
                set(ZLIB_LIBRARIES "${ZLIB_LIBRARY_RELEASE}" CACHE STRING "" FORCE)
            else()
                set(ZLIB_LIBRARY_DEBUG "${_autogitpull_zlib_debug_lib}" CACHE FILEPATH "" FORCE)
                set(ZLIB_LIBRARIES "optimized;${ZLIB_LIBRARY_RELEASE};debug;${ZLIB_LIBRARY_DEBUG}" CACHE STRING "" FORCE)
            endif()
            set(ZLIB_LIBRARY "${ZLIB_LIBRARY_RELEASE}" CACHE FILEPATH "" FORCE)
        else()
            set(ZLIB_LIBRARY "${_autogitpull_zlib_release_lib}" CACHE FILEPATH "" FORCE)
            set(ZLIB_LIBRARIES "${ZLIB_LIBRARY}" CACHE STRING "" FORCE)
        endif()
        set(ZLIB_FOUND TRUE CACHE BOOL "" FORCE)
    endif()
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
        -DBUILD_SHARED_LIBS=OFF
        -DBUILD_TESTS=OFF
        -DBUILD_TESTING=OFF
        -DBUILD_CLI=OFF
        -DBUILD_EXAMPLES=OFF
        -DBUILD_FUZZERS=OFF
        -DUSE_SSH=OFF
        -DUSE_REGEX=builtin
        -DUSE_COMPRESSION=zlib
        -DUSE_BUNDLED_ZLIB=OFF
        -DLINK_WITH_STATIC_LIBRARIES=ON
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DCMAKE_POLICY_DEFAULT_CMP0077=NEW
    )

    if(WIN32)
        # Work around MSVC/Windows struct stat nsec detection issues by disabling USE_NSEC.
        # This avoids: "GIT_USE_NSEC defined but unknown struct stat nanosecond type" during libgit2 build.
        list(APPEND _autogitpull_libgit2_args -DUSE_HTTPS=WinHTTP -DSTATIC_CRT=ON -DUSE_NSEC=OFF)
        list(APPEND LIBGIT2_DEPENDENCIES winhttp rpcrt4 crypt32 ole32 ws2_32 secur32 bcrypt)
    elseif(APPLE)
        list(APPEND _autogitpull_libgit2_args -DUSE_HTTPS=SecureTransport)
        list(APPEND LIBGIT2_DEPENDENCIES Security)
    else()
        set(OPENSSL_USE_STATIC_LIBS TRUE)
        list(APPEND _autogitpull_libgit2_args -DUSE_HTTPS=OpenSSL -DOPENSSL_USE_STATIC_LIBS=ON)
        find_package(OpenSSL REQUIRED)
        list(APPEND LIBGIT2_DEPENDENCIES OpenSSL::SSL OpenSSL::Crypto)
        # Work around Linux (Ubuntu) detection issue for struct stat nanoseconds in libgit2.
        # Force-disable USE_NSEC to avoid: "GIT_USE_NSEC defined but unknown struct stat nanosecond type".
        list(APPEND _autogitpull_libgit2_args -DUSE_NSEC=OFF)
    endif()

    # Prevent libgit2 from registering its own CTest tests in our build
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        autogitpull_libgit2
        GIT_REPOSITORY https://github.com/libgit2/libgit2.git
        GIT_TAG v1.9.1
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        CMAKE_ARGS ${_autogitpull_libgit2_args}
    )
    FetchContent_MakeAvailable(autogitpull_libgit2)
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
    if(NOT TARGET libgit2::libgit2)
        add_library(libgit2::libgit2 ALIAS ${_autogitpull_libgit2_build_target})
    endif()

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
        message(FATAL_ERROR "libgit2 did not expose a zlib target to reuse")
    endif()
    unset(_autogitpull_libgit2_build_target)
endif()
unset(OPENSSL_USE_STATIC_LIBS)
unset(_libgit2_is_static)
unset(_libgit2_type)
unset(_libgit2_location)

find_package(Threads REQUIRED)

if(APPLE)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
    find_library(CORESERVICES_FRAMEWORK CoreServices REQUIRED)
    find_library(FSEVENTS_FRAMEWORK FSEvents REQUIRED)
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
target_link_libraries(autogitpull_lib
    PUBLIC ${LIBGIT2_TARGET} ${LIBGIT2_DEPENDENCIES} yaml-cpp::yaml-cpp nlohmann_json::nlohmann_json ZLIB::ZLIB Threads::Threads)
if(APPLE)
    target_link_libraries(autogitpull_lib PUBLIC
        ${COREFOUNDATION_FRAMEWORK}
        ${CORESERVICES_FRAMEWORK}
        ${FSEVENTS_FRAMEWORK})
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
if(APPLE)
    target_link_libraries(autogitpull PRIVATE
        ${COREFOUNDATION_FRAMEWORK}
        ${CORESERVICES_FRAMEWORK}
        ${FSEVENTS_FRAMEWORK})
    # macOS does not support fully static binaries
endif()

# Required compiler warnings
if(MSVC)
    target_compile_options(autogitpull_lib PRIVATE /W4 /WX)
    target_compile_options(autogitpull PRIVATE /W4 /WX)
else()
    target_compile_options(autogitpull_lib PRIVATE -Wall -Wextra -Werror)
    target_compile_options(autogitpull PRIVATE -Wall -Wextra -Werror)
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
target_link_libraries(autogitpull_tests PRIVATE Catch2::Catch2WithMain autogitpull_lib ${LIBGIT2_TARGET} ZLIB::ZLIB)
set_property(TARGET autogitpull_tests PROPERTY LINK_LIBRARY_DEPENDENCIES ON)
if(UNIX AND NOT APPLE)
    target_link_libraries(autogitpull_tests PRIVATE dl)
endif()
if(APPLE)
    target_link_libraries(autogitpull_tests PRIVATE
        ${COREFOUNDATION_FRAMEWORK}
        ${CORESERVICES_FRAMEWORK}
        ${FSEVENTS_FRAMEWORK})
endif()
add_test(NAME autogitpull_tests COMMAND autogitpull_tests)
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
    target_link_libraries(memory_leak_test PRIVATE Catch2::Catch2WithMain autogitpull_lib ${LIBGIT2_TARGET} yaml-cpp::yaml-cpp nlohmann_json::nlohmann_json ZLIB::ZLIB)
    set_property(TARGET memory_leak_test PROPERTY LINK_LIBRARY_DEPENDENCIES ON)
    add_test(NAME memory_leak_test COMMAND memory_leak_test)
    if(APPLE)
        target_link_libraries(memory_leak_test PRIVATE
            ${COREFOUNDATION_FRAMEWORK}
            ${CORESERVICES_FRAMEWORK}
            ${FSEVENTS_FRAMEWORK})
    endif()
endif()
