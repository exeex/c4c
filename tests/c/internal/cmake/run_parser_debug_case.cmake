cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER SRC EXPECT_ERROR_SUBSTRING EXPECT_STACK_SUBSTRING)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${SRC}")
  message(FATAL_ERROR "[FAIL] parser debug case not found: ${SRC}")
endif()

set(parser_debug_args --parser-debug)
if(DEFINED PARSER_DEBUG_ARGS AND NOT "${PARSER_DEBUG_ARGS}" STREQUAL "")
  separate_arguments(parser_debug_args NATIVE_COMMAND "${PARSER_DEBUG_ARGS}")
endif()

execute_process(
  COMMAND "${COMPILER}" ${parser_debug_args} --parse-only "${SRC}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)

if(rc EQUAL 0)
  message(FATAL_ERROR "[FAIL] expected parser failure but command succeeded: ${SRC}")
endif()

string(FIND "${err}" "${EXPECT_ERROR_SUBSTRING}" error_pos)
if(error_pos EQUAL -1)
  message(FATAL_ERROR
    "[FAIL] parser debug output missing expected error substring\n"
    "substring: ${EXPECT_ERROR_SUBSTRING}\n"
    "stderr:\n${err}")
endif()

string(FIND "${err}" "${EXPECT_STACK_SUBSTRING}" stack_pos)
if(stack_pos EQUAL -1)
  message(FATAL_ERROR
    "[FAIL] parser debug output missing expected stack substring\n"
    "substring: ${EXPECT_STACK_SUBSTRING}\n"
    "stderr:\n${err}")
endif()

if(DEFINED EXPECT_CONTEXT_SUBSTRING AND
   NOT "${EXPECT_CONTEXT_SUBSTRING}" STREQUAL "")
  string(FIND "${err}" "${EXPECT_CONTEXT_SUBSTRING}" context_pos)
  if(context_pos EQUAL -1)
    message(FATAL_ERROR
      "[FAIL] parser debug output missing expected context substring\n"
      "substring: ${EXPECT_CONTEXT_SUBSTRING}\n"
      "stderr:\n${err}")
  endif()
endif()

if(DEFINED EXPECT_ABSENT_SUBSTRING AND
   NOT "${EXPECT_ABSENT_SUBSTRING}" STREQUAL "")
  string(FIND "${err}" "${EXPECT_ABSENT_SUBSTRING}" absent_pos)
  if(NOT absent_pos EQUAL -1)
    message(FATAL_ERROR
      "[FAIL] parser debug output unexpectedly contained forbidden substring\n"
      "substring: ${EXPECT_ABSENT_SUBSTRING}\n"
      "stderr:\n${err}")
  endif()
endif()

message(STATUS "[PASS][parser-debug] ${SRC}")
