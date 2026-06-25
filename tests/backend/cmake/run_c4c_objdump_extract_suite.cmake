cmake_minimum_required(VERSION 3.20)

foreach(v C4C_OBJDUMP C4CLL SOURCE_CASE AARCH64_SOURCE_CASE WORK_DIR)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

foreach(path IN ITEMS "${SOURCE_CASE}" "${AARCH64_SOURCE_CASE}")
  if(NOT EXISTS "${path}")
    message(FATAL_ERROR "[C4C_OBJDUMP_SOURCE_CASE_MISSING] ${path}")
  endif()
endforeach()

if(NOT DEFINED CASE_TIMEOUT_SEC OR "${CASE_TIMEOUT_SEC}" STREQUAL "")
  set(CASE_TIMEOUT_SEC 10)
endif()

file(MAKE_DIRECTORY "${WORK_DIR}")

function(expect_contains haystack needle tag)
  string(FIND "${haystack}" "${needle}" needle_offset)
  if(needle_offset EQUAL -1)
    message(FATAL_ERROR "[${tag}] expected '${needle}' in:\n${haystack}")
  endif()
endfunction()

function(run_objdump_success_case name object_path output_path expected_hex)
  file(REMOVE "${output_path}")
  execute_process(
    COMMAND "${C4C_OBJDUMP}" "${object_path}" -o "${output_path}"
    TIMEOUT "${CASE_TIMEOUT_SEC}"
    RESULT_VARIABLE objdump_rc
    OUTPUT_VARIABLE objdump_out
    ERROR_VARIABLE objdump_err
  )
  if(objdump_rc MATCHES "timeout")
    message(FATAL_ERROR "[C4C_OBJDUMP_TIMEOUT] ${object_path} exceeded ${CASE_TIMEOUT_SEC}s")
  endif()
  if(NOT objdump_rc EQUAL 0)
    message(FATAL_ERROR "[C4C_OBJDUMP_UNEXPECTED_FAIL] ${object_path}\n${objdump_out}${objdump_err}")
  endif()
  if(NOT EXISTS "${output_path}")
    message(FATAL_ERROR "[C4C_OBJDUMP_OUTPUT_MISSING] expected '${output_path}'")
  endif()

  file(READ "${output_path}" asm_text)
  expect_contains("${asm_text}" ".text" "C4C_OBJDUMP_TEXT_DIRECTIVE_MISSING")
  expect_contains("${asm_text}" ".globl main" "C4C_OBJDUMP_GLOBAL_MISSING")
  expect_contains("${asm_text}" "main:" "C4C_OBJDUMP_LABEL_MISSING")
  expect_contains("${asm_text}" "c4c-objdump: extracted 16 .text byte(s)" "C4C_OBJDUMP_SIZE_MISSING")
  expect_contains("${asm_text}" "c4c-objdump: text-bytes ${expected_hex}" "C4C_OBJDUMP_HEX_MISSING")
  expect_contains("${objdump_out}" "extracted 16 .text byte(s)" "C4C_OBJDUMP_STDOUT_SIZE_MISSING")
  expect_contains("${objdump_out}" "${expected_hex}" "C4C_OBJDUMP_STDOUT_HEX_MISSING")
endfunction()

function(run_objdump_failure_case name input_path output_path expected_diagnostic)
  file(REMOVE "${output_path}")
  execute_process(
    COMMAND "${C4C_OBJDUMP}" "${input_path}" -o "${output_path}"
    TIMEOUT "${CASE_TIMEOUT_SEC}"
    RESULT_VARIABLE objdump_rc
    OUTPUT_VARIABLE objdump_out
    ERROR_VARIABLE objdump_err
  )
  if(objdump_rc MATCHES "timeout")
    message(FATAL_ERROR "[C4C_OBJDUMP_TIMEOUT] ${input_path} exceeded ${CASE_TIMEOUT_SEC}s")
  endif()
  if(objdump_rc EQUAL 0)
    message(FATAL_ERROR "[C4C_OBJDUMP_UNEXPECTED_SUCCESS] ${input_path}\n${objdump_out}${objdump_err}")
  endif()
  if(EXISTS "${output_path}")
    message(FATAL_ERROR "[C4C_OBJDUMP_FAILURE_WROTE_OUTPUT] unexpected '${output_path}'")
  endif()
  set(combined_output "${objdump_out}${objdump_err}")
  expect_contains("${combined_output}" "${expected_diagnostic}" "C4C_OBJDUMP_DIAGNOSTIC_MISSING")
endfunction()

set(rv64_object "${WORK_DIR}/rv64_vrm_insn_d_source.o")
set(aarch64_object "${WORK_DIR}/aarch64_return_zero_smoke.o")
set(malformed_input "${WORK_DIR}/malformed.bin")
set(expected_hex "0a0320080b0300001305000067800000")
file(REMOVE "${rv64_object}" "${aarch64_object}")

execute_process(
  COMMAND "${C4CLL}" --codegen obj --target riscv64-linux-gnu "${SOURCE_CASE}" -o "${rv64_object}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE rv64_rc
  OUTPUT_VARIABLE rv64_out
  ERROR_VARIABLE rv64_err
)
if(rv64_rc MATCHES "timeout")
  message(FATAL_ERROR "[C4C_OBJDUMP_RV64_SOURCE_TIMEOUT] ${SOURCE_CASE} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT rv64_rc EQUAL 0)
  message(FATAL_ERROR "[C4C_OBJDUMP_RV64_SOURCE_FAIL] ${SOURCE_CASE}\n${rv64_out}${rv64_err}")
endif()
if(NOT EXISTS "${rv64_object}")
  message(FATAL_ERROR "[C4C_OBJDUMP_RV64_OBJECT_MISSING] expected '${rv64_object}'")
endif()

execute_process(
  COMMAND "${C4CLL}" --codegen obj --target aarch64-linux-gnu "${AARCH64_SOURCE_CASE}" -o "${aarch64_object}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE aarch64_rc
  OUTPUT_VARIABLE aarch64_out
  ERROR_VARIABLE aarch64_err
)
if(aarch64_rc MATCHES "timeout")
  message(FATAL_ERROR "[C4C_OBJDUMP_AARCH64_SOURCE_TIMEOUT] ${AARCH64_SOURCE_CASE} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT aarch64_rc EQUAL 0)
  message(FATAL_ERROR "[C4C_OBJDUMP_AARCH64_SOURCE_FAIL] ${AARCH64_SOURCE_CASE}\n${aarch64_out}${aarch64_err}")
endif()
if(NOT EXISTS "${aarch64_object}")
  message(FATAL_ERROR "[C4C_OBJDUMP_AARCH64_OBJECT_MISSING] expected '${aarch64_object}'")
endif()

run_objdump_success_case(
  rv64_extract
  "${rv64_object}"
  "${WORK_DIR}/rv64_vrm_insn_d_source.s"
  "${expected_hex}"
)

file(WRITE "${malformed_input}" "this is not an ELF object\n")
run_objdump_failure_case(
  malformed
  "${malformed_input}"
  "${WORK_DIR}/malformed.s"
  "input is not an ELF file"
)

run_objdump_failure_case(
  unsupported_machine
  "${aarch64_object}"
  "${WORK_DIR}/aarch64_return_zero_smoke.s"
  "unsupported ELF machine"
)

message(STATUS "[PASS][c4c-objdump-suite] ${WORK_DIR}")
