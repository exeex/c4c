cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER CLANG QEMU_RISCV64 SRC TARGET_TRIPLE OUT_OBJECT OUT_BIN)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${SRC}")
  message(FATAL_ERROR "[FAIL] backend rv64 object runtime case not found: ${SRC}")
endif()

if(NOT DEFINED CASE_TIMEOUT_SEC OR "${CASE_TIMEOUT_SEC}" STREQUAL "")
  set(CASE_TIMEOUT_SEC 30)
endif()
if(NOT DEFINED SYSROOT OR "${SYSROOT}" STREQUAL "")
  set(SYSROOT "/usr/riscv64-linux-gnu")
endif()
if(NOT DEFINED EXPECTED_RUN_CODE OR "${EXPECTED_RUN_CODE}" STREQUAL "")
  set(EXPECTED_RUN_CODE 0)
endif()

get_filename_component(out_object_dir "${OUT_OBJECT}" DIRECTORY)
get_filename_component(out_bin_dir "${OUT_BIN}" DIRECTORY)
file(MAKE_DIRECTORY "${out_object_dir}")
file(MAKE_DIRECTORY "${out_bin_dir}")
file(REMOVE "${OUT_OBJECT}" "${OUT_BIN}")

execute_process(
  COMMAND "${COMPILER}" --codegen obj --target "${TARGET_TRIPLE}" "${SRC}" -o "${OUT_OBJECT}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE compiler_rc
  OUTPUT_VARIABLE compiler_out
  ERROR_VARIABLE compiler_err
)
if(compiler_rc MATCHES "timeout")
  message(FATAL_ERROR "[BACKEND_RV64_OBJ_EMIT_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT compiler_rc EQUAL 0)
  message(FATAL_ERROR "[BACKEND_RV64_OBJ_EMIT_FAIL] ${SRC}\n${compiler_out}${compiler_err}")
endif()
if(NOT EXISTS "${OUT_OBJECT}")
  message(FATAL_ERROR "[BACKEND_RV64_OBJ_OUTPUT_MISSING] expected object output at ${OUT_OBJECT}")
endif()

file(READ "${OUT_OBJECT}" object_hex HEX)
string(SUBSTRING "${object_hex}" 0 8 elf_magic)
if(NOT elf_magic STREQUAL "7f454c46")
  message(FATAL_ERROR "[BACKEND_RV64_OBJ_BAD_MAGIC] ${OUT_OBJECT} did not start with ELF magic")
endif()
string(SUBSTRING "${object_hex}" 36 4 machine_le)
if(NOT machine_le STREQUAL "f300")
  message(FATAL_ERROR
    "[BACKEND_RV64_OBJ_BAD_MACHINE] ${OUT_OBJECT} machine=${machine_le} expected=f300")
endif()

execute_process(
  COMMAND "${CLANG}" "--target=${TARGET_TRIPLE}" "--gcc-toolchain=/usr" "${OUT_OBJECT}" -o "${OUT_BIN}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE clang_rc
  OUTPUT_VARIABLE clang_out
  ERROR_VARIABLE clang_err
)
if(clang_rc MATCHES "timeout")
  message(FATAL_ERROR "[BACKEND_RV64_OBJ_CLANG_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT clang_rc EQUAL 0)
  message(FATAL_ERROR "[BACKEND_RV64_OBJ_CLANG_FAIL] ${SRC}\n${clang_out}${clang_err}")
endif()

execute_process(
  COMMAND "${QEMU_RISCV64}" -L "${SYSROOT}" "${OUT_BIN}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE qemu_rc
  OUTPUT_VARIABLE qemu_out
  ERROR_VARIABLE qemu_err
)
if(qemu_rc MATCHES "timeout")
  message(FATAL_ERROR "[BACKEND_RV64_OBJ_QEMU_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT qemu_rc EQUAL EXPECTED_RUN_CODE)
  message(FATAL_ERROR
    "[BACKEND_RV64_OBJ_QEMU_UNEXPECTED_RETURN] ${SRC} exit=${qemu_rc} expected=${EXPECTED_RUN_CODE}\n"
    "${qemu_out}${qemu_err}")
endif()

message(STATUS "[PASS][backend-rv64-obj-runtime] ${SRC}")
