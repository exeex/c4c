cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER SRC TARGET_TRIPLE OUT_LL)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${SRC}")
  message(FATAL_ERROR "[FAIL] backend LLVM IR case not found: ${SRC}")
endif()

if(NOT DEFINED CASE_TIMEOUT_SEC OR "${CASE_TIMEOUT_SEC}" STREQUAL "")
  set(CASE_TIMEOUT_SEC 30)
endif()

get_filename_component(out_ll_dir "${OUT_LL}" DIRECTORY)
file(MAKE_DIRECTORY "${out_ll_dir}")

execute_process(
  COMMAND "${COMPILER}" --target "${TARGET_TRIPLE}" "${SRC}" -o "${OUT_LL}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE compiler_rc
  OUTPUT_VARIABLE compiler_out
  ERROR_VARIABLE compiler_err
)
if(compiler_rc MATCHES "timeout")
  message(FATAL_ERROR "[BACKEND_LLVM_IR_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT compiler_rc EQUAL 0)
  message(FATAL_ERROR "[BACKEND_LLVM_IR_EMIT_FAIL] ${SRC}\n${compiler_out}${compiler_err}")
endif()

if(NOT EXISTS "${OUT_LL}")
  message(FATAL_ERROR "[BACKEND_LLVM_IR_OUTPUT_MISSING] expected IR output at ${OUT_LL}")
endif()

file(READ "${OUT_LL}" ir_text)

if(DEFINED REQUIRED_SNIPPETS AND NOT "${REQUIRED_SNIPPETS}" STREQUAL "")
  string(REPLACE "|" ";" required_snippets "${REQUIRED_SNIPPETS}")
  foreach(snippet IN LISTS required_snippets)
    if("${snippet}" STREQUAL "")
      continue()
    endif()
    string(FIND "${ir_text}" "${snippet}" snippet_pos)
    if(snippet_pos EQUAL -1)
      message(FATAL_ERROR
        "[BACKEND_LLVM_IR_SNIPPET_MISSING] ${OUT_LL}\nMissing snippet: ${snippet}\n--- output ---\n${ir_text}")
    endif()
  endforeach()
endif()

if(DEFINED FORBIDDEN_SNIPPETS AND NOT "${FORBIDDEN_SNIPPETS}" STREQUAL "")
  string(REPLACE "|" ";" forbidden_snippets "${FORBIDDEN_SNIPPETS}")
  foreach(snippet IN LISTS forbidden_snippets)
    if("${snippet}" STREQUAL "")
      continue()
    endif()
    string(FIND "${ir_text}" "${snippet}" snippet_pos)
    if(NOT snippet_pos EQUAL -1)
      message(FATAL_ERROR
        "[BACKEND_LLVM_IR_FORBIDDEN_SNIPPET] ${OUT_LL}\nUnexpected snippet: ${snippet}\n--- output ---\n${ir_text}")
    endif()
  endforeach()
endif()

message(STATUS "[PASS][backend-llvm-ir] ${SRC}")
