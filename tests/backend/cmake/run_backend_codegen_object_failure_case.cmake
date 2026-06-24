cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER SRC TARGET_TRIPLE EXPECTED_DIAGNOSTICS)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${SRC}")
  message(FATAL_ERROR "[FAIL] backend object route failure case not found: ${SRC}")
endif()

if(NOT DEFINED CASE_TIMEOUT_SEC OR "${CASE_TIMEOUT_SEC}" STREQUAL "")
  set(CASE_TIMEOUT_SEC 30)
endif()

set(output_args)
if(DEFINED OUT_OBJECT AND NOT "${OUT_OBJECT}" STREQUAL "")
  get_filename_component(out_dir "${OUT_OBJECT}" DIRECTORY)
  file(MAKE_DIRECTORY "${out_dir}")
  file(REMOVE "${OUT_OBJECT}")
  set(output_args -o "${OUT_OBJECT}")
endif()

execute_process(
  COMMAND "${COMPILER}" --codegen obj --target "${TARGET_TRIPLE}" "${SRC}" ${output_args}
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE compiler_rc
  OUTPUT_VARIABLE compiler_out
  ERROR_VARIABLE compiler_err
)
if(compiler_rc MATCHES "timeout")
  message(FATAL_ERROR "[BACKEND_OBJ_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(compiler_rc EQUAL 0)
  message(FATAL_ERROR
    "[BACKEND_OBJ_EXPECTED_FAIL] ${SRC} unexpectedly succeeded\n${compiler_out}${compiler_err}")
endif()
if(DEFINED OUT_OBJECT AND NOT "${OUT_OBJECT}" STREQUAL "" AND EXISTS "${OUT_OBJECT}")
  message(FATAL_ERROR "[BACKEND_OBJ_UNEXPECTED_OUTPUT] failed object route wrote ${OUT_OBJECT}")
endif()

set(combined_output "${compiler_out}${compiler_err}")
string(REPLACE "|" ";" expected_diagnostics "${EXPECTED_DIAGNOSTICS}")
foreach(snippet IN LISTS expected_diagnostics)
  if("${snippet}" STREQUAL "")
    continue()
  endif()
  string(FIND "${combined_output}" "${snippet}" snippet_pos)
  if(snippet_pos EQUAL -1)
    message(FATAL_ERROR
      "[BACKEND_OBJ_DIAGNOSTIC_MISSING] ${SRC}\nMissing snippet: ${snippet}\n--- output ---\n${combined_output}")
  endif()
endforeach()

message(STATUS "[PASS][backend-obj-fail] ${SRC}")
