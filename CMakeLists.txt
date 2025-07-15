cmake_minimum_required(VERSION 3.15)
project(autogitpull)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
set(PKG_CONFIG_USE_STATIC_LIBS ON)
pkg_check_modules(LIBGIT2 REQUIRED IMPORTED_TARGET libgit2)

add_executable(autogitpull autogitpull.cpp git_utils.cpp tui.cpp)
target_link_libraries(autogitpull PRIVATE PkgConfig::LIBGIT2 pthread)
if(MSVC)
    target_link_options(autogitpull PRIVATE /MT)
else()
    target_link_options(autogitpull PRIVATE -static)
endif()
if(MINGW)
    target_link_libraries(autogitpull PRIVATE winhttp ole32 rpcrt4 crypt32)
endif()
