cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER SRC TARGET_TRIPLE OUT_ASM)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${SRC}")
  message(FATAL_ERROR "[FAIL] backend AArch64 selected indirect-call case not found: ${SRC}")
endif()

if(NOT DEFINED CASE_TIMEOUT_SEC OR "${CASE_TIMEOUT_SEC}" STREQUAL "")
  set(CASE_TIMEOUT_SEC 30)
endif()

get_filename_component(out_dir "${OUT_ASM}" DIRECTORY)
file(MAKE_DIRECTORY "${out_dir}")
file(REMOVE "${OUT_ASM}")

execute_process(
  COMMAND "${COMPILER}" --codegen asm --target "${TARGET_TRIPLE}" "${SRC}" -o "${OUT_ASM}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE compiler_rc
  OUTPUT_VARIABLE compiler_out
  ERROR_VARIABLE compiler_err
)
if(compiler_rc MATCHES "timeout")
  message(FATAL_ERROR "[AARCH64_SELECTED_INDIRECT_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT compiler_rc EQUAL 0)
  message(FATAL_ERROR "[AARCH64_SELECTED_INDIRECT_EMIT_FAIL] ${SRC}\n${compiler_out}${compiler_err}")
endif()
if(NOT EXISTS "${OUT_ASM}")
  message(FATAL_ERROR "[AARCH64_SELECTED_INDIRECT_ASM_MISSING] expected ${OUT_ASM}")
endif()

file(READ "${OUT_ASM}" asm_text)

foreach(snippet IN ITEMS
    "dispatch_table:"
    ".xword choose_one"
    ".xword choose_two"
    ".xword choose_three")
  string(FIND "${asm_text}" "${snippet}" snippet_pos)
  if(snippet_pos EQUAL -1)
    message(FATAL_ERROR
      "[AARCH64_SELECTED_INDIRECT_GLOBAL_DATA_MISSING] ${OUT_ASM}\nMissing snippet: ${snippet}\n--- output ---\n${asm_text}")
  endif()
endforeach()

string(FIND "${asm_text}" "call_index:" call_start)
string(FIND "${asm_text}" ".size call_index" call_end)
if(call_start EQUAL -1 OR call_end EQUAL -1 OR call_end LESS_EQUAL call_start)
  message(FATAL_ERROR
    "[AARCH64_SELECTED_INDIRECT_FUNCTION_MISSING] ${OUT_ASM}\n--- output ---\n${asm_text}")
endif()

math(EXPR call_len "${call_end} - ${call_start}")
string(SUBSTRING "${asm_text}" "${call_start}" "${call_len}" call_text)

string(FIND "${call_text}" "blr " blr_pos)
if(blr_pos EQUAL -1)
  message(FATAL_ERROR
    "[AARCH64_SELECTED_INDIRECT_BLR_MISSING] ${OUT_ASM}\n--- call_index ---\n${call_text}")
endif()

string(FIND "${call_text}" "cmp x" cmp_pos)
string(FIND "${call_text}" "csel x" csel_pos)
if(cmp_pos EQUAL -1 OR csel_pos EQUAL -1 OR csel_pos GREATER blr_pos)
  message(FATAL_ERROR
    "[AARCH64_SELECTED_INDIRECT_CALLEE_SELECT_MISSING] ${OUT_ASM}\n"
    "Expected call_index to materialize the dynamically selected table callee before the indirect blr.\n"
    "--- call_index ---\n${call_text}")
endif()

message(STATUS "[PASS][backend-aarch64-selected-indirect-call] ${SRC}")
