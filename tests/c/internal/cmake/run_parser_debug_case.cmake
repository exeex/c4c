cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER SRC EXPECT_ERROR_SUBSTRING EXPECT_STACK_SUBSTRING)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${SRC}")
  message(FATAL_ERROR "[FAIL] parser debug case not found: ${SRC}")
endif()

execute_process(
  COMMAND "${COMPILER}" --parser-debug --parse-only "${SRC}"
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

message(STATUS "[PASS][parser-debug] ${SRC}")
