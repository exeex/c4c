cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER SRC)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${SRC}")
  message(FATAL_ERROR "[FAIL] positive case not found: ${SRC}")
endif()

execute_process(
  COMMAND "${COMPILER}" "${SRC}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)

if(NOT rc EQUAL 0)
  message(FATAL_ERROR "[FAIL] expected compile success but failed: ${SRC}\n${err}")
endif()

message(STATUS "[PASS] positive compile succeeded: ${SRC}")
