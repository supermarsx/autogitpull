cmake_minimum_required(VERSION 3.15)
project(autogitpull)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBGIT2 REQUIRED libgit2)

add_executable(autogitpull autogitpull.cpp git_utils.cpp tui.cpp)
target_include_directories(autogitpull PRIVATE ${LIBGIT2_INCLUDE_DIRS})
target_link_libraries(autogitpull ${LIBGIT2_LIBRARIES} pthread)
