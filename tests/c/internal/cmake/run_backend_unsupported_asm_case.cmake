cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER SRC TARGET_TRIPLE)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${SRC}")
  message(FATAL_ERROR "[FAIL] backend case not found: ${SRC}")
endif()

if(NOT DEFINED CASE_TIMEOUT_SEC OR "${CASE_TIMEOUT_SEC}" STREQUAL "")
  set(CASE_TIMEOUT_SEC 30)
endif()

if(NOT DEFINED REQUIRED_STDERR_REGEX OR "${REQUIRED_STDERR_REGEX}" STREQUAL "")
  set(REQUIRED_STDERR_REGEX
      "error: --codegen asm requires backend-native assembly output\\.|error: .*direct (LIR|BIR) module")
endif()

execute_process(
  COMMAND "${COMPILER}" --codegen asm --target "${TARGET_TRIPLE}" "${SRC}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE frontend_rc
  OUTPUT_VARIABLE frontend_out
  ERROR_VARIABLE frontend_err
)
if(frontend_rc MATCHES "timeout")
  message(FATAL_ERROR "[BACKEND_ASM_UNSUPPORTED_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(frontend_rc EQUAL 0)
  message(FATAL_ERROR
    "[BACKEND_ASM_UNSUPPORTED_EXPECTED_FAIL] ${SRC}\n${frontend_out}${frontend_err}")
endif()

if(NOT "${frontend_out}" STREQUAL "")
  message(FATAL_ERROR
    "[BACKEND_ASM_UNSUPPORTED_OUTPUT] expected no stdout backend asm output\n${frontend_out}${frontend_err}")
endif()

if(NOT frontend_err MATCHES "${REQUIRED_STDERR_REGEX}")
  message(FATAL_ERROR
    "[BACKEND_ASM_UNSUPPORTED_DIAGNOSTIC] ${SRC}\nmissing expected diagnostic\n${frontend_out}${frontend_err}")
endif()

if(DEFINED FORBIDDEN_STDERR_SNIPPETS AND NOT "${FORBIDDEN_STDERR_SNIPPETS}" STREQUAL "")
  string(REPLACE "|" ";" forbidden_stderr_snippets "${FORBIDDEN_STDERR_SNIPPETS}")
  foreach(snippet IN LISTS forbidden_stderr_snippets)
    if("${snippet}" STREQUAL "")
      continue()
    endif()
    string(FIND "${frontend_err}" "${snippet}" snippet_pos)
    if(NOT snippet_pos EQUAL -1)
      message(FATAL_ERROR
        "[BACKEND_ASM_UNSUPPORTED_FORBIDDEN_DIAGNOSTIC] ${SRC}\nunexpected stderr snippet: ${snippet}\n${frontend_out}${frontend_err}")
    endif()
  endforeach()
endif()

message(STATUS "[PASS][backend-asm-unsupported] ${SRC}")
