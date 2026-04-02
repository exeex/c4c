cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER ROOT SRC)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${SRC}")
  message(FATAL_ERROR "[FAIL] EASTL source not found: ${SRC}")
endif()

if(NOT DEFINED CASE_TIMEOUT_SEC OR "${CASE_TIMEOUT_SEC}" STREQUAL "")
  set(CASE_TIMEOUT_SEC 30)
endif()

if(NOT DEFINED EXPECT_SUCCESS OR "${EXPECT_SUCCESS}" STREQUAL "")
  set(EXPECT_SUCCESS OFF)
endif()

set(EASTL_INCLUDE "${ROOT}/ref/EASTL/include")
set(EABASE_INCLUDE "${ROOT}/ref/EABase/include/Common")
set(COMMAND_LINE
    "${COMPILER} --parse-only -I ${EASTL_INCLUDE} -I ${EABASE_INCLUDE} ${SRC}")

execute_process(
  COMMAND "${COMPILER}" --parse-only
          -I "${EASTL_INCLUDE}"
          -I "${EABASE_INCLUDE}"
          "${SRC}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)

if(rc MATCHES "timeout")
  message(FATAL_ERROR
    "[FAIL] EASTL parse recipe timed out after ${CASE_TIMEOUT_SEC}s\n"
    "command: ${COMMAND_LINE}\n"
    "stderr:\n${err}")
endif()

if(EXPECT_SUCCESS)
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR
      "[FAIL] expected EASTL parse-only success\n"
      "command: ${COMMAND_LINE}\n"
      "stderr:\n${err}")
  endif()

  message(STATUS "[PASS] EASTL parse recipe")
  message(STATUS "  source: ${SRC}")
  message(STATUS "  result: parse success")
  message(STATUS "  command: ${COMMAND_LINE}")
  return()
endif()

foreach(v EXPECT_FAIL_LOC EXPECT_ERROR_SUBSTRING)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=... for failure expectation")
  endif()
endforeach()

if(rc EQUAL 0)
  message(FATAL_ERROR
    "[FAIL] expected EASTL parse-only failure but command succeeded\n"
    "command: ${COMMAND_LINE}")
endif()

string(FIND "${err}" "${EXPECT_FAIL_LOC}" fail_loc_pos)
if(fail_loc_pos EQUAL -1)
  message(FATAL_ERROR
    "[FAIL] EASTL parse recipe missing expected fail location\n"
    "command: ${COMMAND_LINE}\n"
    "substring: ${EXPECT_FAIL_LOC}\n"
    "stderr:\n${err}")
endif()

string(FIND "${err}" "${EXPECT_ERROR_SUBSTRING}" error_pos)
if(error_pos EQUAL -1)
  message(FATAL_ERROR
    "[FAIL] EASTL parse recipe missing expected parser error substring\n"
    "command: ${COMMAND_LINE}\n"
    "substring: ${EXPECT_ERROR_SUBSTRING}\n"
    "stderr:\n${err}")
endif()

message(STATUS "[PASS] EASTL parse recipe")
message(STATUS "  source: ${SRC}")
message(STATUS "  result: expected parse failure")
message(STATUS "  command: ${COMMAND_LINE}")
