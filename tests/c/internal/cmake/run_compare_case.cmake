cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER SRC)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${SRC}")
  message(FATAL_ERROR "[FAIL] compare case not found: ${SRC}")
endif()

if(NOT DEFINED CASE_TIMEOUT_SEC OR "${CASE_TIMEOUT_SEC}" STREQUAL "")
  set(CASE_TIMEOUT_SEC 30)
endif()

execute_process(
  COMMAND "${COMPILER}" --codegen compare "${SRC}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(rc MATCHES "timeout")
  message(FATAL_ERROR "[COMPARE_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "[COMPARE_FAIL] ${SRC}\n${out}${err}")
endif()

if(NOT err MATCHES "outputs match")
  message(FATAL_ERROR "[COMPARE_MISMATCH] ${SRC}\n${err}")
endif()

message(STATUS "[PASS][compare] ${SRC}")
