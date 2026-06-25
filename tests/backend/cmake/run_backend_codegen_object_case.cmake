cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER SRC TARGET_TRIPLE OUT_OBJECT EXPECTED_MACHINE_LE)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${SRC}")
  message(FATAL_ERROR "[FAIL] backend object route case not found: ${SRC}")
endif()

if(NOT DEFINED CASE_TIMEOUT_SEC OR "${CASE_TIMEOUT_SEC}" STREQUAL "")
  set(CASE_TIMEOUT_SEC 30)
endif()

get_filename_component(out_dir "${OUT_OBJECT}" DIRECTORY)
file(MAKE_DIRECTORY "${out_dir}")
file(REMOVE "${OUT_OBJECT}")

execute_process(
  COMMAND "${COMPILER}" --codegen obj --target "${TARGET_TRIPLE}" "${SRC}" -o "${OUT_OBJECT}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE compiler_rc
  OUTPUT_VARIABLE compiler_out
  ERROR_VARIABLE compiler_err
)
if(compiler_rc MATCHES "timeout")
  message(FATAL_ERROR "[BACKEND_OBJ_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT compiler_rc EQUAL 0)
  message(FATAL_ERROR "[BACKEND_OBJ_EMIT_FAIL] ${SRC}\n${compiler_out}${compiler_err}")
endif()
if(NOT EXISTS "${OUT_OBJECT}")
  message(FATAL_ERROR "[BACKEND_OBJ_OUTPUT_MISSING] expected object output at ${OUT_OBJECT}")
endif()

file(READ "${OUT_OBJECT}" output_hex HEX)
string(SUBSTRING "${output_hex}" 0 8 elf_magic)
if(NOT elf_magic STREQUAL "7f454c46")
  message(FATAL_ERROR "[BACKEND_OBJ_BAD_MAGIC] ${OUT_OBJECT} did not start with ELF magic")
endif()

string(SUBSTRING "${output_hex}" 36 4 machine_le)
if(NOT machine_le STREQUAL "${EXPECTED_MACHINE_LE}")
  message(FATAL_ERROR
    "[BACKEND_OBJ_BAD_MACHINE] ${OUT_OBJECT} machine=${machine_le} expected=${EXPECTED_MACHINE_LE}")
endif()

if(DEFINED EXPECTED_HEX_CONTAINS AND NOT "${EXPECTED_HEX_CONTAINS}" STREQUAL "")
  string(FIND "${output_hex}" "${EXPECTED_HEX_CONTAINS}" expected_hex_offset)
  if(expected_hex_offset EQUAL -1)
    message(FATAL_ERROR
      "[BACKEND_OBJ_MISSING_BYTES] ${OUT_OBJECT} did not contain ${EXPECTED_HEX_CONTAINS}")
  endif()
endif()

message(STATUS "[PASS][backend-obj] ${SRC}")
