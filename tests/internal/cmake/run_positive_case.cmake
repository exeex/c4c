cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER CLANG SRC OUT_LL OUT_CLANG_BIN OUT_C2LL_BIN)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${SRC}")
  message(FATAL_ERROR "[FAIL] positive case not found: ${SRC}")
endif()

if(NOT DEFINED CASE_TIMEOUT_SEC OR "${CASE_TIMEOUT_SEC}" STREQUAL "")
  set(CASE_TIMEOUT_SEC 30)
endif()

get_filename_component(out_ll_dir "${OUT_LL}" DIRECTORY)
get_filename_component(out_clang_dir "${OUT_CLANG_BIN}" DIRECTORY)
get_filename_component(out_c2ll_dir "${OUT_C2LL_BIN}" DIRECTORY)
file(MAKE_DIRECTORY "${out_ll_dir}")
file(MAKE_DIRECTORY "${out_clang_dir}")
file(MAKE_DIRECTORY "${out_c2ll_dir}")

execute_process(
  COMMAND "${CLANG}" -std=gnu11 -w "${SRC}" -o "${OUT_CLANG_BIN}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE clang_build_rc
  OUTPUT_VARIABLE clang_build_out
  ERROR_VARIABLE clang_build_err
)
if(clang_build_rc MATCHES "timeout")
  message(FATAL_ERROR "[CLANG_COMPILE_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT clang_build_rc EQUAL 0)
  message(FATAL_ERROR "[CLANG_COMPILE_FAIL] ${SRC}\n${clang_build_err}")
endif()

execute_process(
  COMMAND "${OUT_CLANG_BIN}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE clang_run_rc
  OUTPUT_VARIABLE clang_run_out
  ERROR_VARIABLE clang_run_err
)
if(clang_run_rc MATCHES "timeout")
  message(FATAL_ERROR "[CLANG_RUNTIME_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT clang_run_rc EQUAL 0)
  message(FATAL_ERROR
    "[CLANG_RUNTIME_UNEXPECTED_RETURN] ${SRC} exit=${clang_run_rc}\n"
    "${clang_run_out}${clang_run_err}")
endif()

execute_process(
  COMMAND "${COMPILER}" "${SRC}" -o "${OUT_LL}"
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

execute_process(
  COMMAND "${CLANG}" "${OUT_LL}" -o "${OUT_C2LL_BIN}"
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
  COMMAND "${OUT_C2LL_BIN}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE c2ll_run_rc
  OUTPUT_VARIABLE c2ll_run_out
  ERROR_VARIABLE c2ll_run_err
)
if(c2ll_run_rc MATCHES "timeout")
  message(FATAL_ERROR "[C2LL_RUNTIME_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT c2ll_run_rc EQUAL 0)
  message(FATAL_ERROR
    "[C2LL_RUNTIME_UNEXPECTED_RETURN] ${SRC} exit=${c2ll_run_rc}\n"
    "${c2ll_run_out}${c2ll_run_err}")
endif()

message(STATUS "[PASS] ${SRC}")
