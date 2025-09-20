cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED PROJECT_ROOT)
  set(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
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
  message("No source files matched cpplint patterns; skipping")
  return()
endif()

find_program(CPPLINT_EXECUTABLE NAMES cpplint)
if(NOT CPPLINT_EXECUTABLE)
  message("cpplint not found; skipping cpplint (pip install cpplint)")
  return()
endif()

execute_process(
  COMMAND "${CPPLINT_EXECUTABLE}" --linelength=120 ${FILES}
  RESULT_VARIABLE result)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "cpplint failed")
endif()
