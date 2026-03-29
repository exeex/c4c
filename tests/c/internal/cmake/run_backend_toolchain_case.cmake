cmake_minimum_required(VERSION 3.20)

foreach(v CLANG TARGET_TRIPLE BACKEND_OUTPUT_KIND BACKEND_OUTPUT_PATH OUT_ARTIFACT TOOLCHAIN_MODE)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${BACKEND_OUTPUT_PATH}")
  message(FATAL_ERROR
    "[BACKEND_OUTPUT_MISSING] expected backend output at ${BACKEND_OUTPUT_PATH}")
endif()

if(NOT DEFINED CASE_TIMEOUT_SEC OR "${CASE_TIMEOUT_SEC}" STREQUAL "")
  set(CASE_TIMEOUT_SEC 30)
endif()

get_filename_component(out_artifact_dir "${OUT_ARTIFACT}" DIRECTORY)
file(MAKE_DIRECTORY "${out_artifact_dir}")

set(toolchain_command "${CLANG}" "--target=${TARGET_TRIPLE}")
if(TOOLCHAIN_MODE STREQUAL "object")
  list(APPEND toolchain_command -c)
elseif(TOOLCHAIN_MODE STREQUAL "binary")
else()
  message(FATAL_ERROR
    "Unsupported TOOLCHAIN_MODE='${TOOLCHAIN_MODE}' (expected object or binary)")
endif()

if(BACKEND_OUTPUT_KIND STREQUAL "llvm-ir")
  list(APPEND toolchain_command -x ir)
elseif(BACKEND_OUTPUT_KIND STREQUAL "asm")
  list(APPEND toolchain_command -x assembler)
else()
  message(FATAL_ERROR
    "Unsupported BACKEND_OUTPUT_KIND='${BACKEND_OUTPUT_KIND}' (expected llvm-ir or asm)")
endif()

list(APPEND toolchain_command "${BACKEND_OUTPUT_PATH}" -o "${OUT_ARTIFACT}")

execute_process(
  COMMAND ${toolchain_command}
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE toolchain_rc
  OUTPUT_VARIABLE toolchain_out
  ERROR_VARIABLE toolchain_err
)
if(toolchain_rc MATCHES "timeout")
  message(FATAL_ERROR "[BACKEND_TOOLCHAIN_TIMEOUT] ${BACKEND_OUTPUT_PATH} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT toolchain_rc EQUAL 0)
  message(FATAL_ERROR "[BACKEND_TOOLCHAIN_FAIL] ${BACKEND_OUTPUT_PATH}\n${toolchain_out}${toolchain_err}")
endif()

if(NOT EXISTS "${OUT_ARTIFACT}")
  message(FATAL_ERROR "[BACKEND_TOOLCHAIN_OUTPUT_MISSING] expected artifact at ${OUT_ARTIFACT}")
endif()

message(STATUS "[PASS][backend-toolchain] ${BACKEND_OUTPUT_KIND} -> ${TOOLCHAIN_MODE}")
