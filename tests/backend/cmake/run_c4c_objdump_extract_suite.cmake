cmake_minimum_required(VERSION 3.20)

foreach(v C4C_OBJDUMP C4C_AS C4CLL SOURCE_CASE AARCH64_SOURCE_CASE WORK_DIR)
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

find_program(PYTHON3_EXECUTABLE NAMES python3 python)
if(NOT PYTHON3_EXECUTABLE)
  message(FATAL_ERROR "python3 or python is required for c4c-objdump binary fixture patching")
endif()

file(MAKE_DIRECTORY "${WORK_DIR}")

function(expect_contains haystack needle tag)
  string(FIND "${haystack}" "${needle}" needle_offset)
  if(needle_offset EQUAL -1)
    message(FATAL_ERROR "[${tag}] expected '${needle}' in:\n${haystack}")
  endif()
endfunction()

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
    message(FATAL_ERROR "[C4C_OBJDUMP_BAD_OBJECT] ${object_path} is not an ELF file")
  endif()
  read_le_hex("${object_hex}" 40 8 shoff)
  read_le_hex("${object_hex}" 58 2 shentsize)
  read_le_hex("${object_hex}" 60 2 shnum)
  read_le_hex("${object_hex}" 62 2 shstrndx)
  if(shoff EQUAL 0 OR NOT shentsize EQUAL 64 OR shnum LESS_EQUAL 1 OR
     shstrndx GREATER_EQUAL shnum)
    message(FATAL_ERROR "[C4C_OBJDUMP_BAD_OBJECT] invalid ELF section header table")
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
    message(FATAL_ERROR "[C4C_OBJDUMP_TEXT_MISSING] ${object_path} has no .text section")
  endif()
  math(EXPR text_hex_offset "${text_offset} * 2")
  math(EXPR text_hex_size "${text_size} * 2")
  string(SUBSTRING "${object_hex}" ${text_hex_offset} ${text_hex_size} text_hex)
  set(${out_var} "${text_hex}" PARENT_SCOPE)
endfunction()

function(run_objdump_success_case name object_path output_path expected_hex expected_decode_count expected_snippets)
  set(roundtrip_object "${WORK_DIR}/${name}.roundtrip.o")
  set(roundtrip_output "${WORK_DIR}/${name}.roundtrip.s")
  file(REMOVE "${output_path}")
  file(REMOVE "${roundtrip_object}")
  file(REMOVE "${roundtrip_output}")
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
  string(REPLACE "|" ";" expected_list "${expected_snippets}")
  foreach(expected_snippet IN LISTS expected_list)
    expect_contains("${asm_text}" "${expected_snippet}" "C4C_OBJDUMP_ASM_SNIPPET_MISSING")
  endforeach()
  string(LENGTH "${expected_hex}" expected_hex_length)
  math(EXPR expected_byte_count "${expected_hex_length} / 2")
  expect_contains("${objdump_out}" "extracted ${expected_byte_count} .text byte(s)" "C4C_OBJDUMP_STDOUT_SIZE_MISSING")
  expect_contains("${objdump_out}" "${expected_hex}" "C4C_OBJDUMP_STDOUT_HEX_MISSING")
  expect_contains("${objdump_out}" "decoded ${expected_decode_count} instruction(s)" "C4C_OBJDUMP_STDOUT_DECODE_COUNT_MISSING")

  execute_process(
    COMMAND "${C4C_AS}" "${output_path}" -o "${roundtrip_object}"
    TIMEOUT "${CASE_TIMEOUT_SEC}"
    RESULT_VARIABLE as_rc
    OUTPUT_VARIABLE as_out
    ERROR_VARIABLE as_err
  )
  if(as_rc MATCHES "timeout")
    message(FATAL_ERROR "[C4C_OBJDUMP_ROUNDTRIP_AS_TIMEOUT] ${output_path} exceeded ${CASE_TIMEOUT_SEC}s")
  endif()
  if(NOT as_rc EQUAL 0)
    message(FATAL_ERROR "[C4C_OBJDUMP_ROUNDTRIP_AS_FAIL] ${output_path}\n${as_out}${as_err}")
  endif()
  if(NOT EXISTS "${roundtrip_object}")
    message(FATAL_ERROR "[C4C_OBJDUMP_ROUNDTRIP_OBJECT_MISSING] expected '${roundtrip_object}'")
  endif()
  read_elf_text_hex("${roundtrip_object}" roundtrip_hex)
  if(NOT roundtrip_hex STREQUAL expected_hex)
    message(FATAL_ERROR
      "[C4C_OBJDUMP_ROUNDTRIP_TEXT_BYTES] expected '${expected_hex}', got '${roundtrip_hex}'")
  endif()

  execute_process(
    COMMAND "${C4C_OBJDUMP}" "${roundtrip_object}" -o "${roundtrip_output}"
    TIMEOUT "${CASE_TIMEOUT_SEC}"
    RESULT_VARIABLE second_objdump_rc
    OUTPUT_VARIABLE second_objdump_out
    ERROR_VARIABLE second_objdump_err
  )
  if(second_objdump_rc MATCHES "timeout")
    message(FATAL_ERROR
      "[C4C_OBJDUMP_SECOND_ROUNDTRIP_TIMEOUT] ${roundtrip_object} exceeded ${CASE_TIMEOUT_SEC}s")
  endif()
  if(NOT second_objdump_rc EQUAL 0)
    message(FATAL_ERROR
      "[C4C_OBJDUMP_SECOND_ROUNDTRIP_FAIL] ${roundtrip_object}\n${second_objdump_out}${second_objdump_err}")
  endif()
  if(NOT EXISTS "${roundtrip_output}")
    message(FATAL_ERROR "[C4C_OBJDUMP_SECOND_ROUNDTRIP_OUTPUT_MISSING] expected '${roundtrip_output}'")
  endif()

  file(READ "${roundtrip_output}" roundtrip_asm_text)
  if(NOT asm_text STREQUAL roundtrip_asm_text)
    message(FATAL_ERROR
      "[C4C_OBJDUMP_ROUNDTRIP_ASM_STABILITY] expected '${output_path}' and '${roundtrip_output}' to match exactly")
  endif()
endfunction()

function(run_objdump_asm_success_case name source_text expected_decode_count expected_snippets)
  set(src "${WORK_DIR}/${name}.input.s")
  set(object_path "${WORK_DIR}/${name}.o")
  set(output_path "${WORK_DIR}/${name}.s")
  file(WRITE "${src}" "${source_text}")
  file(REMOVE "${object_path}" "${output_path}")

  execute_process(
    COMMAND "${C4C_AS}" "${src}" -o "${object_path}"
    TIMEOUT "${CASE_TIMEOUT_SEC}"
    RESULT_VARIABLE as_rc
    OUTPUT_VARIABLE as_out
    ERROR_VARIABLE as_err
  )
  if(as_rc MATCHES "timeout")
    message(FATAL_ERROR "[C4C_OBJDUMP_ASM_SOURCE_TIMEOUT] ${src} exceeded ${CASE_TIMEOUT_SEC}s")
  endif()
  if(NOT as_rc EQUAL 0)
    message(FATAL_ERROR "[C4C_OBJDUMP_ASM_SOURCE_FAIL] ${src}\n${as_out}${as_err}")
  endif()
  if(NOT EXISTS "${object_path}")
    message(FATAL_ERROR "[C4C_OBJDUMP_ASM_OBJECT_MISSING] expected '${object_path}'")
  endif()
  read_elf_text_hex("${object_path}" expected_hex)
  run_objdump_success_case(
    "${name}"
    "${object_path}"
    "${output_path}"
    "${expected_hex}"
    "${expected_decode_count}"
    "${expected_snippets}"
  )
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

function(run_objdump_asm_failure_case name source_text expected_diagnostic)
  set(src "${WORK_DIR}/${name}.input.s")
  set(object_path "${WORK_DIR}/${name}.o")
  set(output_path "${WORK_DIR}/${name}.s")
  file(WRITE "${src}" "${source_text}")
  file(REMOVE "${object_path}" "${output_path}")

  execute_process(
    COMMAND "${C4C_AS}" "${src}" -o "${object_path}"
    TIMEOUT "${CASE_TIMEOUT_SEC}"
    RESULT_VARIABLE as_rc
    OUTPUT_VARIABLE as_out
    ERROR_VARIABLE as_err
  )
  if(as_rc MATCHES "timeout")
    message(FATAL_ERROR "[C4C_OBJDUMP_ASM_FAILURE_SOURCE_TIMEOUT] ${src} exceeded ${CASE_TIMEOUT_SEC}s")
  endif()
  if(NOT as_rc EQUAL 0)
    message(FATAL_ERROR "[C4C_OBJDUMP_ASM_FAILURE_SOURCE_FAIL] ${src}\n${as_out}${as_err}")
  endif()
  if(NOT EXISTS "${object_path}")
    message(FATAL_ERROR "[C4C_OBJDUMP_ASM_FAILURE_OBJECT_MISSING] expected '${object_path}'")
  endif()
  run_objdump_failure_case(
    "${name}"
    "${object_path}"
    "${output_path}"
    "${expected_diagnostic}"
  )
endfunction()

set(rv64_object "${WORK_DIR}/rv64_vrm_insn_d_source.o")
set(aarch64_object "${WORK_DIR}/aarch64_return_zero_smoke.o")
set(unsupported_rv64_object "${WORK_DIR}/rv64_unsupported_instruction.o")
set(malformed_input "${WORK_DIR}/malformed.bin")
set(expected_hex "3f0320140b0403001305000067800000")
# Same instruction shape with EV64 reserved bits [47:45] set nonzero.
set(unsupported_hex "3f0320140b2403001305000067800000")
file(REMOVE "${rv64_object}" "${aarch64_object}" "${unsupported_rv64_object}")

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
  3
  ".insn.d 10, 11, v6, v0, v2, v4, 3|li a0, 0|ret"
)

run_objdump_asm_success_case(
  rv64i_step4_non_control_subset
  ".text\n.globl main\nmain:\n  lui t0, 0x7ffff\n  auipc t1, -0x80000\n  addi t2, zero, -2048\n  slti s1, s0, -1\n  ori s4, s0, 0x7f\n  slli s6, s5, 63\n  srai s8, s6, 63\n  addiw s9, s8, -2048\n  sraiw a0, s10, 31\n  add a1, a0, s1\n  sub a2, a1, s2\n  sraw tp, gp, a4\n  lb a0, -2048(sp)\n  lwu a6, 60(sp)\n  sd a3, 2040(sp)\n  jalr t0, 4(ra)\n  .insn.d 10, 11, v6, v0, v2, v4, 3\n  li a0, 0\n  ret\n"
  19
  "lui t0, 524287|auipc t1, -524288|addi t2, zero, -2048|slti s1, s0, -1|ori s4, s0, 127|slli s6, s5, 63|srai s8, s6, 63|addiw s9, s8, -2048|sraiw a0, s10, 31|add a1, a0, s1|sub a2, a1, s2|sraw tp, gp, a4|lb a0, -2048(sp)|lwu a6, 60(sp)|sd a3, 2040(sp)|jalr t0, 4(ra)|.insn.d 10, 11, v6, v0, v2, v4, 3|li a0, 0|ret"
)

run_objdump_asm_success_case(
  rv64i_step4_control_flow_subset
  ".text\n.globl main\nmain:\n  beq a0, a1, .Ldone\n  bne a0, a1, main\n.Ldone:\n  jal ra, .Lexit\n  blt a0, a1, .Ldone\n  bge a0, a1, .Lexit\n.Lexit:\n  bltu a0, a1, .Ldone\n  bgeu a0, a1, .Lexit\n  ret\n"
  8
  "beq a0, a1, .Ldone|bne a0, a1, main|.Ldone:|jal ra, .Lexit|blt a0, a1, .Ldone|bge a0, a1, .Lexit|.Lexit:|bltu a0, a1, .Ldone|bgeu a0, a1, .Lexit|ret"
)

set(control_untruthful_source "${WORK_DIR}/rv64i_control_untruthful.input.s")
set(control_truthful_object "${WORK_DIR}/rv64i_control_truthful.o")
set(control_untruthful_object "${WORK_DIR}/rv64i_control_untruthful.o")
set(control_untruthful_output "${WORK_DIR}/rv64i_control_untruthful.s")
file(WRITE "${control_untruthful_source}" ".text\n.globl main\nmain:\n  beq a0, a1, .Llabel\n.Llabel:\n  ret\n  ret\n")
file(REMOVE "${control_truthful_object}" "${control_untruthful_object}" "${control_untruthful_output}")
execute_process(
  COMMAND "${C4C_AS}" "${control_untruthful_source}" -o "${control_truthful_object}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE control_as_rc
  OUTPUT_VARIABLE control_as_out
  ERROR_VARIABLE control_as_err
)
if(control_as_rc MATCHES "timeout")
  message(FATAL_ERROR "[C4C_OBJDUMP_CONTROL_SOURCE_TIMEOUT] ${control_untruthful_source} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT control_as_rc EQUAL 0)
  message(FATAL_ERROR "[C4C_OBJDUMP_CONTROL_SOURCE_FAIL] ${control_untruthful_source}\n${control_as_out}${control_as_err}")
endif()
execute_process(
  COMMAND "${PYTHON3_EXECUTABLE}" -c [=[
import sys
src, dst = sys.argv[1:]
data = bytearray(open(src, "rb").read())

def encode_b_type(funct3, rs1, rs2, imm):
    imm &= 0x1fff
    return (
        ((imm >> 12) & 1) << 31
        | ((imm >> 5) & 0x3f) << 25
        | (rs2 & 0x1f) << 20
        | (rs1 & 0x1f) << 15
        | (funct3 & 7) << 12
        | ((imm >> 1) & 0xf) << 8
        | ((imm >> 11) & 1) << 7
        | 0x63
    ).to_bytes(4, "little")

original = encode_b_type(0, 10, 11, 4)
patched = encode_b_type(0, 10, 11, 8)
offset = data.find(original)
if offset < 0:
    raise SystemExit("expected original branch bytes not found")
data[offset:offset + 4] = patched
open(dst, "wb").write(data)
]=] "${control_truthful_object}" "${control_untruthful_object}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE control_patch_rc
  OUTPUT_VARIABLE control_patch_out
  ERROR_VARIABLE control_patch_err
)
if(control_patch_rc MATCHES "timeout")
  message(FATAL_ERROR "[C4C_OBJDUMP_CONTROL_PATCH_TIMEOUT] exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT control_patch_rc EQUAL 0)
  message(FATAL_ERROR "[C4C_OBJDUMP_CONTROL_PATCH_FAIL]\n${control_patch_out}${control_patch_err}")
endif()
run_objdump_failure_case(
  branch_target_without_label
  "${control_untruthful_object}"
  "${control_untruthful_output}"
  "unsupported RV64 control-flow target without label at .text offset 0x0"
)

execute_process(
  COMMAND "${PYTHON3_EXECUTABLE}" -c [=[
import sys
src, dst, expected_hex, replacement_hex = sys.argv[1:]
data = open(src, "rb").read()
expected = bytes.fromhex(expected_hex)
replacement = bytes.fromhex(replacement_hex)
if data.count(expected) != 1:
    raise SystemExit("expected canonical .text byte string exactly once")
open(dst, "wb").write(data.replace(expected, replacement, 1))
]=] "${rv64_object}" "${unsupported_rv64_object}" "${expected_hex}" "${unsupported_hex}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE patch_rc
  OUTPUT_VARIABLE patch_out
  ERROR_VARIABLE patch_err
)
if(patch_rc MATCHES "timeout")
  message(FATAL_ERROR "[C4C_OBJDUMP_UNSUPPORTED_PATCH_TIMEOUT] exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT patch_rc EQUAL 0)
  message(FATAL_ERROR "[C4C_OBJDUMP_UNSUPPORTED_PATCH_FAIL]\n${patch_out}${patch_err}")
endif()
if(NOT EXISTS "${unsupported_rv64_object}")
  message(FATAL_ERROR "[C4C_OBJDUMP_UNSUPPORTED_OBJECT_MISSING] expected '${unsupported_rv64_object}'")
endif()
run_objdump_failure_case(
  unsupported_instruction
  "${unsupported_rv64_object}"
  "${WORK_DIR}/rv64_unsupported_instruction.s"
  "unsupported RV64 instruction bytes at .text offset 0x0"
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
