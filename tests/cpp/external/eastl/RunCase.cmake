cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER CLANG CXX_COMPILER ROOT SRC TEST_MODE OUT_LL OUT_CLANG_BIN OUT_C4C_BIN)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${SRC}")
  message(FATAL_ERROR "[FAIL] EASTL external case not found: ${SRC}")
endif()

if(NOT DEFINED CASE_TIMEOUT_SEC OR "${CASE_TIMEOUT_SEC}" STREQUAL "")
  set(CASE_TIMEOUT_SEC 60)
endif()

if(NOT TEST_MODE STREQUAL "runtime" AND
   NOT TEST_MODE STREQUAL "frontend" AND
   NOT TEST_MODE STREQUAL "parse")
  message(FATAL_ERROR "Unsupported TEST_MODE='${TEST_MODE}'")
endif()

get_filename_component(out_ll_dir "${OUT_LL}" DIRECTORY)
get_filename_component(out_clang_dir "${OUT_CLANG_BIN}" DIRECTORY)
get_filename_component(out_c4c_dir "${OUT_C4C_BIN}" DIRECTORY)
file(MAKE_DIRECTORY "${out_ll_dir}")
file(MAKE_DIRECTORY "${out_clang_dir}")
file(MAKE_DIRECTORY "${out_c4c_dir}")

set(EASTL_INCLUDE "${ROOT}/ref/EASTL/include")
set(EABASE_INCLUDE "${ROOT}/ref/EABase/include/Common")

if(TEST_MODE STREQUAL "parse")
  execute_process(
    COMMAND "${COMPILER}" --parse-only
            -I "${EASTL_INCLUDE}"
            -I "${EABASE_INCLUDE}"
            "${SRC}"
    TIMEOUT "${CASE_TIMEOUT_SEC}"
    RESULT_VARIABLE parse_rc
    OUTPUT_VARIABLE parse_out
    ERROR_VARIABLE parse_err
  )
  if(parse_rc MATCHES "timeout")
    message(FATAL_ERROR "[PARSE_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
  endif()
  if(NOT parse_rc EQUAL 0)
    message(FATAL_ERROR "[PARSE_FAIL] ${SRC}\n${parse_err}")
  endif()

  message(STATUS "[PASS][parse] ${SRC}")
  return()
endif()

execute_process(
  COMMAND "${COMPILER}"
          -I "${EASTL_INCLUDE}"
          -I "${EABASE_INCLUDE}"
          "${SRC}"
          -o "${OUT_LL}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE front_rc
  OUTPUT_VARIABLE front_out
  ERROR_VARIABLE front_err
)
if(front_rc MATCHES "timeout")
  message(FATAL_ERROR "[FRONTEND_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT front_rc EQUAL 0)
  message(FATAL_ERROR "[FRONTEND_FAIL] ${SRC}\n${front_err}")
endif()

if(TEST_MODE STREQUAL "frontend")
  message(STATUS "[PASS][frontend] ${SRC}")
  return()
endif()

execute_process(
  COMMAND "${CXX_COMPILER}"
          "-std=gnu++20"
          "-I" "${EASTL_INCLUDE}"
          "-I" "${EABASE_INCLUDE}"
          "-w"
          "${SRC}"
          "-o" "${OUT_CLANG_BIN}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE host_build_rc
  OUTPUT_VARIABLE host_build_out
  ERROR_VARIABLE host_build_err
)
if(host_build_rc MATCHES "timeout")
  message(FATAL_ERROR "[HOST_COMPILE_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT host_build_rc EQUAL 0)
  message(FATAL_ERROR "[HOST_COMPILE_FAIL] ${SRC}\n${host_build_err}")
endif()

execute_process(
  COMMAND "${OUT_CLANG_BIN}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE host_run_rc
  OUTPUT_VARIABLE host_run_out
  ERROR_VARIABLE host_run_err
)
if(host_run_rc MATCHES "timeout")
  message(FATAL_ERROR "[HOST_RUNTIME_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT host_run_rc EQUAL 0)
  message(FATAL_ERROR
    "[HOST_RUNTIME_UNEXPECTED_RETURN] ${SRC} exit=${host_run_rc}\n"
    "${host_run_out}${host_run_err}")
endif()

execute_process(
  COMMAND "${CLANG}" -x ir "${OUT_LL}" -o "${OUT_C4C_BIN}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE back_rc
  OUTPUT_VARIABLE back_out
  ERROR_VARIABLE back_err
)
if(back_rc MATCHES "timeout")
  message(FATAL_ERROR "[BACKEND_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT back_rc EQUAL 0)
  message(FATAL_ERROR "[BACKEND_FAIL] ${SRC}\n${back_err}")
endif()

execute_process(
  COMMAND "${OUT_C4C_BIN}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE c4c_run_rc
  OUTPUT_VARIABLE c4c_run_out
  ERROR_VARIABLE c4c_run_err
)
if(c4c_run_rc MATCHES "timeout")
  message(FATAL_ERROR "[C4C_RUNTIME_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT c4c_run_rc EQUAL 0)
  message(FATAL_ERROR
    "[C4C_RUNTIME_UNEXPECTED_RETURN] ${SRC} exit=${c4c_run_rc}\n"
    "${c4c_run_out}${c4c_run_err}")
endif()

message(STATUS "[PASS][runtime] ${SRC}")
