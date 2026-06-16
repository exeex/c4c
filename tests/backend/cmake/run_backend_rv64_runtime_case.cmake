cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER CLANG QEMU_RISCV64 SRC TARGET_TRIPLE OUT_ASM OUT_BIN)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${SRC}")
  message(FATAL_ERROR "[FAIL] backend rv64 runtime case not found: ${SRC}")
endif()

if(NOT DEFINED CASE_TIMEOUT_SEC OR "${CASE_TIMEOUT_SEC}" STREQUAL "")
  set(CASE_TIMEOUT_SEC 30)
endif()
if(NOT DEFINED SYSROOT OR "${SYSROOT}" STREQUAL "")
  set(SYSROOT "/usr/riscv64-linux-gnu")
endif()

get_filename_component(out_asm_dir "${OUT_ASM}" DIRECTORY)
get_filename_component(out_bin_dir "${OUT_BIN}" DIRECTORY)
file(MAKE_DIRECTORY "${out_asm_dir}")
file(MAKE_DIRECTORY "${out_bin_dir}")

execute_process(
  COMMAND "${COMPILER}" --codegen asm --target "${TARGET_TRIPLE}" "${SRC}" -o "${OUT_ASM}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE compiler_rc
  OUTPUT_VARIABLE compiler_out
  ERROR_VARIABLE compiler_err
)
if(compiler_rc MATCHES "timeout")
  message(FATAL_ERROR "[BACKEND_RV64_EMIT_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT compiler_rc EQUAL 0)
  message(FATAL_ERROR "[BACKEND_RV64_EMIT_FAIL] ${SRC}\n${compiler_out}${compiler_err}")
endif()
if(NOT EXISTS "${OUT_ASM}")
  message(FATAL_ERROR "[BACKEND_RV64_OUTPUT_MISSING] expected backend output at ${OUT_ASM}")
endif()

file(READ "${OUT_ASM}" asm_text)
if(asm_text MATCHES "(^|\n)bir\\.func")
  message(FATAL_ERROR
    "[BACKEND_RV64_FALLBACK_BIR] ${OUT_ASM}\nExpected RISC-V assembly, got BIR text\n--- output ---\n${asm_text}")
endif()
if(asm_text MATCHES "(^|\n)define [^\n]*@")
  message(FATAL_ERROR
    "[BACKEND_RV64_FALLBACK_LLVM] ${OUT_ASM}\nExpected RISC-V assembly, got LLVM IR\n--- output ---\n${asm_text}")
endif()

execute_process(
  COMMAND "${CLANG}" "--target=${TARGET_TRIPLE}" "--gcc-toolchain=/usr" "${OUT_ASM}" -o "${OUT_BIN}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE clang_rc
  OUTPUT_VARIABLE clang_out
  ERROR_VARIABLE clang_err
)
if(clang_rc MATCHES "timeout")
  message(FATAL_ERROR "[BACKEND_RV64_CLANG_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT clang_rc EQUAL 0)
  message(FATAL_ERROR "[BACKEND_RV64_CLANG_FAIL] ${SRC}\n${clang_out}${clang_err}")
endif()

execute_process(
  COMMAND "${QEMU_RISCV64}" -L "${SYSROOT}" "${OUT_BIN}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE qemu_rc
  OUTPUT_VARIABLE qemu_out
  ERROR_VARIABLE qemu_err
)
if(qemu_rc MATCHES "timeout")
  message(FATAL_ERROR "[BACKEND_RV64_QEMU_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT qemu_rc EQUAL 0)
  message(FATAL_ERROR
    "[BACKEND_RV64_QEMU_UNEXPECTED_RETURN] ${SRC} exit=${qemu_rc}\n"
    "${qemu_out}${qemu_err}")
endif()

message(STATUS "[PASS][backend-rv64-runtime] ${SRC}")
