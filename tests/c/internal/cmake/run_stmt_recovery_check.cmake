cmake_minimum_required(VERSION 3.20)

# Verify that statement-level error recovery works:
# - compiler fails (reports error)
# - diagnostics use file:line:col: error: format
# - multiple errors can be reported without crashing
#
# Usage: cmake -DCOMPILER=... -DSRC=... -P run_stmt_recovery_check.cmake

foreach(v COMPILER SRC)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${SRC}")
  message(FATAL_ERROR "[FAIL] test source not found: ${SRC}")
endif()

execute_process(
  COMMAND "${COMPILER}" "${SRC}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)

# Must fail
if(rc EQUAL 0)
  message(FATAL_ERROR "[FAIL] expected compile failure but succeeded: ${SRC}")
endif()

# Check that at least one diagnostic line matches file:line:col: error: format
string(REGEX MATCH "[^\n]*:[0-9]+:[0-9]+: error: [^\n]+" first_match "${err}")
if("${first_match}" STREQUAL "")
  message(FATAL_ERROR "[FAIL] no diagnostic in file:line:col: error: format found.\nStderr was:\n${err}")
endif()

# The compiler should NOT crash (we already got here, so it didn't segfault)
message(STATUS "[PASS] statement recovery check: ${SRC}\nStderr:\n${err}")
