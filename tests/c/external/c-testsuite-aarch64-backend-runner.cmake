cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER CLANG SRC OUT_BIN ROOT OUT_LL TARGET_TRIPLE)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT DEFINED CODEGEN_MODE OR NOT CODEGEN_MODE STREQUAL "backend-aarch64")
  message(FATAL_ERROR "AArch64 backend runner requires -DCODEGEN_MODE=backend-aarch64")
endif()
if(NOT TARGET_TRIPLE STREQUAL "aarch64-unknown-linux-gnu")
  message(FATAL_ERROR "AArch64 backend runner requires -DTARGET_TRIPLE=aarch64-unknown-linux-gnu")
endif()

set(expected_file "${SRC}.expected")
if(EXISTS "${expected_file}")
  file(READ "${expected_file}" expected_output)
else()
  set(expected_output "")
endif()

get_filename_component(out_bin_dir "${OUT_BIN}" DIRECTORY)
file(MAKE_DIRECTORY "${out_bin_dir}")
get_filename_component(out_ll_dir "${OUT_LL}" DIRECTORY)
file(MAKE_DIRECTORY "${out_ll_dir}")

execute_process(
  COMMAND "${COMPILER}" --codegen asm --target "${TARGET_TRIPLE}" "${SRC}" -o "${OUT_LL}"
  WORKING_DIRECTORY "${ROOT}"
  RESULT_VARIABLE front_rc
  OUTPUT_VARIABLE front_out
  ERROR_VARIABLE front_err
)
if(NOT front_rc EQUAL 0)
  message(FATAL_ERROR "[FRONTEND_FAIL] ${SRC}\n${front_out}${front_err}")
endif()
if(NOT EXISTS "${OUT_LL}")
  message(FATAL_ERROR "[BACKEND_OUTPUT_MISSING] ${SRC}\nexpected backend output at ${OUT_LL}")
endif()

file(READ "${OUT_LL}" backend_output LIMIT 256)
if(NOT backend_output MATCHES "(^|\n)[ \t]*\\.text([ \t]*(\n|$)|[ \t]+)")
  message(FATAL_ERROR
    "[BACKEND_FALLBACK_IR] ${SRC}\n"
    "expected backend asm output for ${TARGET_TRIPLE}, but backend output did not start the asm path")
endif()

execute_process(
  COMMAND "${CLANG}" "--target=${TARGET_TRIPLE}" -x assembler "${OUT_LL}" -o "${OUT_BIN}" -lm
  WORKING_DIRECTORY "${ROOT}"
  RESULT_VARIABLE back_rc
  OUTPUT_VARIABLE back_out
  ERROR_VARIABLE back_err
)
if(NOT back_rc EQUAL 0)
  message(FATAL_ERROR "[BACKEND_FAIL] ${SRC}\n${back_out}${back_err}")
endif()

set(c_testsuite_host_processor "${CMAKE_HOST_SYSTEM_PROCESSOR}")
if(c_testsuite_host_processor STREQUAL "")
  execute_process(
    COMMAND uname -m
    RESULT_VARIABLE uname_rc
    OUTPUT_VARIABLE uname_out
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(uname_rc EQUAL 0)
    set(c_testsuite_host_processor "${uname_out}")
  endif()
endif()
string(TOLOWER "${c_testsuite_host_processor}" C_TESTSUITE_HOST_CPU)
set(runtime_command "${OUT_BIN}")
if(NOT (C_TESTSUITE_HOST_CPU STREQUAL "aarch64" OR C_TESTSUITE_HOST_CPU STREQUAL "arm64"))
  if(NOT DEFINED BACKEND_RUNTIME_RUNNER OR "${BACKEND_RUNTIME_RUNNER}" STREQUAL "")
    message(FATAL_ERROR
      "[RUNTIME_UNAVAILABLE] ${SRC}\n"
      "AArch64 backend binary ${OUT_BIN} cannot be run directly on host ${c_testsuite_host_processor}; "
      "set C_TESTSUITE_AARCH64_BACKEND_RUNNER to a runner/emulator command")
  endif()
  separate_arguments(runtime_command UNIX_COMMAND "${BACKEND_RUNTIME_RUNNER}")
  list(APPEND runtime_command "${OUT_BIN}")
endif()

execute_process(
  COMMAND ${runtime_command}
  RESULT_VARIABLE run_rc
  OUTPUT_VARIABLE run_out
  ERROR_VARIABLE run_err
)
set(actual_output "${run_out}${run_err}")
if(NOT run_rc EQUAL 0)
  message(FATAL_ERROR "[RUNTIME_NONZERO] ${SRC} exit=${run_rc}\nstdout+stderr:\n${actual_output}")
endif()

if(NOT actual_output STREQUAL expected_output)
  message(FATAL_ERROR "[RUNTIME_MISMATCH] ${SRC}\nexpected:\n${expected_output}\nactual:\n${actual_output}")
endif()

message(STATUS "[PASS] ${SRC}")
