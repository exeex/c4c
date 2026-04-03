cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER SRC EXPECT_TIMEOUT_SEC)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${SRC}")
  message(FATAL_ERROR "[FAIL] std::vector source not found: ${SRC}")
endif()

set(compiler_args --parse-only)
if(DEFINED PARSER_DEBUG_ARGS AND NOT "${PARSER_DEBUG_ARGS}" STREQUAL "")
  separate_arguments(parser_debug_args NATIVE_COMMAND "${PARSER_DEBUG_ARGS}")
  list(APPEND compiler_args ${parser_debug_args})
endif()

execute_process(
  COMMAND "${COMPILER}" ${compiler_args} "${SRC}"
  TIMEOUT "${EXPECT_TIMEOUT_SEC}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)

if(NOT rc MATCHES "timeout")
  message(FATAL_ERROR
    "[FAIL] expected bounded std::vector parse-only timeout after ${EXPECT_TIMEOUT_SEC}s\n"
    "rc=${rc}\n"
    "stdout:\n${out}\n"
    "stderr:\n${err}")
endif()

if(NOT "${out}" STREQUAL "")
  message(FATAL_ERROR
    "[FAIL] expected no stdout from timed out std::vector parse baseline\n"
    "stdout:\n${out}")
endif()

if(DEFINED EXPECT_STDERR_SUBSTRING AND
   NOT "${EXPECT_STDERR_SUBSTRING}" STREQUAL "")
  string(FIND "${err}" "${EXPECT_STDERR_SUBSTRING}" stderr_pos)
  if(stderr_pos EQUAL -1)
    message(FATAL_ERROR
      "[FAIL] std::vector parse baseline missing expected stderr substring\n"
      "substring: ${EXPECT_STDERR_SUBSTRING}\n"
      "stderr:\n${err}")
  endif()
elseif(NOT "${err}" STREQUAL "")
  message(FATAL_ERROR
    "[FAIL] expected no stderr from timed out std::vector parse baseline\n"
    "stderr:\n${err}")
endif()

message(STATUS "[PASS] std::vector parse baseline")
message(STATUS "  source: ${SRC}")
message(STATUS "  expected timeout: ${EXPECT_TIMEOUT_SEC}s")
