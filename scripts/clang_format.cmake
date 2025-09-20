cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED PROJECT_ROOT)
  set(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
endif()
if(NOT DEFINED ACTION)
  set(ACTION "check")
endif()

set(SOURCE_GLOBS
  "${PROJECT_ROOT}/src/*.c"
  "${PROJECT_ROOT}/src/*.cc"
  "${PROJECT_ROOT}/src/*.cpp"
  "${PROJECT_ROOT}/include/*.h"
  "${PROJECT_ROOT}/include/*.hpp")

set(FILES)
foreach(pattern IN LISTS SOURCE_GLOBS)
  file(GLOB files "${pattern}")
  list(APPEND FILES ${files})
endforeach()
list(REMOVE_DUPLICATES FILES)
if(NOT FILES)
  message("No source files matched clang-format patterns; skipping")
  return()
endif()

find_program(CLANG_FORMAT_EXECUTABLE NAMES clang-format
  HINTS
    "C:/Program Files/LLVM/bin"
    "C:/Program Files (x86)/LLVM/bin"
)
if(NOT CLANG_FORMAT_EXECUTABLE)
  message("clang-format not found; skipping format (install clang-format to enable)")
  return()
endif()

set(result 0)
if(ACTION STREQUAL "fix")
  message("Running clang-format")
  execute_process(
    COMMAND "${CLANG_FORMAT_EXECUTABLE}" -i ${FILES}
    RESULT_VARIABLE result)
elseif(ACTION STREQUAL "check")
  message("Checking format")
  execute_process(
    COMMAND "${CLANG_FORMAT_EXECUTABLE}" --dry-run --Werror ${FILES}
    RESULT_VARIABLE result)
else()
  message(FATAL_ERROR "Unknown ACTION '${ACTION}'")
endif()

if(NOT result EQUAL 0)
  message(FATAL_ERROR "clang-format returned ${result}")
endif()
