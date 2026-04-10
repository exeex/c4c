cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER CLANG CXX_COMPILER SRC ROOT BINARY_DIR)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${SRC}")
  message(FATAL_ERROR "Source not found: ${SRC}")
endif()

if(NOT DEFINED CASE_TIMEOUT_SEC OR "${CASE_TIMEOUT_SEC}" STREQUAL "")
  set(CASE_TIMEOUT_SEC 60)
endif()

find_program(BASH_EXECUTABLE NAMES bash)
if(NOT BASH_EXECUTABLE)
  message(FATAL_ERROR "bash is required for EASTL runtime workflow")
endif()

get_filename_component(CASE_STEM "${SRC}" NAME_WE)
set(WORKFLOW_DIR "${BINARY_DIR}/${CASE_STEM}")
set(OUT_LL "${WORKFLOW_DIR}/${CASE_STEM}.ll")
set(OUT_CLANG_BIN "${WORKFLOW_DIR}/${CASE_STEM}.host.bin")
set(OUT_C4C_BIN "${WORKFLOW_DIR}/${CASE_STEM}.c4c.bin")
set(EASTL_INCLUDE "${ROOT}/ref/EASTL/include")
set(EABASE_INCLUDE "${ROOT}/ref/EABase/include/Common")

file(MAKE_DIRECTORY "${WORKFLOW_DIR}")

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
  COMMAND "${BASH_EXECUTABLE}" "-lc"
          "front_err=$(mktemp); back_err=$(mktemp); \
           \"${COMPILER}\" -I \"${EASTL_INCLUDE}\" -I \"${EABASE_INCLUDE}\" \"${SRC}\" > \"${OUT_LL}\" 2>\"\${front_err}\"; \
           st_front=$?; \
           if [ \${st_front} -ne 0 ]; then cat \"\${front_err}\"; rm -f \"\${front_err}\" \"\${back_err}\"; exit 101; fi; \
           \"${CLANG}\" -x ir \"${OUT_LL}\" -o \"${OUT_C4C_BIN}\" 2>\"\${back_err}\"; \
           st_back=$?; \
           if [ \${st_back} -ne 0 ]; then cat \"\${back_err}\"; rm -f \"\${front_err}\" \"\${back_err}\"; exit 102; fi; \
           rm -f \"\${front_err}\" \"\${back_err}\""
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE pipe_rc
  OUTPUT_VARIABLE pipe_out
  ERROR_VARIABLE pipe_err
)
if(pipe_rc MATCHES "timeout")
  message(FATAL_ERROR "[C4C_PIPE_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(pipe_rc EQUAL 101)
  message(FATAL_ERROR "[C4C_FRONTEND_FAIL] ${SRC}\n${pipe_out}${pipe_err}")
endif()
if(pipe_rc EQUAL 102)
  message(FATAL_ERROR "[C4C_BACKEND_FAIL] ${SRC}\n${pipe_out}${pipe_err}")
endif()
if(NOT pipe_rc EQUAL 0)
  message(FATAL_ERROR "[C4C_PIPE_FAIL] ${SRC}\n${pipe_out}${pipe_err}")
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

message(STATUS "[PASS] EASTL runtime workflow")
message(STATUS "  source: ${SRC}")
message(STATUS "  ir: ${OUT_LL}")
message(STATUS "  host_bin: ${OUT_CLANG_BIN}")
message(STATUS "  c4c_bin: ${OUT_C4C_BIN}")
