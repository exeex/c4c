cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER CLANG SRC OUT_C2LL_BIN EXPECT_EXIT_CODE)
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

get_filename_component(out_c2ll_dir "${OUT_C2LL_BIN}" DIRECTORY)
file(MAKE_DIRECTORY "${out_c2ll_dir}")

find_program(BASH_EXECUTABLE NAMES bash)
if(NOT BASH_EXECUTABLE)
  message(FATAL_ERROR "bash is required for piped backend test execution")
endif()

execute_process(
  COMMAND "${BASH_EXECUTABLE}" "-lc"
          "front_err=$(mktemp); back_err=$(mktemp); \
           \"${COMPILER}\" --codegen lir \"${SRC}\" 2>\"\${front_err}\" | \"${CLANG}\" -x ir - -o \"${OUT_C2LL_BIN}\" 2>\"\${back_err}\"; \
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
  message(FATAL_ERROR "[BACKEND_COMPILE_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(pipe_rc EQUAL 101)
  message(FATAL_ERROR "[BACKEND_FRONTEND_FAIL] ${SRC}\n${pipe_out}${pipe_err}")
endif()
if(pipe_rc EQUAL 102)
  message(FATAL_ERROR "[BACKEND_CLANG_FAIL] ${SRC}\n${pipe_out}${pipe_err}")
endif()
if(NOT pipe_rc EQUAL 0)
  message(FATAL_ERROR "[BACKEND_COMPILE_FAIL] ${SRC}\n${pipe_out}${pipe_err}")
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
