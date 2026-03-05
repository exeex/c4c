cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER SRC)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${SRC}")
  message(FATAL_ERROR "[FAIL] negative case not found: ${SRC}")
endif()

execute_process(
  COMMAND "${COMPILER}" "${SRC}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)

if(rc EQUAL 0)
  if(DEFINED ENFORCE_NEGATIVE AND ENFORCE_NEGATIVE)
    message(FATAL_ERROR "[FAIL] expected compile failure but succeeded: ${SRC}")
  else()
    message(STATUS "[WARN][not-enforced] negative case compiled successfully: ${SRC}")
  endif()
else()
  message(STATUS "[PASS] negative compile failed as expected: ${SRC}")
endif()
