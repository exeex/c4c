cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER SRC OUT_LL OUT_C2LL_BIN TARGET_TRIPLE)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

execute_process(
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=${COMPILER}
          -DCLANG=${CMAKE_BINARY_DIR}/missing-clang-for-backend-test
          -DSRC=${SRC}
          -DTARGET_TRIPLE=${TARGET_TRIPLE}
          -DOUT_LL=${OUT_LL}
          -DEXPECT_EXIT_CODE=0
          -DOUT_C2LL_BIN=${OUT_C2LL_BIN}
          -P "${CMAKE_CURRENT_LIST_DIR}/run_backend_positive_case.cmake"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
  TIMEOUT 30
)

if(rc EQUAL 0)
  message(FATAL_ERROR "[BACKEND_TOOLCHAIN_DIAG] expected missing-toolchain failure")
endif()

set(combined "${out}${err}")
if(NOT combined MATCHES "\\[BACKEND_TOOLCHAIN_FAIL\\]")
  message(FATAL_ERROR "[BACKEND_TOOLCHAIN_DIAG] missing failure category\n${combined}")
endif()

message(STATUS "[PASS][backend] missing toolchain failure attributed")
