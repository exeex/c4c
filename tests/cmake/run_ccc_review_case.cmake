cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER CLANG SRC OUT_LL OUT_CLANG_BIN OUT_C2LL_BIN)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

file(READ "${SRC}" src_text)
set(expect_fail OFF)
string(REGEX MATCH "//[ \t]*CCC_EXPECT:[ \t]*([A-Za-z_]+)" _m "${src_text}")
if(CMAKE_MATCH_1)
  string(TOLOWER "${CMAKE_MATCH_1}" _expect)
  if(_expect STREQUAL "fail")
    set(expect_fail ON)
  endif()
endif()

get_filename_component(out_ll_dir "${OUT_LL}" DIRECTORY)
get_filename_component(out_clang_dir "${OUT_CLANG_BIN}" DIRECTORY)
get_filename_component(out_c2ll_dir "${OUT_C2LL_BIN}" DIRECTORY)
file(MAKE_DIRECTORY "${out_ll_dir}")
file(MAKE_DIRECTORY "${out_clang_dir}")
file(MAKE_DIRECTORY "${out_c2ll_dir}")

execute_process(
  COMMAND "${CLANG}" -std=gnu11 -fdollars-in-identifiers "${SRC}" -o "${OUT_CLANG_BIN}"
  RESULT_VARIABLE clang_build_rc
  OUTPUT_VARIABLE clang_build_out
  ERROR_VARIABLE clang_build_err
)
if(NOT clang_build_rc EQUAL 0)
  message(FATAL_ERROR "[CLANG_COMPILE_FAIL] ${SRC}\n${clang_build_err}")
endif()

execute_process(
  COMMAND "${OUT_CLANG_BIN}"
  RESULT_VARIABLE clang_run_rc
  OUTPUT_VARIABLE clang_run_out
  ERROR_VARIABLE clang_run_err
)
set(clang_all_out "${clang_run_out}${clang_run_err}")
if(NOT clang_run_rc EQUAL 0)
  message(FATAL_ERROR "[CLANG_RUNTIME_FAIL] ${SRC} exit=${clang_run_rc}\n${clang_all_out}")
endif()

execute_process(
  COMMAND "${COMPILER}" "${SRC}" -o "${OUT_LL}"
  RESULT_VARIABLE front_rc
  OUTPUT_VARIABLE front_out
  ERROR_VARIABLE front_err
)
if(NOT front_rc EQUAL 0)
  if(expect_fail)
    message(STATUS "[PASS][XFAIL][FRONTEND] ${SRC}")
    return()
  endif()
  message(FATAL_ERROR "[FRONTEND_FAIL] ${SRC}\n${front_err}")
endif()

execute_process(
  COMMAND "${CLANG}" "${OUT_LL}" -o "${OUT_C2LL_BIN}"
  RESULT_VARIABLE back_rc
  OUTPUT_VARIABLE back_out
  ERROR_VARIABLE back_err
)
if(NOT back_rc EQUAL 0)
  if(expect_fail)
    message(STATUS "[PASS][XFAIL][BACKEND] ${SRC}")
    return()
  endif()
  message(FATAL_ERROR "[BACKEND_FAIL] ${SRC}\n${back_err}")
endif()

execute_process(
  COMMAND "${OUT_C2LL_BIN}"
  RESULT_VARIABLE c2ll_run_rc
  OUTPUT_VARIABLE c2ll_run_out
  ERROR_VARIABLE c2ll_run_err
)
set(c2ll_all_out "${c2ll_run_out}${c2ll_run_err}")

set(matched OFF)
if(c2ll_run_rc EQUAL clang_run_rc AND c2ll_all_out STREQUAL clang_all_out)
  set(matched ON)
endif()

if(expect_fail)
  if(matched)
    message(FATAL_ERROR "[XPASS] ${SRC} expected fail but matched clang")
  endif()
  message(STATUS "[PASS][XFAIL][RUNTIME] ${SRC}")
  return()
endif()

if(NOT matched)
  message(FATAL_ERROR
    "[RUNTIME_FAIL] ${SRC}\n"
    "clang_exit=${clang_run_rc} c2ll_exit=${c2ll_run_rc}\n"
    "clang_out:\n${clang_all_out}\n"
    "c2ll_out:\n${c2ll_all_out}")
endif()

message(STATUS "[PASS] ${SRC}")
