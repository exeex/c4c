cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER SRC TARGET_TRIPLE DUMP_FLAG)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${SRC}")
  message(FATAL_ERROR "[FAIL] backend dump case not found: ${SRC}")
endif()

if(NOT DEFINED CASE_TIMEOUT_SEC OR "${CASE_TIMEOUT_SEC}" STREQUAL "")
  set(CASE_TIMEOUT_SEC 30)
endif()

execute_process(
  COMMAND "${COMPILER}" "${DUMP_FLAG}" --target "${TARGET_TRIPLE}" "${SRC}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE compiler_rc
  OUTPUT_VARIABLE compiler_out
  ERROR_VARIABLE compiler_err
)
if(compiler_rc MATCHES "timeout")
  message(FATAL_ERROR "[BACKEND_DUMP_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT compiler_rc EQUAL 0)
  message(FATAL_ERROR "[BACKEND_DUMP_FAIL] ${SRC}\n${compiler_out}${compiler_err}")
endif()

if(DEFINED REQUIRED_SNIPPETS AND NOT "${REQUIRED_SNIPPETS}" STREQUAL "")
  string(REPLACE "|" ";" required_snippets "${REQUIRED_SNIPPETS}")
  foreach(snippet IN LISTS required_snippets)
    if("${snippet}" STREQUAL "")
      continue()
    endif()
    string(FIND "${compiler_out}" "${snippet}" snippet_pos)
    if(snippet_pos EQUAL -1)
      message(FATAL_ERROR
        "[BACKEND_DUMP_SNIPPET_MISSING] ${DUMP_FLAG} ${SRC}\nMissing snippet: ${snippet}\n--- output ---\n${compiler_out}")
    endif()
  endforeach()
endif()

if(DEFINED FORBIDDEN_SNIPPETS AND NOT "${FORBIDDEN_SNIPPETS}" STREQUAL "")
  string(REPLACE "|" ";" forbidden_snippets "${FORBIDDEN_SNIPPETS}")
  foreach(snippet IN LISTS forbidden_snippets)
    if("${snippet}" STREQUAL "")
      continue()
    endif()
    string(FIND "${compiler_out}" "${snippet}" snippet_pos)
    if(NOT snippet_pos EQUAL -1)
      message(FATAL_ERROR
        "[BACKEND_DUMP_FORBIDDEN_SNIPPET] ${DUMP_FLAG} ${SRC}\nUnexpected snippet: ${snippet}\n--- output ---\n${compiler_out}")
    endif()
  endforeach()
endif()

message(STATUS "[PASS][backend-dump] ${DUMP_FLAG} ${SRC}")
