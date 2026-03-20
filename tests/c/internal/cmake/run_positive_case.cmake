cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER CLANG SRC)
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
if(NOT DEFINED TEST_MODE OR "${TEST_MODE}" STREQUAL "")
  set(TEST_MODE runtime)
endif()
if(NOT TEST_MODE STREQUAL "runtime" AND
   NOT TEST_MODE STREQUAL "frontend" AND
   NOT TEST_MODE STREQUAL "parse")
  message(FATAL_ERROR "Unsupported TEST_MODE='${TEST_MODE}'")
endif()

get_filename_component(out_ll_dir "${OUT_LL}" DIRECTORY)
get_filename_component(out_clang_dir "${OUT_CLANG_BIN}" DIRECTORY)
get_filename_component(out_c2ll_dir "${OUT_C2LL_BIN}" DIRECTORY)
get_filename_component(src_ext "${SRC}" EXT)

set(host_compiler "${CLANG}")
set(host_std_flag "-std=gnu11")
if(src_ext STREQUAL ".cpp" OR src_ext STREQUAL ".cc" OR src_ext STREQUAL ".cxx")
  if(NOT DEFINED CXX_COMPILER OR "${CXX_COMPILER}" STREQUAL "")
    message(FATAL_ERROR "Missing required -DCXX_COMPILER=... for C++ positive case ${SRC}")
  endif()
  set(host_compiler "${CXX_COMPILER}")
  set(host_std_flag "-std=gnu++20")
endif()

if(TEST_MODE STREQUAL "runtime")
  foreach(v OUT_CLANG_BIN OUT_C2LL_BIN)
    if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
      message(FATAL_ERROR "Missing required -D${v}=... for TEST_MODE=${TEST_MODE}")
    endif()
  endforeach()
  file(MAKE_DIRECTORY "${out_clang_dir}")
  file(MAKE_DIRECTORY "${out_c2ll_dir}")

  find_program(BASH_EXECUTABLE NAMES bash)
  if(NOT BASH_EXECUTABLE)
    message(FATAL_ERROR "bash is required for piped test execution")
  endif()

  execute_process(
    COMMAND "${host_compiler}" "${host_std_flag}" -w "${SRC}" -o "${OUT_CLANG_BIN}"
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
endif()

if(TEST_MODE STREQUAL "parse")
  execute_process(
    COMMAND "${COMPILER}" --parse-only "${SRC}"
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
elseif(TEST_MODE STREQUAL "frontend")
  execute_process(
    COMMAND "${COMPILER}" "${SRC}"
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

  message(STATUS "[PASS][frontend] ${SRC}")
else()
  execute_process(
    COMMAND "${BASH_EXECUTABLE}" "-lc"
            "front_err=$(mktemp); back_err=$(mktemp); \
             \"${COMPILER}\" \"${SRC}\" 2>\"\${front_err}\" | \"${CLANG}\" -x ir - -o \"${OUT_C2LL_BIN}\" 2>\"\${back_err}\"; \
             st_front=\${PIPESTATUS[0]}; st_back=\${PIPESTATUS[1]}; \
             if [ \${st_front} -ne 0 ]; then cat \"\${front_err}\"; rm -f \"\${front_err}\" \"\${back_err}\"; exit 101; fi; \
             if [ \${st_back} -ne 0 ]; then cat \"\${back_err}\"; rm -f \"\${front_err}\" \"\${back_err}\"; exit 102; fi; \
             rm -f \"\${front_err}\" \"\${back_err}\""
    TIMEOUT "${CASE_TIMEOUT_SEC}"
    RESULT_VARIABLE pipe_rc
    OUTPUT_VARIABLE pipe_out
    ERROR_VARIABLE pipe_err
  )
  if(pipe_rc MATCHES "timeout")
    message(FATAL_ERROR "[COMPILE_PIPE_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
  endif()
  if(pipe_rc EQUAL 101)
    message(FATAL_ERROR "[FRONTEND_FAIL] ${SRC}\n${pipe_out}${pipe_err}")
  endif()
  if(pipe_rc EQUAL 102)
    message(FATAL_ERROR "[BACKEND_FAIL] ${SRC}\n${pipe_out}${pipe_err}")
  endif()
  if(NOT pipe_rc EQUAL 0)
    message(FATAL_ERROR "[COMPILE_PIPE_FAIL] ${SRC}\n${pipe_out}${pipe_err}")
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
endif()
