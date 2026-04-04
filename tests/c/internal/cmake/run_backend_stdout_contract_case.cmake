cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER CLANG TARGET_TRIPLE BACKEND_OUTPUT_PATH OUT_ARTIFACT OBJDUMP SRC)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${SRC}")
  message(FATAL_ERROR "Missing source file: ${SRC}")
endif()

if(NOT DEFINED CASE_TIMEOUT_SEC OR "${CASE_TIMEOUT_SEC}" STREQUAL "")
  set(CASE_TIMEOUT_SEC 30)
endif()

get_filename_component(backend_output_dir "${BACKEND_OUTPUT_PATH}" DIRECTORY)
get_filename_component(out_artifact_dir "${OUT_ARTIFACT}" DIRECTORY)
file(MAKE_DIRECTORY "${backend_output_dir}")
file(MAKE_DIRECTORY "${out_artifact_dir}")

execute_process(
  COMMAND "${COMPILER}" --codegen asm --target "${TARGET_TRIPLE}" "${SRC}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE compiler_rc
  OUTPUT_VARIABLE compiler_out
  ERROR_VARIABLE compiler_err
)
if(compiler_rc MATCHES "timeout")
  message(FATAL_ERROR "[BACKEND_STDOUT_CONTRACT_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT compiler_rc EQUAL 0)
  message(FATAL_ERROR "[BACKEND_STDOUT_CONTRACT_EMIT_FAIL] ${SRC}\n${compiler_out}${compiler_err}")
endif()

file(WRITE "${BACKEND_OUTPUT_PATH}" "${compiler_out}")

if(NOT EXISTS "${BACKEND_OUTPUT_PATH}")
  message(FATAL_ERROR
    "[BACKEND_STDOUT_CONTRACT_OUTPUT_MISSING] expected backend output at ${BACKEND_OUTPUT_PATH}")
endif()

file(READ "${BACKEND_OUTPUT_PATH}" backend_output)

if(DEFINED REQUIRED_BACKEND_SNIPPETS AND NOT "${REQUIRED_BACKEND_SNIPPETS}" STREQUAL "")
  string(REPLACE "|" ";" required_backend_snippets "${REQUIRED_BACKEND_SNIPPETS}")
  foreach(snippet IN LISTS required_backend_snippets)
    if("${snippet}" STREQUAL "")
      continue()
    endif()
    string(FIND "${backend_output}" "${snippet}" snippet_pos)
    if(snippet_pos EQUAL -1)
      message(FATAL_ERROR
        "[BACKEND_STDOUT_CONTRACT_SNIPPET_MISSING] ${BACKEND_OUTPUT_PATH}\nMissing backend snippet: ${snippet}\n--- backend output ---\n${backend_output}")
    endif()
  endforeach()
endif()

execute_process(
  COMMAND "${CLANG}" "--target=${TARGET_TRIPLE}" -x assembler "${BACKEND_OUTPUT_PATH}" -c
          -o "${OUT_ARTIFACT}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE toolchain_rc
  OUTPUT_VARIABLE toolchain_out
  ERROR_VARIABLE toolchain_err
)
if(toolchain_rc MATCHES "timeout")
  message(FATAL_ERROR
    "[BACKEND_STDOUT_CONTRACT_TOOLCHAIN_TIMEOUT] ${BACKEND_OUTPUT_PATH} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT toolchain_rc EQUAL 0)
  message(FATAL_ERROR
    "[BACKEND_STDOUT_CONTRACT_TOOLCHAIN_FAIL] ${BACKEND_OUTPUT_PATH}\n${toolchain_out}${toolchain_err}")
endif()

execute_process(
  COMMAND "${OBJDUMP}" -h -r -t "${OUT_ARTIFACT}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE objdump_rc
  OUTPUT_VARIABLE objdump_out
  ERROR_VARIABLE objdump_err
)
if(objdump_rc MATCHES "timeout")
  message(FATAL_ERROR
    "[BACKEND_STDOUT_CONTRACT_OBJDUMP_TIMEOUT] ${OUT_ARTIFACT} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT objdump_rc EQUAL 0)
  message(FATAL_ERROR
    "[BACKEND_STDOUT_CONTRACT_OBJDUMP_FAIL] ${OUT_ARTIFACT}\n${objdump_out}${objdump_err}")
endif()

set(objdump_text "${objdump_out}${objdump_err}")

if(DEFINED REQUIRED_OBJDUMP_SNIPPETS AND NOT "${REQUIRED_OBJDUMP_SNIPPETS}" STREQUAL "")
  string(REPLACE "|" ";" required_objdump_snippets "${REQUIRED_OBJDUMP_SNIPPETS}")
  foreach(snippet IN LISTS required_objdump_snippets)
    if("${snippet}" STREQUAL "")
      continue()
    endif()
    string(FIND "${objdump_text}" "${snippet}" snippet_pos)
    if(snippet_pos EQUAL -1)
      message(FATAL_ERROR
        "[BACKEND_STDOUT_CONTRACT_OBJDUMP_SNIPPET_MISSING] ${OUT_ARTIFACT}\nMissing objdump snippet: ${snippet}\n--- objdump ---\n${objdump_text}")
    endif()
  endforeach()
endif()

if(DEFINED FORBIDDEN_OBJDUMP_SNIPPETS AND NOT "${FORBIDDEN_OBJDUMP_SNIPPETS}" STREQUAL "")
  string(REPLACE "|" ";" forbidden_objdump_snippets "${FORBIDDEN_OBJDUMP_SNIPPETS}")
  foreach(snippet IN LISTS forbidden_objdump_snippets)
    if("${snippet}" STREQUAL "")
      continue()
    endif()
    string(FIND "${objdump_text}" "${snippet}" snippet_pos)
    if(NOT snippet_pos EQUAL -1)
      message(FATAL_ERROR
        "[BACKEND_STDOUT_CONTRACT_OBJDUMP_FORBIDDEN_SNIPPET] ${OUT_ARTIFACT}\nUnexpected objdump snippet: ${snippet}\n--- objdump ---\n${objdump_text}")
    endif()
  endforeach()
endif()

message(STATUS "[PASS][backend-stdout-contract] ${BACKEND_OUTPUT_PATH} -> ${OUT_ARTIFACT}")
