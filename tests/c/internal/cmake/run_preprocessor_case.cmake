cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER SRC)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${SRC}")
  message(FATAL_ERROR "[FAIL] preprocessor case not found: ${SRC}")
endif()

if(NOT DEFINED CASE_TIMEOUT_SEC OR "${CASE_TIMEOUT_SEC}" STREQUAL "")
  set(CASE_TIMEOUT_SEC 30)
endif()

execute_process(
  COMMAND "${COMPILER}" --parse-only "${SRC}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE parse_rc
  OUTPUT_VARIABLE parse_out
  ERROR_VARIABLE parse_err
)
if(parse_rc MATCHES "timeout")
  message(FATAL_ERROR "[PREPROCESSOR_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT parse_rc EQUAL 0)
  message(FATAL_ERROR "[PREPROCESSOR_FAIL] ${SRC}\n${parse_err}")
endif()

message(STATUS "[PASS][preprocessor] ${SRC}")
