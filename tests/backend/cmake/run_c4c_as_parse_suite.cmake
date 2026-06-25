cmake_minimum_required(VERSION 3.20)

foreach(v C4C_AS WORK_DIR)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT DEFINED CASE_TIMEOUT_SEC OR "${CASE_TIMEOUT_SEC}" STREQUAL "")
  set(CASE_TIMEOUT_SEC 10)
endif()

file(MAKE_DIRECTORY "${WORK_DIR}")

function(run_success_case name source_text expected_stdout)
  set(src "${WORK_DIR}/${name}.s")
  set(out_object "${WORK_DIR}/${name}.o")
  file(WRITE "${src}" "${source_text}")
  file(REMOVE "${out_object}")

  execute_process(
    COMMAND "${C4C_AS}" "${src}" -o "${out_object}"
    TIMEOUT "${CASE_TIMEOUT_SEC}"
    RESULT_VARIABLE as_rc
    OUTPUT_VARIABLE as_out
    ERROR_VARIABLE as_err
  )
  if(as_rc MATCHES "timeout")
    message(FATAL_ERROR "[C4C_AS_TIMEOUT] ${src} exceeded ${CASE_TIMEOUT_SEC}s")
  endif()
  if(NOT as_rc EQUAL 0)
    message(FATAL_ERROR "[C4C_AS_PARSE_FAIL] ${src}\n${as_out}${as_err}")
  endif()
  string(FIND "${as_out}" "${expected_stdout}" expected_stdout_offset)
  if(expected_stdout_offset EQUAL -1)
    message(FATAL_ERROR
      "[C4C_AS_STDOUT_MISSING] expected '${expected_stdout}' in stdout:\n${as_out}")
  endif()
  if(EXISTS "${out_object}")
    message(FATAL_ERROR
      "[C4C_AS_UNEXPECTED_OBJECT] parse-only Step 2 wrote '${out_object}'")
  endif()
endfunction()

function(run_failure_case name source_text expected_diagnostics)
  set(src "${WORK_DIR}/${name}.s")
  set(out_object "${WORK_DIR}/${name}.o")
  file(WRITE "${src}" "${source_text}")
  file(REMOVE "${out_object}")

  execute_process(
    COMMAND "${C4C_AS}" "${src}" -o "${out_object}"
    TIMEOUT "${CASE_TIMEOUT_SEC}"
    RESULT_VARIABLE as_rc
    OUTPUT_VARIABLE as_out
    ERROR_VARIABLE as_err
  )
  if(as_rc MATCHES "timeout")
    message(FATAL_ERROR "[C4C_AS_TIMEOUT] ${src} exceeded ${CASE_TIMEOUT_SEC}s")
  endif()
  if(as_rc EQUAL 0)
    message(FATAL_ERROR "[C4C_AS_UNEXPECTED_SUCCESS] ${src}\n${as_out}${as_err}")
  endif()

  set(combined_output "${as_out}${as_err}")
  string(REPLACE "|" ";" expected_list "${expected_diagnostics}")
  foreach(expected IN LISTS expected_list)
    string(FIND "${combined_output}" "${expected}" expected_offset)
    if(expected_offset EQUAL -1)
      message(FATAL_ERROR
        "[C4C_AS_DIAGNOSTIC_MISSING] expected '${expected}' in output:\n${combined_output}")
    endif()
  endforeach()
endfunction()

run_success_case(
  canonical
  ".text\n# canonical RV64 source accepted by c4c-as\n.globl main\nmain:\n  .insn.d 10, 11, v6, v0, v2, v4, 3 # custom instruction\n\n  li a0, 0\n  ret\n"
  "parsed 3 instruction line(s)"
)
run_failure_case(
  instruction_outside_text
  "li a0, 0\n.text\nret\n"
  "instruction outside .text section"
)
run_failure_case(
  unsupported_directive
  ".text\n.section .rodata\n"
  "unsupported directive"
)

message(STATUS "[PASS][c4c-as-suite] ${WORK_DIR}")
