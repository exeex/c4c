cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER CLANG SRC OUT_CLANG_BIN OUT_C2LL_BIN)
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

get_filename_component(out_clang_dir "${OUT_CLANG_BIN}" DIRECTORY)
get_filename_component(out_c2ll_dir "${OUT_C2LL_BIN}" DIRECTORY)
file(MAKE_DIRECTORY "${out_clang_dir}")
file(MAKE_DIRECTORY "${out_c2ll_dir}")

find_program(BASH_EXECUTABLE NAMES bash)
if(NOT BASH_EXECUTABLE)
  message(FATAL_ERROR "bash is required for piped test execution")
endif()

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
  COMMAND "${BASH_EXECUTABLE}" "-lc"
          "front_err=$(mktemp); back_err=$(mktemp); \
           \"${COMPILER}\" \"${SRC}\" 2>\"\${front_err}\" | \"${CLANG}\" -x ir - -o \"${OUT_C2LL_BIN}\" 2>\"\${back_err}\"; \
           st_front=\${PIPESTATUS[0]}; st_back=\${PIPESTATUS[1]}; \
           if [ \${st_front} -ne 0 ]; then cat \"\${front_err}\"; rm -f \"\${front_err}\" \"\${back_err}\"; exit 101; fi; \
           if [ \${st_back} -ne 0 ]; then cat \"\${back_err}\"; rm -f \"\${front_err}\" \"\${back_err}\"; exit 102; fi; \
           rm -f \"\${front_err}\" \"\${back_err}\""
  RESULT_VARIABLE pipe_rc
  OUTPUT_VARIABLE pipe_out
  ERROR_VARIABLE pipe_err
)
if(NOT pipe_rc EQUAL 0)
  if(expect_fail)
    message(STATUS "[PASS][XFAIL][COMPILE_PIPE] ${SRC}")
    return()
  endif()
  if(pipe_rc EQUAL 101)
    message(FATAL_ERROR "[FRONTEND_FAIL] ${SRC}\n${pipe_out}${pipe_err}")
  endif()
  if(pipe_rc EQUAL 102)
    message(FATAL_ERROR "[BACKEND_FAIL] ${SRC}\n${pipe_out}${pipe_err}")
  endif()
  message(FATAL_ERROR "[COMPILE_PIPE_FAIL] ${SRC}\n${pipe_out}${pipe_err}")
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
