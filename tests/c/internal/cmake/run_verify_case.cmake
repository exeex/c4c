cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER PYTHON3_EXECUTABLE RUNNER SRC OUT_LL)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${SRC}")
  message(FATAL_ERROR "[FAIL] verify case not found: ${SRC}")
endif()

execute_process(
  COMMAND "${PYTHON3_EXECUTABLE}" "${RUNNER}"
          --compiler "${COMPILER}"
          --src "${SRC}"
          --out-ll "${OUT_LL}"
          --expect-mode verify
  RESULT_VARIABLE runner_rc
  OUTPUT_VARIABLE runner_out
  ERROR_VARIABLE runner_err
)

if(NOT runner_rc EQUAL 0)
  message(FATAL_ERROR "[VERIFY_FAIL] ${SRC}\n${runner_out}${runner_err}")
endif()

message(STATUS "[PASS] ${SRC} (verify)")
