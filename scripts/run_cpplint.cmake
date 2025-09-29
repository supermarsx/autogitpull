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

set(CPPLINT_REPORT "${PROJECT_ROOT}/cpplint.txt")
unset(result)
unset(out)
unset(err)
find_program(CPPLINT_EXECUTABLE NAMES cpplint)
if(CPPLINT_EXECUTABLE)
  execute_process(
    COMMAND "${CPPLINT_EXECUTABLE}" --linelength=120 ${FILES}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err)
else()
  find_package(Python3 COMPONENTS Interpreter)
  if(NOT Python3_Interpreter_FOUND)
    message("cpplint not found and no Python3 interpreter; skipping cpplint")
    return()
  endif()
  execute_process(
    COMMAND "${Python3_EXECUTABLE}" -m cpplint --linelength=120 ${FILES}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err)
endif()

# Write combined output to report
file(WRITE "${CPPLINT_REPORT}" "${out}${err}")

if(NOT result EQUAL 0)
  if(DEFINED ENV{CPPLINT_NON_BLOCKING} AND "$ENV{CPPLINT_NON_BLOCKING}" STREQUAL "ON")
    message(WARNING "cpplint reported issues; see ${CPPLINT_REPORT}")
  else()
    message(FATAL_ERROR "cpplint failed; see ${CPPLINT_REPORT}")
  endif()
endif()
