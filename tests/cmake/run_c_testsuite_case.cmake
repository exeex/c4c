cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER CLANG SRC OUT_LL OUT_BIN ROOT)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

set(expected_file "${SRC}.expected")
if(EXISTS "${expected_file}")
  file(READ "${expected_file}" expected_output)
else()
  set(expected_output "")
endif()

get_filename_component(out_ll_dir "${OUT_LL}" DIRECTORY)
get_filename_component(out_bin_dir "${OUT_BIN}" DIRECTORY)
file(MAKE_DIRECTORY "${out_ll_dir}")
file(MAKE_DIRECTORY "${out_bin_dir}")

execute_process(
  COMMAND "${COMPILER}" "${SRC}" -o "${OUT_LL}"
  RESULT_VARIABLE front_rc
  OUTPUT_VARIABLE front_out
  ERROR_VARIABLE front_err
)
if(NOT front_rc EQUAL 0)
  message(FATAL_ERROR "[FRONTEND_FAIL] ${SRC}\n${front_err}")
endif()

execute_process(
  COMMAND "${CLANG}" "${OUT_LL}" -o "${OUT_BIN}"
  RESULT_VARIABLE back_rc
  OUTPUT_VARIABLE back_out
  ERROR_VARIABLE back_err
)
if(NOT back_rc EQUAL 0)
  message(FATAL_ERROR "[BACKEND_FAIL] ${SRC}\n${back_err}")
endif()

execute_process(
  COMMAND "${OUT_BIN}"
  RESULT_VARIABLE run_rc
  OUTPUT_VARIABLE run_out
  ERROR_VARIABLE run_err
)
set(actual_output "${run_out}${run_err}")
if(NOT run_rc EQUAL 0)
  message(FATAL_ERROR "[RUNTIME_NONZERO] ${SRC} exit=${run_rc}\nstdout+stderr:\n${actual_output}")
endif()

if(NOT actual_output STREQUAL expected_output)
  message(FATAL_ERROR "[RUNTIME_MISMATCH] ${SRC}\nexpected:\n${expected_output}\nactual:\n${actual_output}")
endif()

message(STATUS "[PASS] ${SRC}")
