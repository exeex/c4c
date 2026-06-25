cmake_minimum_required(VERSION 3.20)

foreach(v C4CLL C4C_AS C4C_OBJDUMP ROOT C_TESTSUITE_ROOT ALLOWLIST WORK_DIR)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

foreach(path IN ITEMS "${C4CLL}" "${C4C_AS}" "${C4C_OBJDUMP}" "${ROOT}" "${C_TESTSUITE_ROOT}" "${ALLOWLIST}")
  if(NOT EXISTS "${path}")
    message(FATAL_ERROR "[RV64_C_TESTSUITE_SCAN_MISSING] ${path}")
  endif()
endforeach()

if(NOT DEFINED CASE_TIMEOUT_SEC OR "${CASE_TIMEOUT_SEC}" STREQUAL "")
  set(CASE_TIMEOUT_SEC 10)
endif()
if(NOT DEFINED TARGET_TRIPLE OR "${TARGET_TRIPLE}" STREQUAL "")
  set(TARGET_TRIPLE riscv64-linux-gnu)
endif()

function(run_scan_stage case_rel stage_name)
  execute_process(
    COMMAND ${ARGN}
    TIMEOUT "${CASE_TIMEOUT_SEC}"
    RESULT_VARIABLE stage_rc
    OUTPUT_VARIABLE stage_out
    ERROR_VARIABLE stage_err
  )
  if(stage_rc MATCHES "timeout")
    message(FATAL_ERROR
      "[RV64_C_TESTSUITE_STAGE_TIMEOUT] case=${case_rel} stage=${stage_name} exceeded ${CASE_TIMEOUT_SEC}s")
  endif()
  if(NOT stage_rc EQUAL 0)
    message(FATAL_ERROR
      "[RV64_C_TESTSUITE_STAGE_FAIL] case=${case_rel} stage=${stage_name}\n${stage_out}${stage_err}")
  endif()
endfunction()

function(require_output case_rel stage_name output_path)
  if(NOT EXISTS "${output_path}")
    message(FATAL_ERROR
      "[RV64_C_TESTSUITE_STAGE_OUTPUT_MISSING] case=${case_rel} stage=${stage_name} expected=${output_path}")
  endif()
endfunction()

file(MAKE_DIRECTORY "${WORK_DIR}")
file(READ "${ALLOWLIST}" allowlist_text)
string(REPLACE "\r\n" "\n" allowlist_text "${allowlist_text}")
string(REPLACE "\r" "\n" allowlist_text "${allowlist_text}")
string(REPLACE ";" "\\;" allowlist_text "${allowlist_text}")
string(REPLACE "\n" ";" allowlist_lines "${allowlist_text}")

set(case_count 0)
foreach(raw_line IN LISTS allowlist_lines)
  string(REGEX REPLACE "#.*$" "" case_rel "${raw_line}")
  string(STRIP "${case_rel}" case_rel)
  if(case_rel STREQUAL "")
    continue()
  endif()

  math(EXPR case_count "${case_count} + 1")
  set(src "${C_TESTSUITE_ROOT}/${case_rel}")
  if(NOT EXISTS "${src}")
    message(FATAL_ERROR "[RV64_C_TESTSUITE_CASE_MISSING] case=${case_rel} path=${src}")
  endif()

  string(REGEX REPLACE "[^A-Za-z0-9_.-]" "_" case_id "${case_rel}")
  set(case_work "${WORK_DIR}/${case_id}")
  file(REMOVE_RECURSE "${case_work}")
  file(MAKE_DIRECTORY "${case_work}")

  set(pass0_s "${case_work}/case.pass0.s")
  set(pass1_o "${case_work}/case.pass1.o")
  set(pass1_s "${case_work}/case.pass1.s")
  set(pass2_o "${case_work}/case.pass2.o")
  set(pass2_s "${case_work}/case.pass2.s")
  set(pass3_o "${case_work}/case.pass3.o")

  message(STATUS "[RUN][rv64-c-testsuite-roundtrip] case=${case_rel} stage=c4cll-asm")
  run_scan_stage("${case_rel}" "c4cll-asm"
    "${C4CLL}" --codegen asm --target "${TARGET_TRIPLE}" "${src}" -o "${pass0_s}")
  require_output("${case_rel}" "c4cll-asm" "${pass0_s}")

  file(READ "${pass0_s}" pass0_text)
  if(pass0_text MATCHES "(^|\n)bir\\.func")
    message(FATAL_ERROR
      "[RV64_C_TESTSUITE_UNSUPPORTED_ASM] case=${case_rel} stage=c4cll-asm fallback=bir.func output=${pass0_s}")
  endif()
  if(pass0_text MATCHES "(^|\n)define [^\n]*@")
    message(FATAL_ERROR
      "[RV64_C_TESTSUITE_UNSUPPORTED_ASM] case=${case_rel} stage=c4cll-asm fallback=llvm-ir output=${pass0_s}")
  endif()

  message(STATUS "[RUN][rv64-c-testsuite-roundtrip] case=${case_rel} stage=c4c-as-pass1")
  run_scan_stage("${case_rel}" "c4c-as-pass1"
    "${C4C_AS}" "${pass0_s}" -o "${pass1_o}")
  require_output("${case_rel}" "c4c-as-pass1" "${pass1_o}")

  message(STATUS "[RUN][rv64-c-testsuite-roundtrip] case=${case_rel} stage=objdump-pass1")
  run_scan_stage("${case_rel}" "objdump-pass1"
    "${C4C_OBJDUMP}" "${pass1_o}" -o "${pass1_s}")
  require_output("${case_rel}" "objdump-pass1" "${pass1_s}")

  message(STATUS "[RUN][rv64-c-testsuite-roundtrip] case=${case_rel} stage=c4c-as-pass2")
  run_scan_stage("${case_rel}" "c4c-as-pass2"
    "${C4C_AS}" "${pass1_s}" -o "${pass2_o}")
  require_output("${case_rel}" "c4c-as-pass2" "${pass2_o}")

  message(STATUS "[RUN][rv64-c-testsuite-roundtrip] case=${case_rel} stage=objdump-pass2")
  run_scan_stage("${case_rel}" "objdump-pass2"
    "${C4C_OBJDUMP}" "${pass2_o}" -o "${pass2_s}")
  require_output("${case_rel}" "objdump-pass2" "${pass2_s}")

  message(STATUS "[RUN][rv64-c-testsuite-roundtrip] case=${case_rel} stage=c4c-as-pass3")
  run_scan_stage("${case_rel}" "c4c-as-pass3"
    "${C4C_AS}" "${pass2_s}" -o "${pass3_o}")
  require_output("${case_rel}" "c4c-as-pass3" "${pass3_o}")

  file(READ "${pass1_s}" pass1_text)
  file(READ "${pass2_s}" pass2_text)
  if(NOT pass1_text STREQUAL pass2_text)
    message(FATAL_ERROR
      "[RV64_C_TESTSUITE_TEXT_STABILITY] case=${case_rel} stage=comparison comparison=pass1.s == pass2.s left=${pass1_s} right=${pass2_s}")
  endif()

  file(READ "${pass2_o}" pass2_hex HEX)
  file(READ "${pass3_o}" pass3_hex HEX)
  if(NOT pass2_hex STREQUAL pass3_hex)
    message(FATAL_ERROR
      "[RV64_C_TESTSUITE_OBJECT_STABILITY] case=${case_rel} stage=comparison comparison=pass2.o == pass3.o left=${pass2_o} right=${pass3_o}")
  endif()

  message(STATUS "[PASS][rv64-c-testsuite-roundtrip] case=${case_rel}")
endforeach()

if(case_count EQUAL 0)
  message(FATAL_ERROR "[RV64_C_TESTSUITE_ALLOWLIST_EMPTY] ${ALLOWLIST}")
endif()

message(STATUS "[PASS][rv64-c-testsuite-roundtrip-scan] cases=${case_count}")
