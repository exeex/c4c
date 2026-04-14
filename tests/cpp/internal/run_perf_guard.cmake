cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER BINARY_DIR REPEAT_COUNT EXPECT_TIMEOUT_SEC TEST_NAME GENERATOR)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

file(MAKE_DIRECTORY "${BINARY_DIR}/tmp")
set(src "${BINARY_DIR}/tmp/${TEST_NAME}_perf.cpp")

execute_process(
  COMMAND "${CMAKE_COMMAND}" -E env python3
          "${GENERATOR}"
          --test "${TEST_NAME}"
          --output "${src}"
          --repeat-count "${REPEAT_COUNT}"
  RESULT_VARIABLE gen_rc
  OUTPUT_VARIABLE gen_out
  ERROR_VARIABLE  gen_err
)

if(NOT gen_rc EQUAL 0)
  message(FATAL_ERROR
    "[FAIL] perf_gen.py failed for '${TEST_NAME}'\n"
    "rc=${gen_rc}\n"
    "stdout:\n${gen_out}\n"
    "stderr:\n${gen_err}")
endif()

execute_process(
  COMMAND "${COMPILER}" --parse-only "${src}"
  TIMEOUT "${EXPECT_TIMEOUT_SEC}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE  err
)

if(NOT rc EQUAL 0)
  message(FATAL_ERROR
    "[FAIL] ${TEST_NAME} perf guard failed\n"
    "rc=${rc}\n"
    "stdout:\n${out}\n"
    "stderr:\n${err}")
endif()

if(NOT "${err}" STREQUAL "")
  message(FATAL_ERROR
    "[FAIL] expected no stderr from ${TEST_NAME} perf guard\n"
    "stderr:\n${err}")
endif()

message(STATUS "[PASS] ${TEST_NAME} perf guard")
message(STATUS "  source: ${src}")
message(STATUS "  repeats: ${REPEAT_COUNT}")
message(STATUS "  timeout budget: ${EXPECT_TIMEOUT_SEC}s")
