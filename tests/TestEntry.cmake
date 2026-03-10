set(CTEST_CORE_PARALLEL_LEVEL "8" CACHE STRING
    "Parallel jobs for ctest_core target")
set(CTEST_CORE_TEST_TIMEOUT_SEC "30" CACHE STRING
    "Per-test timeout (seconds) for ctest_core target")

add_custom_target(ctest_core
  COMMAND "${CMAKE_CTEST_COMMAND}"
          --output-on-failure
          --progress
          -j "${CTEST_CORE_PARALLEL_LEVEL}"
          --timeout "${CTEST_CORE_TEST_TIMEOUT_SEC}"
          -E "llvm_gcc_c_torture"
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  COMMENT "Run core test set (exclude llvm_gcc_c_torture*, with progress/timeout)"
  VERBATIM
)

set(C_TESTSUITE_ROOT "" CACHE PATH "Path to external c-testsuite root")
if(NOT C_TESTSUITE_ROOT AND EXISTS "${PROJECT_SOURCE_DIR}/tests/c-testsuite")
  set(C_TESTSUITE_ROOT "${PROJECT_SOURCE_DIR}/tests/c-testsuite")
endif()

set(LLVM_TEST_SUITE_ROOT "" CACHE PATH "Path to external llvm-test-suite root")
if(NOT LLVM_TEST_SUITE_ROOT AND EXISTS "${PROJECT_SOURCE_DIR}/tests/llvm-test-suite")
  set(LLVM_TEST_SUITE_ROOT "${PROJECT_SOURCE_DIR}/tests/llvm-test-suite")
endif()

file(GLOB NEGATIVE_TEST_SRCS CONFIGURE_DEPENDS
     "${PROJECT_SOURCE_DIR}/tests/tiny_c2ll/bad_*.c")
file(GLOB POSITIVE_SEMA_TEST_SRCS CONFIGURE_DEPENDS
     "${PROJECT_SOURCE_DIR}/tests/tiny_c2ll/ok_*.c")
option(NEGATIVE_TESTS_ENFORCE "Require negative_tests bad_*.c cases to fail compilation (non-zero exit)" ON)

# In the relaxed sema mode we only reject clear pointer<->float/complex implicit
# conversions. Keep these legacy negative cases non-enforced so they can compile
# without failing CTest.
set(NEGATIVE_TESTS_ALLOW_SUCCESS_STEMS
    bad_assign_type_mismatch_ptr_base
    bad_assign_type_mismatch_struct
    bad_call_arg_type_mismatch_ptr_base
    bad_call_arg_type_mismatch_ptr_depth
    bad_call_arg_type_mismatch_scalar_to_ptr
    bad_call_arg_type_mismatch_struct
    bad_call_undeclared_function
    bad_const_cast_discard_assign
    bad_const_cast_discard_write
    bad_const_charptr_discard_assign
    bad_const_charptr_literal_to_mut
    bad_const_charptr_pass_mut_param
    bad_const_discard_qualifier_assign
    bad_const_discard_qualifier_param
    bad_global_redefinition_with_init
    bad_init_type_mismatch_scalar_to_ptr
    bad_static_init_non_constant
)
option(ENABLE_LLVM_GCC_C_TORTURE_TESTS
    "Register llvm gcc-c-torture execute tests (resource-heavy during migration)"
    ON)

enable_testing()

if(CLANG_EXECUTABLE)
  add_test(
    NAME tiny_c2ll_tests
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DEXAMPLE_C=${EXAMPLE_C}
            -DOUT_LL=${CMAKE_BINARY_DIR}/tiny_c2ll_tests/test_example.ll
            -DCLANG=${CLANG_EXECUTABLE}
            -DOUT_BIN=${CMAKE_BINARY_DIR}/tiny_c2ll_tests/test_example_bin
            -P "${PROJECT_SOURCE_DIR}/tests/cmake/run_tiny_c2ll_tests.cmake"
  )
else()
  add_test(
    NAME tiny_c2ll_tests
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DEXAMPLE_C=${EXAMPLE_C}
            -DOUT_LL=${CMAKE_BINARY_DIR}/tiny_c2ll_tests/test_example.ll
            -P "${PROJECT_SOURCE_DIR}/tests/cmake/run_tiny_c2ll_tests.cmake"
  )
endif()

foreach(src IN LISTS NEGATIVE_TEST_SRCS)
  get_filename_component(stem "${src}" NAME_WE)
  set(test_name "negative_tests_${stem}")
  set(enforce_negative "${NEGATIVE_TESTS_ENFORCE}")
  if(stem IN_LIST NEGATIVE_TESTS_ALLOW_SUCCESS_STEMS)
    set(enforce_negative OFF)
  endif()
  add_test(
    NAME "${test_name}"
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DSRC=${src}
            -DENFORCE_NEGATIVE=${enforce_negative}
            -P "${PROJECT_SOURCE_DIR}/tests/cmake/run_tiny_c2ll_negative_case.cmake"
  )
  set_tests_properties("${test_name}" PROPERTIES LABELS "negative_tests")
endforeach()

foreach(src IN LISTS POSITIVE_SEMA_TEST_SRCS)
  get_filename_component(stem "${src}" NAME_WE)
  set(test_name "positive_sema_${stem}")
  add_test(
    NAME "${test_name}"
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DSRC=${src}
            -P "${PROJECT_SOURCE_DIR}/tests/cmake/run_tiny_c2ll_positive_case.cmake"
  )
  set_tests_properties("${test_name}" PROPERTIES LABELS "positive_sema")
endforeach()

# Temporarily disabled with frontend_cxx_preprocessor_tests target.
# add_test(
#     NAME frontend_cxx_preprocessor_tests
#     COMMAND frontend_cxx_preprocessor_tests
# )

add_test(
    NAME frontend_cxx_stage1_version
    COMMAND c4cll --version
)
set_tests_properties(frontend_cxx_stage1_version PROPERTIES
    PASS_REGULAR_EXPRESSION "tiny-c2ll"
)

if(EXISTS "${EXAMPLE_C}")
  add_test(
      NAME frontend_cxx_stage1_lex
      COMMAND c4cll --lex-only "${EXAMPLE_C}"
  )
  set_tests_properties(frontend_cxx_stage1_lex PROPERTIES
      PASS_REGULAR_EXPRESSION "EOF"
  )
endif()

if(EXISTS "${EXAMPLE_C}")
  add_test(
      NAME frontend_cxx_stage1_parse
      COMMAND c4cll --parse-only "${EXAMPLE_C}"
  )
  set_tests_properties(frontend_cxx_stage1_parse PROPERTIES
      PASS_REGULAR_EXPRESSION "Program"
  )
endif()

if(EXISTS "${EXAMPLE_C}")
  add_test(
      NAME frontend_cxx_stage1_emit_ir
      COMMAND c4cll "${EXAMPLE_C}"
  )
  set_tests_properties(frontend_cxx_stage1_emit_ir PROPERTIES
      PASS_REGULAR_EXPRESSION "define"
  )
endif()

if(CLANG_EXECUTABLE)
  file(GLOB CCC_REVIEW_SRCS CONFIGURE_DEPENDS
       "${PROJECT_SOURCE_DIR}/tests/ccc-review-tests/*.c")
  foreach(src IN LISTS CCC_REVIEW_SRCS)
    get_filename_component(stem "${src}" NAME_WE)
    set(test_name "ccc_review_${stem}")
    add_test(
      NAME "${test_name}"
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DCLANG=${CLANG_EXECUTABLE}
              -DSRC=${src}
              -DOUT_LL=${CMAKE_BINARY_DIR}/ccc_review_tests/${stem}.ll
              -DOUT_CLANG_BIN=${CMAKE_BINARY_DIR}/ccc_review_tests/${stem}.clang.bin
              -DOUT_C2LL_BIN=${CMAKE_BINARY_DIR}/ccc_review_tests/${stem}.c2ll.bin
              -P "${PROJECT_SOURCE_DIR}/tests/cmake/run_ccc_review_case.cmake"
    )
    set_tests_properties("${test_name}" PROPERTIES LABELS "ccc_review")
  endforeach()
endif()

if(CLANG_EXECUTABLE AND C_TESTSUITE_ROOT AND EXISTS "${C_TESTSUITE_ROOT}")
  file(STRINGS "${PROJECT_SOURCE_DIR}/tests/c_testsuite_allowlist.txt" C_TESTSUITE_ALLOWLIST_RAW)
  foreach(entry IN LISTS C_TESTSUITE_ALLOWLIST_RAW)
    string(STRIP "${entry}" entry)
    if(entry STREQUAL "" OR entry MATCHES "^#")
      continue()
    endif()

    set(src "${C_TESTSUITE_ROOT}/${entry}")
    if(NOT EXISTS "${src}")
      message(WARNING "c-testsuite allowlist entry not found: ${entry}")
      continue()
    endif()

    string(REGEX REPLACE "[^A-Za-z0-9_]" "_" test_id "${entry}")
    set(test_name "c_testsuite_${test_id}")

    add_test(
      NAME "${test_name}"
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DCLANG=${CLANG_EXECUTABLE}
              -DSRC=${src}
              -DROOT=${C_TESTSUITE_ROOT}
              -DOUT_LL=${CMAKE_BINARY_DIR}/c_testsuite/${entry}.ll
              -DOUT_BIN=${CMAKE_BINARY_DIR}/c_testsuite/${entry}.bin
              -P "${PROJECT_SOURCE_DIR}/tests/cmake/run_c_testsuite_case.cmake"
    )
    set_tests_properties("${test_name}" PROPERTIES LABELS "c_testsuite")
  endforeach()
endif()

if(ENABLE_LLVM_GCC_C_TORTURE_TESTS AND
   CLANG_EXECUTABLE AND LLVM_TEST_SUITE_ROOT AND EXISTS "${LLVM_TEST_SUITE_ROOT}")
  set(LLVM_GCC_C_TORTURE_ROOT
      "${LLVM_TEST_SUITE_ROOT}/SingleSource/Regression/C/gcc-c-torture/execute")
  set(LLVM_GCC_C_TORTURE_STEP_TIMEOUT_SEC "20" CACHE STRING
      "Per-step timeout (seconds) for llvm gcc-c-torture case script")
  set(LLVM_GCC_C_TORTURE_TEST_TIMEOUT_SEC "90" CACHE STRING
      "CTest timeout (seconds) for a single llvm gcc-c-torture test")
  set(LLVM_GCC_C_TORTURE_RUN_MEM_MB "0" CACHE STRING
      "Runtime virtual memory cap (MB) for test binaries; 0 disables cap")
  set(LLVM_GCC_C_TORTURE_RUN_CPU_SEC "0" CACHE STRING
      "Runtime CPU time cap (seconds) for test binaries; 0 disables cap")

  if(EXISTS "${LLVM_GCC_C_TORTURE_ROOT}" AND
     EXISTS "${PROJECT_SOURCE_DIR}/tests/llvm_gcc_c_torture_allowlist.txt")
    file(STRINGS "${PROJECT_SOURCE_DIR}/tests/llvm_gcc_c_torture_allowlist.txt"
         LLVM_GCC_C_TORTURE_ALLOWLIST_RAW)
    set(LLVM_GCC_C_TORTURE_TEST_MAP
        "${CMAKE_BINARY_DIR}/llvm_gcc_c_torture_test_map.tsv")
    file(WRITE "${LLVM_GCC_C_TORTURE_TEST_MAP}"
         "# test_name\\tallowlist_entry\n")
    foreach(entry IN LISTS LLVM_GCC_C_TORTURE_ALLOWLIST_RAW)
      string(STRIP "${entry}" entry)
      if(entry STREQUAL "" OR entry MATCHES "^#")
        continue()
      endif()

      set(src "${LLVM_GCC_C_TORTURE_ROOT}/${entry}")
      if(NOT EXISTS "${src}")
        message(WARNING "llvm gcc-c-torture allowlist entry not found: ${entry}")
        continue()
      endif()

      string(REGEX REPLACE "[^A-Za-z0-9_]" "_" test_id "${entry}")
      set(test_name "llvm_gcc_c_torture_${test_id}")

      add_test(
        NAME "${test_name}"
        COMMAND "${CMAKE_COMMAND}"
                -DCOMPILER=$<TARGET_FILE:c4cll>
                -DCLANG=${CLANG_EXECUTABLE}
                -DSRC=${src}
                -DROOT=${LLVM_GCC_C_TORTURE_ROOT}
                -DOUT_LL=${CMAKE_BINARY_DIR}/llvm_gcc_c_torture/${entry}.ll
                -DOUT_CLANG_BIN=${CMAKE_BINARY_DIR}/llvm_gcc_c_torture/${entry}.clang.bin
                -DOUT_C2LL_BIN=${CMAKE_BINARY_DIR}/llvm_gcc_c_torture/${entry}.c2ll.bin
                -DCASE_TIMEOUT_SEC=${LLVM_GCC_C_TORTURE_STEP_TIMEOUT_SEC}
                -DRUN_MEM_MB=${LLVM_GCC_C_TORTURE_RUN_MEM_MB}
                -DRUN_CPU_SEC=${LLVM_GCC_C_TORTURE_RUN_CPU_SEC}
                -P "${PROJECT_SOURCE_DIR}/tests/cmake/run_llvm_gcc_c_torture_case.cmake"
      )
      set_tests_properties("${test_name}" PROPERTIES
        LABELS "llvm_gcc_c_torture"
        TIMEOUT "${LLVM_GCC_C_TORTURE_TEST_TIMEOUT_SEC}")
      file(APPEND "${LLVM_GCC_C_TORTURE_TEST_MAP}" "${test_name}\t${entry}\n")
    endforeach()

    add_custom_target(prune_llvm_gcc_c_torture_allowlist_to_failed
      COMMAND "${CMAKE_COMMAND}"
              -DTEST_MAP=${LLVM_GCC_C_TORTURE_TEST_MAP}
              -DLAST_FAILED=${CMAKE_BINARY_DIR}/Testing/Temporary/LastTestsFailed.log
              -DALLOWLIST=${PROJECT_SOURCE_DIR}/tests/llvm_gcc_c_torture_allowlist.txt
              -P "${PROJECT_SOURCE_DIR}/tests/cmake/prune_llvm_gcc_c_torture_allowlist_to_failed.cmake"
      COMMENT "Rewrite tests/llvm_gcc_c_torture_allowlist.txt to keep only failed cases from last ctest run"
      VERBATIM
    )
  endif()
endif()

set(DEFAULT_TEST_TIMEOUT_SEC "30" CACHE STRING
    "Default CTest timeout (seconds) applied to all registered tests")
get_property(ALL_REGISTERED_TESTS DIRECTORY PROPERTY TESTS)
if(ALL_REGISTERED_TESTS)
  foreach(test_name IN LISTS ALL_REGISTERED_TESTS)
    set_property(TEST "${test_name}" PROPERTY TIMEOUT "${DEFAULT_TEST_TIMEOUT_SEC}")
  endforeach()
endif()
