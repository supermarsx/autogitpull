cmake_minimum_required(VERSION 3.15)
project(autogitpull)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
if(APPLE)
    set(PKG_CONFIG_USE_STATIC_LIBS OFF)
else()
    set(PKG_CONFIG_USE_STATIC_LIBS ON)
endif()
pkg_check_modules(LIBGIT2 QUIET IMPORTED_TARGET libgit2)
if(NOT LIBGIT2_FOUND)
    message(FATAL_ERROR "libgit2 not found. Install the libgit2 development package or run ./install_deps.sh")
endif()

add_library(autogitpull_lib STATIC git_utils.cpp logger.cpp resource_utils.cpp time_utils.cpp)
target_link_libraries(autogitpull_lib PRIVATE PkgConfig::LIBGIT2 pthread)

add_executable(autogitpull autogitpull.cpp tui.cpp)
target_link_libraries(autogitpull PRIVATE autogitpull_lib)
if(MSVC)
    target_link_options(autogitpull PRIVATE /MT)
elseif(APPLE)
    # macOS does not support fully static binaries
else()
    # Link dynamically on platforms where static libgit2 is unavailable
endif()
if(MINGW)
    target_link_libraries(autogitpull PRIVATE winhttp ole32 rpcrt4 crypt32)
endif()

include(FetchContent)

enable_testing()
find_package(Catch2 3 QUIET)
if(NOT Catch2_FOUND)
    message(STATUS "Catch2 not found, fetching via FetchContent...")
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.2
    )
    FetchContent_MakeAvailable(Catch2)
endif()
add_executable(autogitpull_tests tests/tests.cpp)
target_include_directories(autogitpull_tests PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(autogitpull_tests PRIVATE Catch2::Catch2WithMain autogitpull_lib)
add_test(NAME autogitpull_tests COMMAND autogitpull_tests)
