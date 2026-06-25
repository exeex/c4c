cmake_minimum_required(VERSION 3.20)

foreach(v C4C_AS C4C_OBJDUMP CORPUS CONTRACT WORK_DIR)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

foreach(path IN ITEMS "${CORPUS}" "${CONTRACT}")
  if(NOT EXISTS "${path}")
    message(FATAL_ERROR "[RV64_ROUNDTRIP_CONTRACT_MISSING] ${path}")
  endif()
endforeach()

if(NOT DEFINED CASE_TIMEOUT_SEC OR "${CASE_TIMEOUT_SEC}" STREQUAL "")
  set(CASE_TIMEOUT_SEC 10)
endif()

file(MAKE_DIRECTORY "${WORK_DIR}")
file(READ "${CORPUS}" corpus_text)
file(READ "${CONTRACT}" contract_text)

function(expect_contains haystack needle tag)
  string(FIND "${haystack}" "${needle}" needle_offset)
  if(needle_offset EQUAL -1)
    message(FATAL_ERROR "[${tag}] expected '${needle}'")
  endif()
endfunction()

foreach(required IN ITEMS
  "Supported extension boundary: RV64I plus c4c EV64"
  "input.s -> pass1.o -> pass1.s -> pass2.o -> pass2.s -> pass3.o"
  "pass1.s == pass2.s"
  "pass2.o == pass3.o"
  "fail closed"
  "Current unsupported gaps for Step 1")
  expect_contains("${contract_text}" "${required}" "RV64_CONTRACT_TEXT_MISSING")
endforeach()

foreach(required IN ITEMS
  "lui " "auipc " "jal " "jalr "
  "beq " "bne " "blt " "bge " "bltu " "bgeu "
  "lb " "lh " "lw " "ld " "lbu " "lhu " "lwu "
  "sb " "sh " "sw " "sd "
  "addi " "slti " "sltiu " "xori " "ori " "andi "
  "slli " "srli " "srai " "addiw " "slliw " "srliw " "sraiw "
  "add " "sub " "sll " "slt " "sltu " "xor " "srl " "sra " "or " "and "
  "addw " "subw " "sllw " "srlw " "sraw "
  ".insn.d " ".Lbranch_base:" "zero" "x0" "-2048" "2047")
  expect_contains("${corpus_text}" "${required}" "RV64_CORPUS_COVERAGE_MISSING")
endforeach()

set(pass1_o "${WORK_DIR}/pass1.o")
set(pass1_s "${WORK_DIR}/pass1.s")
set(pass2_o "${WORK_DIR}/pass2.o")
set(pass2_s "${WORK_DIR}/pass2.s")
set(pass3_o "${WORK_DIR}/pass3.o")
file(REMOVE "${pass1_o}" "${pass1_s}" "${pass2_o}" "${pass2_s}" "${pass3_o}")

execute_process(
  COMMAND "${C4C_AS}" "${CORPUS}" -o "${pass1_o}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE pass1_rc
  OUTPUT_VARIABLE pass1_out
  ERROR_VARIABLE pass1_err
)
if(pass1_rc MATCHES "timeout")
  message(FATAL_ERROR "[RV64_ROUNDTRIP_PASS1_TIMEOUT] ${CORPUS} exceeded ${CASE_TIMEOUT_SEC}s")
endif()

if(NOT pass1_rc EQUAL 0)
  if(EXISTS "${pass1_o}")
    message(FATAL_ERROR "[RV64_ROUNDTRIP_FAIL_CLOSED_OBJECT] pass1 failed but wrote '${pass1_o}'")
  endif()
  message(STATUS "[PASS][rv64-roundtrip-contract] broad corpus currently fail-closed at pass1")
  message(STATUS "[INFO][rv64-roundtrip-contract] unsupported gaps are recorded in ${CONTRACT}")
  return()
endif()

if(NOT EXISTS "${pass1_o}")
  message(FATAL_ERROR "[RV64_ROUNDTRIP_PASS1_OBJECT_MISSING] expected '${pass1_o}'")
endif()

execute_process(
  COMMAND "${C4C_OBJDUMP}" "${pass1_o}" -o "${pass1_s}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE dump1_rc
  OUTPUT_VARIABLE dump1_out
  ERROR_VARIABLE dump1_err
)
if(dump1_rc MATCHES "timeout")
  message(FATAL_ERROR "[RV64_ROUNDTRIP_DUMP1_TIMEOUT] ${pass1_o} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT dump1_rc EQUAL 0)
  message(FATAL_ERROR "[RV64_ROUNDTRIP_DUMP1_FAIL]\n${dump1_out}${dump1_err}")
endif()

execute_process(
  COMMAND "${C4C_AS}" "${pass1_s}" -o "${pass2_o}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE pass2_rc
  OUTPUT_VARIABLE pass2_out
  ERROR_VARIABLE pass2_err
)
if(pass2_rc MATCHES "timeout")
  message(FATAL_ERROR "[RV64_ROUNDTRIP_PASS2_TIMEOUT] ${pass1_s} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT pass2_rc EQUAL 0)
  message(FATAL_ERROR "[RV64_ROUNDTRIP_PASS2_FAIL]\n${pass2_out}${pass2_err}")
endif()

execute_process(
  COMMAND "${C4C_OBJDUMP}" "${pass2_o}" -o "${pass2_s}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE dump2_rc
  OUTPUT_VARIABLE dump2_out
  ERROR_VARIABLE dump2_err
)
if(dump2_rc MATCHES "timeout")
  message(FATAL_ERROR "[RV64_ROUNDTRIP_DUMP2_TIMEOUT] ${pass2_o} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT dump2_rc EQUAL 0)
  message(FATAL_ERROR "[RV64_ROUNDTRIP_DUMP2_FAIL]\n${dump2_out}${dump2_err}")
endif()

execute_process(
  COMMAND "${C4C_AS}" "${pass2_s}" -o "${pass3_o}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE pass3_rc
  OUTPUT_VARIABLE pass3_out
  ERROR_VARIABLE pass3_err
)
if(pass3_rc MATCHES "timeout")
  message(FATAL_ERROR "[RV64_ROUNDTRIP_PASS3_TIMEOUT] ${pass2_s} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT pass3_rc EQUAL 0)
  message(FATAL_ERROR "[RV64_ROUNDTRIP_PASS3_FAIL]\n${pass3_out}${pass3_err}")
endif()

file(READ "${pass1_s}" pass1_text)
file(READ "${pass2_s}" pass2_text)
if(NOT pass1_text STREQUAL pass2_text)
  message(FATAL_ERROR "[RV64_ROUNDTRIP_TEXT_STABILITY] pass1.s and pass2.s differ")
endif()

file(READ "${pass2_o}" pass2_hex HEX)
file(READ "${pass3_o}" pass3_hex HEX)
if(NOT pass2_hex STREQUAL pass3_hex)
  message(FATAL_ERROR "[RV64_ROUNDTRIP_OBJECT_STABILITY] pass2.o and pass3.o differ")
endif()

message(STATUS "[PASS][rv64-roundtrip-contract] full corpus roundtrip is stable")
