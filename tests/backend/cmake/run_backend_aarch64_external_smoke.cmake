cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER SRC TARGET_TRIPLE OUT_ASM OUT_OBJ OUT_BIN CLANG AS HOST_PROCESSOR)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${SRC}")
  message(FATAL_ERROR "[FAIL] backend AArch64 smoke case not found: ${SRC}")
endif()

if(NOT DEFINED CASE_TIMEOUT_SEC OR "${CASE_TIMEOUT_SEC}" STREQUAL "")
  set(CASE_TIMEOUT_SEC 30)
endif()

if(NOT DEFINED REQUIRED_SNIPPETS OR "${REQUIRED_SNIPPETS}" STREQUAL "")
  set(REQUIRED_SNIPPETS ".globl main|main:|mov w0, #0|ret")
endif()

if(NOT DEFINED EXPECTED_RUN_RC OR "${EXPECTED_RUN_RC}" STREQUAL "")
  set(EXPECTED_RUN_RC 0)
endif()

get_filename_component(out_dir "${OUT_ASM}" DIRECTORY)
file(MAKE_DIRECTORY "${out_dir}")
file(REMOVE "${OUT_ASM}" "${OUT_OBJ}" "${OUT_BIN}")

execute_process(
  COMMAND "${COMPILER}" --codegen asm --target "${TARGET_TRIPLE}" "${SRC}" -o "${OUT_ASM}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE compiler_rc
  OUTPUT_VARIABLE compiler_out
  ERROR_VARIABLE compiler_err
)
if(compiler_rc MATCHES "timeout")
  message(FATAL_ERROR "[AARCH64_SMOKE_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT compiler_rc EQUAL 0)
  message(FATAL_ERROR "[AARCH64_SMOKE_EMIT_FAIL] ${SRC}\n${compiler_out}${compiler_err}")
endif()
if(NOT EXISTS "${OUT_ASM}")
  message(FATAL_ERROR "[AARCH64_SMOKE_ASM_MISSING] expected ${OUT_ASM}")
endif()

file(READ "${OUT_ASM}" asm_text)
string(REPLACE "|" ";" required_snippets "${REQUIRED_SNIPPETS}")
foreach(snippet IN LISTS required_snippets)
  if("${snippet}" STREQUAL "")
    continue()
  endif()
  string(FIND "${asm_text}" "${snippet}" snippet_pos)
  if(snippet_pos EQUAL -1)
    message(FATAL_ERROR
      "[AARCH64_SMOKE_SNIPPET_MISSING] ${OUT_ASM}\nMissing snippet: ${snippet}\n--- output ---\n${asm_text}")
  endif()
endforeach()

execute_process(
  COMMAND "${AS}" -o "${OUT_OBJ}" "${OUT_ASM}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE as_rc
  OUTPUT_VARIABLE as_out
  ERROR_VARIABLE as_err
)
if(as_rc MATCHES "timeout")
  message(FATAL_ERROR "[AARCH64_SMOKE_AS_TIMEOUT] ${OUT_ASM} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT as_rc EQUAL 0)
  message(FATAL_ERROR "[AARCH64_SMOKE_AS_FAIL] ${OUT_ASM}\n${as_out}${as_err}")
endif()

execute_process(
  COMMAND "${CLANG}" "${OUT_ASM}" -o "${OUT_BIN}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE clang_rc
  OUTPUT_VARIABLE clang_out
  ERROR_VARIABLE clang_err
)
if(clang_rc MATCHES "timeout")
  message(FATAL_ERROR "[AARCH64_SMOKE_CLANG_TIMEOUT] ${OUT_ASM} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT clang_rc EQUAL 0)
  message(FATAL_ERROR "[AARCH64_SMOKE_CLANG_FAIL] ${OUT_ASM}\n${clang_out}${clang_err}")
endif()

string(TOLOWER "${HOST_PROCESSOR}" host_processor_lower)
if(host_processor_lower MATCHES "^(aarch64|arm64)$")
  execute_process(
    COMMAND "${OUT_BIN}"
    TIMEOUT "${CASE_TIMEOUT_SEC}"
    RESULT_VARIABLE run_rc
    OUTPUT_VARIABLE run_out
    ERROR_VARIABLE run_err
  )
  if(run_rc MATCHES "timeout")
    message(FATAL_ERROR "[AARCH64_SMOKE_RUN_TIMEOUT] ${OUT_BIN} exceeded ${CASE_TIMEOUT_SEC}s")
  endif()
  if(NOT run_rc EQUAL EXPECTED_RUN_RC)
    message(FATAL_ERROR
      "[AARCH64_SMOKE_RUN_FAIL] ${OUT_BIN} rc=${run_rc} expected=${EXPECTED_RUN_RC}\n${run_out}${run_err}")
  endif()
else()
  message(STATUS "[AARCH64_SMOKE_RUN_SKIPPED] host processor is ${HOST_PROCESSOR}")
endif()

message(STATUS "[PASS][backend-aarch64-external-smoke] ${SRC}")
