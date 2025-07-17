cmake_minimum_required(VERSION 3.15)
project(autogitpull)

set(CMAKE_CXX_STANDARD 20)

find_package(PkgConfig REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(nlohmann_json 3 REQUIRED)
if(APPLE)
    set(PKG_CONFIG_USE_STATIC_LIBS OFF)
else()
    set(PKG_CONFIG_USE_STATIC_LIBS ON)
endif()
pkg_check_modules(LIBGIT2 QUIET IMPORTED_TARGET libgit2)
if(NOT LIBGIT2_FOUND)
    message(FATAL_ERROR "libgit2 not found. Install the libgit2 development package or run ./scripts/install_deps.sh")
endif()

add_library(autogitpull_lib STATIC git_utils.cpp logger.cpp resource_utils.cpp system_utils.cpp time_utils.cpp config_utils.cpp debug_utils.cpp)
target_link_libraries(autogitpull_lib PRIVATE PkgConfig::LIBGIT2 yaml-cpp nlohmann_json::nlohmann_json pthread)

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
add_executable(autogitpull_tests tests/tests.cpp autogitpull.cpp)
target_include_directories(autogitpull_tests PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_definitions(autogitpull_tests PRIVATE AUTOGITPULL_NO_MAIN)
target_link_libraries(autogitpull_tests PRIVATE Catch2::Catch2WithMain autogitpull_lib)
add_test(NAME autogitpull_tests COMMAND autogitpull_tests)

# Address sanitizer memory leak test
add_executable(memory_leak_test
    tests/memory_leak.cpp
    autogitpull.cpp
    git_utils.cpp
    logger.cpp
    resource_utils.cpp
    system_utils.cpp
    time_utils.cpp
    debug_utils.cpp)
target_include_directories(memory_leak_test PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_definitions(memory_leak_test PRIVATE AUTOGITPULL_NO_MAIN)
target_compile_options(memory_leak_test PRIVATE -fsanitize=address)
target_link_options(memory_leak_test PRIVATE -fsanitize=address)
target_link_libraries(memory_leak_test PRIVATE Catch2::Catch2WithMain PkgConfig::LIBGIT2 pthread)
add_test(NAME memory_leak_test COMMAND memory_leak_test)
