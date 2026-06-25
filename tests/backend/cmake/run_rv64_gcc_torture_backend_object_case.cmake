cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER CLANG QEMU_RISCV64 SRC ROOT OUT_CLANG_BIN OUT_OBJECT OUT_C4C_BIN)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${SRC}")
  message(FATAL_ERROR "[RV64_GCC_TORTURE_CASE_MISSING] ${SRC}")
endif()

if(NOT DEFINED CASE_TIMEOUT_SEC OR "${CASE_TIMEOUT_SEC}" STREQUAL "")
  set(CASE_TIMEOUT_SEC 20)
endif()
if(NOT DEFINED SYSROOT OR "${SYSROOT}" STREQUAL "")
  set(SYSROOT "/usr/riscv64-linux-gnu")
endif()
if(NOT DEFINED TARGET_TRIPLE OR "${TARGET_TRIPLE}" STREQUAL "")
  set(TARGET_TRIPLE "riscv64-linux-gnu")
endif()

foreach(path OUT_CLANG_BIN OUT_OBJECT OUT_C4C_BIN)
  get_filename_component(out_dir "${${path}}" DIRECTORY)
  file(MAKE_DIRECTORY "${out_dir}")
  file(REMOVE "${${path}}")
endforeach()

execute_process(
  COMMAND "${COMPILER}" -I "${ROOT}" --codegen obj --target "${TARGET_TRIPLE}" "${SRC}" -o "${OUT_OBJECT}"
  WORKING_DIRECTORY "${ROOT}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE c4c_compile_rc
  OUTPUT_VARIABLE c4c_compile_out
  ERROR_VARIABLE c4c_compile_err
)
if(c4c_compile_rc MATCHES "timeout")
  message(FATAL_ERROR "[RV64_C4C_OBJ_COMPILE_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT c4c_compile_rc EQUAL 0)
  message(FATAL_ERROR "[RV64_C4C_OBJ_COMPILE_FAIL] ${SRC}\n${c4c_compile_out}${c4c_compile_err}")
endif()
if(NOT EXISTS "${OUT_OBJECT}")
  message(FATAL_ERROR "[RV64_C4C_OBJ_OUTPUT_MISSING] ${SRC}\nexpected=${OUT_OBJECT}")
endif()

file(READ "${OUT_OBJECT}" object_hex HEX)
string(SUBSTRING "${object_hex}" 0 8 elf_magic)
if(NOT elf_magic STREQUAL "7f454c46")
  message(FATAL_ERROR "[RV64_C4C_OBJ_BAD_MAGIC] ${OUT_OBJECT} did not start with ELF magic")
endif()
string(SUBSTRING "${object_hex}" 36 4 machine_le)
if(NOT machine_le STREQUAL "f300")
  message(FATAL_ERROR "[RV64_C4C_OBJ_BAD_MACHINE] ${OUT_OBJECT} machine=${machine_le} expected=f300")
endif()

execute_process(
  COMMAND "${CLANG}" "--target=${TARGET_TRIPLE}" "--gcc-toolchain=/usr"
          -std=gnu89 -w -I "${ROOT}" "${SRC}" -o "${OUT_CLANG_BIN}"
  WORKING_DIRECTORY "${ROOT}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE clang_build_rc
  OUTPUT_VARIABLE clang_build_out
  ERROR_VARIABLE clang_build_err
)
if(clang_build_rc MATCHES "timeout")
  message(FATAL_ERROR "[RV64_CLANG_COMPILE_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT clang_build_rc EQUAL 0)
  message(FATAL_ERROR "[RV64_CLANG_COMPILE_FAIL] ${SRC}\n${clang_build_out}${clang_build_err}")
endif()

execute_process(
  COMMAND "${QEMU_RISCV64}" -L "${SYSROOT}" "${OUT_CLANG_BIN}"
  WORKING_DIRECTORY "${ROOT}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE clang_run_rc
  OUTPUT_VARIABLE clang_run_out
  ERROR_VARIABLE clang_run_err
)
if(clang_run_rc MATCHES "timeout")
  message(FATAL_ERROR "[RV64_CLANG_RUN_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
set(clang_all_out "${clang_run_out}${clang_run_err}")

execute_process(
  COMMAND "${CLANG}" "--target=${TARGET_TRIPLE}" "--gcc-toolchain=/usr" "${OUT_OBJECT}" -o "${OUT_C4C_BIN}"
  WORKING_DIRECTORY "${ROOT}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE c4c_link_rc
  OUTPUT_VARIABLE c4c_link_out
  ERROR_VARIABLE c4c_link_err
)
if(c4c_link_rc MATCHES "timeout")
  message(FATAL_ERROR "[RV64_C4C_LINK_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT c4c_link_rc EQUAL 0)
  message(FATAL_ERROR "[RV64_C4C_LINK_FAIL] ${SRC}\n${c4c_link_out}${c4c_link_err}")
endif()

execute_process(
  COMMAND "${QEMU_RISCV64}" -L "${SYSROOT}" "${OUT_C4C_BIN}"
  WORKING_DIRECTORY "${ROOT}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE c4c_run_rc
  OUTPUT_VARIABLE c4c_run_out
  ERROR_VARIABLE c4c_run_err
)
if(c4c_run_rc MATCHES "timeout")
  message(FATAL_ERROR "[RV64_C4C_RUN_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
set(c4c_all_out "${c4c_run_out}${c4c_run_err}")

if(NOT c4c_run_rc EQUAL clang_run_rc OR NOT c4c_all_out STREQUAL clang_all_out)
  message(FATAL_ERROR
    "[RV64_BACKEND_RUNTIME_MISMATCH] ${SRC}\n"
    "clang_exit=${clang_run_rc} c4c_exit=${c4c_run_rc}\n"
    "clang_out:\n${clang_all_out}\n"
    "c4c_out:\n${c4c_all_out}")
endif()

message(STATUS "[PASS][rv64-gcc-torture-backend-obj] ${SRC}")
