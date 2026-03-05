cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER EXAMPLE_C OUT_LL)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

get_filename_component(out_ll_dir "${OUT_LL}" DIRECTORY)
file(MAKE_DIRECTORY "${out_ll_dir}")

execute_process(
  COMMAND "${COMPILER}" "${EXAMPLE_C}" -o "${OUT_LL}"
  RESULT_VARIABLE front_rc
  OUTPUT_VARIABLE front_out
  ERROR_VARIABLE front_err
)
if(NOT front_rc EQUAL 0)
  message(FATAL_ERROR "[FAIL] compile example.c\n${front_err}")
endif()

file(READ "${OUT_LL}" ll_text)
string(FIND "${ll_text}" "define i32 @main()" main_pos)
if(main_pos EQUAL -1)
  message(FATAL_ERROR "[FAIL] generated IR missing main definition")
endif()

if(DEFINED CLANG AND NOT "${CLANG}" STREQUAL "")
  if(NOT DEFINED OUT_BIN OR "${OUT_BIN}" STREQUAL "")
    message(FATAL_ERROR "Missing required -DOUT_BIN=... when CLANG is set")
  endif()
  get_filename_component(out_bin_dir "${OUT_BIN}" DIRECTORY)
  file(MAKE_DIRECTORY "${out_bin_dir}")

  execute_process(
    COMMAND "${CLANG}" "${OUT_LL}" -o "${OUT_BIN}"
    RESULT_VARIABLE link_rc
    OUTPUT_VARIABLE link_out
    ERROR_VARIABLE link_err
  )
  if(NOT link_rc EQUAL 0)
    message(FATAL_ERROR "[FAIL] clang failed to build generated IR\n${link_err}")
  endif()

  execute_process(
    COMMAND "${OUT_BIN}"
    RESULT_VARIABLE run_rc
    OUTPUT_VARIABLE run_out
    ERROR_VARIABLE run_err
  )
  if(NOT run_rc EQUAL 31)
    message(FATAL_ERROR "[FAIL] executable returned ${run_rc}, expected 31")
  endif()
endif()

if(DEFINED ENFORCE_NEGATIVE AND ENFORCE_NEGATIVE AND
   DEFINED BAD_SRCS AND NOT "${BAD_SRCS}" STREQUAL "")
  foreach(bad_src IN LISTS BAD_SRCS)
    if(NOT EXISTS "${bad_src}")
      message(FATAL_ERROR "[FAIL] bad case not found: ${bad_src}")
    endif()

    execute_process(
      COMMAND "${COMPILER}" "${bad_src}"
      RESULT_VARIABLE bad_rc
      OUTPUT_VARIABLE bad_out
      ERROR_VARIABLE bad_err
    )
    if(bad_rc EQUAL 0)
      message(FATAL_ERROR "[FAIL] expected compile failure but succeeded: ${bad_src}")
    endif()
  endforeach()
endif()

message(STATUS "[PASS] tiny_c2ll_tests")
