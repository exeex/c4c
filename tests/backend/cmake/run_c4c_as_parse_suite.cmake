cmake_minimum_required(VERSION 3.20)

foreach(v C4C_AS C4CLL SOURCE_CASE WORK_DIR)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${SOURCE_CASE}")
  message(FATAL_ERROR "[C4C_AS_SOURCE_CASE_MISSING] ${SOURCE_CASE}")
endif()

if(NOT DEFINED CASE_TIMEOUT_SEC OR "${CASE_TIMEOUT_SEC}" STREQUAL "")
  set(CASE_TIMEOUT_SEC 10)
endif()

file(MAKE_DIRECTORY "${WORK_DIR}")

function(read_le_hex hex byte_offset byte_count out_var)
  set(value 0)
  set(multiplier 1)
  math(EXPR last "${byte_count} - 1")
  foreach(i RANGE 0 ${last})
    math(EXPR byte_pos "(${byte_offset} + ${i}) * 2")
    string(SUBSTRING "${hex}" ${byte_pos} 2 byte_hex)
    math(EXPR byte_value "0x${byte_hex}")
    math(EXPR value "${value} + ${byte_value} * ${multiplier}")
    math(EXPR multiplier "${multiplier} * 256")
  endforeach()
  set(${out_var} "${value}" PARENT_SCOPE)
endfunction()

function(read_c_string_hex hex byte_offset out_var)
  set(result "")
  set(offset "${byte_offset}")
  while(TRUE)
    math(EXPR byte_pos "${offset} * 2")
    string(SUBSTRING "${hex}" ${byte_pos} 2 byte_hex)
    math(EXPR byte_value "0x${byte_hex}")
    if(byte_value EQUAL 0)
      break()
    endif()
    string(ASCII ${byte_value} byte_char)
    string(APPEND result "${byte_char}")
    math(EXPR offset "${offset} + 1")
  endwhile()
  set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

function(read_elf_text_hex object_path out_var)
  file(READ "${object_path}" object_hex HEX)
  string(SUBSTRING "${object_hex}" 0 8 magic)
  if(NOT magic STREQUAL "7f454c46")
    message(FATAL_ERROR "[C4C_AS_BAD_OBJECT] ${object_path} is not an ELF file")
  endif()
  read_le_hex("${object_hex}" 40 8 shoff)
  read_le_hex("${object_hex}" 58 2 shentsize)
  read_le_hex("${object_hex}" 60 2 shnum)
  read_le_hex("${object_hex}" 62 2 shstrndx)
  if(shoff EQUAL 0 OR NOT shentsize EQUAL 64 OR shnum LESS_EQUAL 1 OR
     shstrndx GREATER_EQUAL shnum)
    message(FATAL_ERROR "[C4C_AS_BAD_OBJECT] invalid ELF section header table")
  endif()

  math(EXPR shstr_header "${shoff} + ${shstrndx} * ${shentsize}")
  read_le_hex("${object_hex}" "${shstr_header} + 24" 8 shstr_offset)
  set(text_offset "")
  set(text_size "")
  math(EXPR last_section "${shnum} - 1")
  foreach(section_index RANGE 1 ${last_section})
    math(EXPR header "${shoff} + ${section_index} * ${shentsize}")
    read_le_hex("${object_hex}" "${header}" 4 name_offset)
    math(EXPR name_addr "${shstr_offset} + ${name_offset}")
    read_c_string_hex("${object_hex}" "${name_addr}" section_name)
    if(section_name STREQUAL ".text")
      read_le_hex("${object_hex}" "${header} + 24" 8 text_offset)
      read_le_hex("${object_hex}" "${header} + 32" 8 text_size)
    endif()
  endforeach()
  if(text_offset STREQUAL "" OR text_size STREQUAL "")
    message(FATAL_ERROR "[C4C_AS_TEXT_MISSING] ${object_path} has no .text section")
  endif()
  math(EXPR text_hex_offset "${text_offset} * 2")
  math(EXPR text_hex_size "${text_size} * 2")
  string(SUBSTRING "${object_hex}" ${text_hex_offset} ${text_hex_size} text_hex)
  set(${out_var} "${text_hex}" PARENT_SCOPE)
endfunction()

function(run_success_case name source_text expected_stdout expected_text_hex)
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
  if(NOT EXISTS "${out_object}")
    message(FATAL_ERROR
      "[C4C_AS_OBJECT_MISSING] expected c4c-as to write '${out_object}'")
  endif()
  read_elf_text_hex("${out_object}" actual_text_hex)
  if(NOT actual_text_hex STREQUAL expected_text_hex)
    message(FATAL_ERROR
      "[C4C_AS_TEXT_BYTES] expected '${expected_text_hex}', got '${actual_text_hex}'")
  endif()
endfunction()

function(run_source_equivalence_case name asm_source_text source_case expected_text_hex)
  set(as_src "${WORK_DIR}/${name}.s")
  set(as_object "${WORK_DIR}/${name}.c4c-as.o")
  set(source_object "${WORK_DIR}/${name}.source.o")
  file(WRITE "${as_src}" "${asm_source_text}")
  file(REMOVE "${as_object}" "${source_object}")

  execute_process(
    COMMAND "${C4C_AS}" "${as_src}" -o "${as_object}"
    TIMEOUT "${CASE_TIMEOUT_SEC}"
    RESULT_VARIABLE as_rc
    OUTPUT_VARIABLE as_out
    ERROR_VARIABLE as_err
  )
  if(as_rc MATCHES "timeout")
    message(FATAL_ERROR "[C4C_AS_TIMEOUT] ${as_src} exceeded ${CASE_TIMEOUT_SEC}s")
  endif()
  if(NOT as_rc EQUAL 0)
    message(FATAL_ERROR "[C4C_AS_PARSE_FAIL] ${as_src}\n${as_out}${as_err}")
  endif()
  if(NOT EXISTS "${as_object}")
    message(FATAL_ERROR
      "[C4C_AS_OBJECT_MISSING] expected c4c-as to write '${as_object}'")
  endif()

  execute_process(
    COMMAND "${C4CLL}" --codegen obj --target riscv64-linux-gnu "${source_case}" -o "${source_object}"
    TIMEOUT "${CASE_TIMEOUT_SEC}"
    RESULT_VARIABLE source_rc
    OUTPUT_VARIABLE source_out
    ERROR_VARIABLE source_err
  )
  if(source_rc MATCHES "timeout")
    message(FATAL_ERROR
      "[C4C_AS_SOURCE_ROUTE_TIMEOUT] ${source_case} exceeded ${CASE_TIMEOUT_SEC}s")
  endif()
  if(NOT source_rc EQUAL 0)
    message(FATAL_ERROR
      "[C4C_AS_SOURCE_ROUTE_FAIL] ${source_case}\n${source_out}${source_err}")
  endif()
  if(NOT EXISTS "${source_object}")
    message(FATAL_ERROR
      "[C4C_AS_SOURCE_OBJECT_MISSING] expected c4cll to write '${source_object}'")
  endif()

  read_elf_text_hex("${as_object}" as_text_hex)
  read_elf_text_hex("${source_object}" source_text_hex)
  if(NOT as_text_hex STREQUAL expected_text_hex)
    message(FATAL_ERROR
      "[C4C_AS_EQ_AS_TEXT_BYTES] expected '${expected_text_hex}', got '${as_text_hex}'")
  endif()
  if(NOT source_text_hex STREQUAL expected_text_hex)
    message(FATAL_ERROR
      "[C4C_AS_EQ_SOURCE_TEXT_BYTES] expected '${expected_text_hex}', got '${source_text_hex}'")
  endif()
  if(NOT as_text_hex STREQUAL source_text_hex)
    message(FATAL_ERROR
      "[C4C_AS_SOURCE_EQ_TEXT_BYTES] c4c-as='${as_text_hex}' source='${source_text_hex}'")
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
  "assembled 3 instruction line(s)"
  "0a0320080b0300001305000067800000"
)
run_success_case(
  canonical_text_bytes
  ".text\n.globl main\nmain:\n  .insn.d 10, 11, v6, v0, v2, v4, 3\n  li a0, 0\n  ret\n"
  "text byte(s): 0a0320080b0300001305000067800000"
  "0a0320080b0300001305000067800000"
)
run_source_equivalence_case(
  source_route_equivalence
  ".text\n.globl main\nmain:\n  .insn.d 10, 11, v6, v0, v2, v4, 3\n  li a0, 0\n  ret\n"
  "${SOURCE_CASE}"
  "0a0320080b0300001305000067800000"
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
