cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER CLANG SRC OUT_BIN)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

get_filename_component(out_bin_dir "${OUT_BIN}" DIRECTORY)
file(MAKE_DIRECTORY "${out_bin_dir}")

find_program(BASH_EXECUTABLE NAMES bash)
if(NOT BASH_EXECUTABLE)
  message(FATAL_ERROR "bash is required for piped test execution")
endif()

execute_process(
  COMMAND "${BASH_EXECUTABLE}" "-lc"
          "front_err=$(mktemp); back_err=$(mktemp); \
           \"${COMPILER}\" \"${SRC}\" 2>\"\${front_err}\" | \"${CLANG}\" -x ir - -o \"${OUT_BIN}\" 2>\"\${back_err}\"; \
           st_front=\${PIPESTATUS[0]}; st_back=\${PIPESTATUS[1]}; \
           if [ \${st_front} -ne 0 ]; then cat \"\${front_err}\"; rm -f \"\${front_err}\" \"\${back_err}\"; exit 101; fi; \
           if [ \${st_back} -ne 0 ]; then cat \"\${back_err}\"; rm -f \"\${front_err}\" \"\${back_err}\"; exit 102; fi; \
           rm -f \"\${front_err}\" \"\${back_err}\""
  RESULT_VARIABLE pipe_rc
  OUTPUT_VARIABLE pipe_out
  ERROR_VARIABLE pipe_err
)
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
  COMMAND "${OUT_BIN}"
  RESULT_VARIABLE run_rc
  OUTPUT_VARIABLE run_out
  ERROR_VARIABLE run_err
)
if(NOT run_rc EQUAL 0)
  message(FATAL_ERROR
    "[RUNTIME_FAIL] ${SRC}\n"
    "exit=${run_rc}\n"
    "stdout:\n${run_out}\n"
    "stderr:\n${run_err}")
endif()

message(STATUS "[PASS] ${SRC}")
