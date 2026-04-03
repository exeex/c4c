cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER SRC TARGET_TRIPLE OUT_TEXT)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${SRC}")
  message(FATAL_ERROR "[FAIL] backend route case not found: ${SRC}")
endif()

if(NOT DEFINED CASE_TIMEOUT_SEC OR "${CASE_TIMEOUT_SEC}" STREQUAL "")
  set(CASE_TIMEOUT_SEC 30)
endif()

get_filename_component(out_dir "${OUT_TEXT}" DIRECTORY)
file(MAKE_DIRECTORY "${out_dir}")

execute_process(
  COMMAND "${COMPILER}" --codegen asm --target "${TARGET_TRIPLE}" "${SRC}" -o "${OUT_TEXT}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE compiler_rc
  OUTPUT_VARIABLE compiler_out
  ERROR_VARIABLE compiler_err
)
if(compiler_rc MATCHES "timeout")
  message(FATAL_ERROR "[BACKEND_ROUTE_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT compiler_rc EQUAL 0)
  message(FATAL_ERROR "[BACKEND_ROUTE_EMIT_FAIL] ${SRC}\n${compiler_out}${compiler_err}")
endif()

if(NOT EXISTS "${OUT_TEXT}")
  message(FATAL_ERROR "[BACKEND_ROUTE_OUTPUT_MISSING] expected backend output at ${OUT_TEXT}")
endif()

file(READ "${OUT_TEXT}" output_text)

if(DEFINED REQUIRED_SNIPPETS AND NOT "${REQUIRED_SNIPPETS}" STREQUAL "")
  string(REPLACE "|" ";" required_snippets "${REQUIRED_SNIPPETS}")
  foreach(snippet IN LISTS required_snippets)
    if("${snippet}" STREQUAL "")
      continue()
    endif()
    string(FIND "${output_text}" "${snippet}" snippet_pos)
    if(snippet_pos EQUAL -1)
      message(FATAL_ERROR
        "[BACKEND_ROUTE_SNIPPET_MISSING] ${OUT_TEXT}\nMissing snippet: ${snippet}\n--- output ---\n${output_text}")
    endif()
  endforeach()
endif()

if(DEFINED FORBIDDEN_SNIPPETS AND NOT "${FORBIDDEN_SNIPPETS}" STREQUAL "")
  string(REPLACE "|" ";" forbidden_snippets "${FORBIDDEN_SNIPPETS}")
  foreach(snippet IN LISTS forbidden_snippets)
    if("${snippet}" STREQUAL "")
      continue()
    endif()
    string(FIND "${output_text}" "${snippet}" snippet_pos)
    if(NOT snippet_pos EQUAL -1)
      message(FATAL_ERROR
        "[BACKEND_ROUTE_FORBIDDEN_SNIPPET] ${OUT_TEXT}\nUnexpected snippet: ${snippet}\n--- output ---\n${output_text}")
    endif()
  endforeach()
endif()

message(STATUS "[PASS][backend-route] ${SRC}")
