cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER CLANG SRC OUT_LL OUT_C2LL_BIN EXPECT_EXIT_CODE TARGET_TRIPLE)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${SRC}")
  message(FATAL_ERROR "[FAIL] backend case not found: ${SRC}")
endif()

if(NOT DEFINED CASE_TIMEOUT_SEC OR "${CASE_TIMEOUT_SEC}" STREQUAL "")
  set(CASE_TIMEOUT_SEC 30)
endif()

get_filename_component(out_ll_dir "${OUT_LL}" DIRECTORY)
get_filename_component(out_c2ll_dir "${OUT_C2LL_BIN}" DIRECTORY)
file(MAKE_DIRECTORY "${out_ll_dir}")
file(MAKE_DIRECTORY "${out_c2ll_dir}")

execute_process(
  COMMAND "${COMPILER}" --codegen lir --target "${TARGET_TRIPLE}" "${SRC}" -o "${OUT_LL}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE frontend_rc
  OUTPUT_VARIABLE frontend_out
  ERROR_VARIABLE frontend_err
)
if(frontend_rc MATCHES "timeout")
  message(FATAL_ERROR "[BACKEND_FRONTEND_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT frontend_rc EQUAL 0)
  message(FATAL_ERROR "[BACKEND_FRONTEND_FAIL] ${SRC}\n${frontend_out}${frontend_err}")
endif()

if(NOT EXISTS "${OUT_LL}")
  message(FATAL_ERROR "[BACKEND_OUTPUT_MISSING] ${SRC}\nexpected LLVM text at ${OUT_LL}")
endif()

execute_process(
  COMMAND "${CLANG}" "--target=${TARGET_TRIPLE}" -x ir "${OUT_LL}" -o "${OUT_C2LL_BIN}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE toolchain_rc
  OUTPUT_VARIABLE toolchain_out
  ERROR_VARIABLE toolchain_err
)
if(toolchain_rc MATCHES "timeout")
  message(FATAL_ERROR "[BACKEND_TOOLCHAIN_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT toolchain_rc EQUAL 0)
  message(FATAL_ERROR "[BACKEND_TOOLCHAIN_FAIL] ${SRC}\n${toolchain_out}${toolchain_err}")
endif()

execute_process(
  COMMAND "${OUT_C2LL_BIN}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE run_rc
  OUTPUT_VARIABLE run_out
  ERROR_VARIABLE run_err
)
if(run_rc MATCHES "timeout")
  message(FATAL_ERROR "[BACKEND_RUNTIME_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT run_rc EQUAL "${EXPECT_EXIT_CODE}")
  message(FATAL_ERROR
    "[BACKEND_RUNTIME_UNEXPECTED_RETURN] ${SRC} exit=${run_rc} expected=${EXPECT_EXIT_CODE}\n"
    "${run_out}${run_err}")
endif()

message(STATUS "[PASS][backend] ${SRC}")
