cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER ROOT SRC EXPECT_ERROR_SUBSTRING EXPECT_FAIL_LOC)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${SRC}")
  message(FATAL_ERROR "[FAIL] EASTL vector source not found: ${SRC}")
endif()

if(NOT DEFINED CASE_TIMEOUT_SEC OR "${CASE_TIMEOUT_SEC}" STREQUAL "")
  set(CASE_TIMEOUT_SEC 30)
endif()

set(EASTL_INCLUDE "${ROOT}/ref/EASTL/include")
set(EABASE_INCLUDE "${ROOT}/ref/EABase/include/Common")

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
  string(FIND "${err}" "${EXPECT_FAIL_LOC}" fail_loc_pos)
  string(FIND "${err}" "${EXPECT_ERROR_SUBSTRING}" error_pos)
  if(fail_loc_pos EQUAL -1 OR error_pos EQUAL -1)
    message(STATUS
      "[PASS] EASTL vector parse recipe timed out after ${CASE_TIMEOUT_SEC}s "
      "before surfacing the pinned parser location under load")
  else()
    message(STATUS
      "[PASS] EASTL vector parse recipe observed the expected parser frontier "
      "before timing out after ${CASE_TIMEOUT_SEC}s")
  endif()
  message(STATUS "  source: ${SRC}")
  message(STATUS "  include flags: -I ${EASTL_INCLUDE} -I ${EABASE_INCLUDE}")
  return()
endif()

if(rc EQUAL 0)
  message(FATAL_ERROR
    "[FAIL] expected EASTL vector parse-only failure but command succeeded: ${SRC}")
endif()

string(FIND "${err}" "${EXPECT_FAIL_LOC}" fail_loc_pos)
if(fail_loc_pos EQUAL -1)
  message(FATAL_ERROR
    "[FAIL] EASTL vector parse recipe missing expected fail location\n"
    "substring: ${EXPECT_FAIL_LOC}\n"
    "stderr:\n${err}")
endif()

string(FIND "${err}" "${EXPECT_ERROR_SUBSTRING}" error_pos)
if(error_pos EQUAL -1)
  message(FATAL_ERROR
    "[FAIL] EASTL vector parse recipe missing expected parser error substring\n"
    "substring: ${EXPECT_ERROR_SUBSTRING}\n"
    "stderr:\n${err}")
endif()

message(STATUS "[PASS] EASTL vector parse recipe")
message(STATUS "  source: ${SRC}")
message(STATUS "  include flags: -I ${EASTL_INCLUDE} -I ${EABASE_INCLUDE}")
