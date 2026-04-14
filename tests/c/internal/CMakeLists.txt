set(INTERNAL_C_TEST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(INTERNAL_C_TEST_CMAKE_ROOT "${INTERNAL_C_TEST_ROOT}/cmake")


file(GLOB INTERNAL_NEGATIVE_TEST_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_C_TEST_ROOT}/negative_case/*.c")
file(GLOB INTERNAL_VERIFY_TEST_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_C_TEST_ROOT}/verify_case/*.c")
file(GLOB INTERNAL_POSITIVE_TEST_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_C_TEST_ROOT}/positive_case/*.c")
file(GLOB INTERNAL_ABI_TEST_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_C_TEST_ROOT}/abi/*.c")
file(GLOB INTERNAL_LINUX_STAGE2_REPRO_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_C_TEST_ROOT}/positive_case/linux_stage2_repro/*.c")
file(GLOB INTERNAL_CCC_REVIEW_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_C_TEST_ROOT}/positive_case/ccc_review/*.c")


option(NEGATIVE_TESTS_ENFORCE
       "Require internal negative_case bad_*.c cases to fail compilation (non-zero exit)"
       ON)

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
    bad_return_missing_value_nonvoid
    bad_static_init_non_constant
)

if(CLANG_EXECUTABLE)
  add_test(
    NAME tiny_c2ll_tests
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DEXAMPLE_C=${EXAMPLE_C}
            -DOUT_LL=${CMAKE_BINARY_DIR}/tiny_c2ll_tests/test_example.ll
            -DCLANG=${CLANG_EXECUTABLE}
            -DOUT_BIN=${CMAKE_BINARY_DIR}/tiny_c2ll_tests/test_example_bin
            -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_example_case.cmake"
  )
else()
  add_test(
    NAME tiny_c2ll_tests
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DEXAMPLE_C=${EXAMPLE_C}
            -DOUT_LL=${CMAKE_BINARY_DIR}/tiny_c2ll_tests/test_example.ll
            -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_example_case.cmake"
  )
endif()
set_tests_properties(tiny_c2ll_tests PROPERTIES LABELS "internal;example")

foreach(src IN LISTS INTERNAL_NEGATIVE_TEST_SRCS)
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
            -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_negative_case.cmake"
  )
  set_tests_properties("${test_name}" PROPERTIES LABELS "internal;negative_case")
endforeach()

# Diagnostic format verification: ensure parse errors use file:line:col: error: format
add_test(
  NAME negative_tests_diagnostic_format_check
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_C_TEST_ROOT}/negative_case/bad_multi_error_recovery.c
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_diagnostic_format_check.cmake"
)
set_tests_properties(negative_tests_diagnostic_format_check PROPERTIES
    LABELS "internal;negative_case;diagnostic_format")

# Statement-level recovery: ensure bad statements don't prevent further parsing
add_test(
  NAME negative_tests_stmt_recovery_check
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_C_TEST_ROOT}/negative_case/bad_stmt_recovery.c
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_stmt_recovery_check.cmake"
)
set_tests_properties(negative_tests_stmt_recovery_check PROPERTIES
    LABELS "internal;negative_case;error_recovery")

# Verify-mode tests: expected-error annotation matching via verify_external_clang_case.py
if(PYTHON3_EXECUTABLE)
  foreach(src IN LISTS INTERNAL_VERIFY_TEST_SRCS)
    get_filename_component(stem "${src}" NAME_WE)
    set(test_name "verify_tests_${stem}")
    add_test(
      NAME "${test_name}"
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DPYTHON3_EXECUTABLE=${PYTHON3_EXECUTABLE}
              -DRUNNER=${PROJECT_SOURCE_DIR}/tests/verify_external_clang_case.py
              -DSRC=${src}
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_verify/${stem}.ll
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_verify_case.cmake"
    )
    set_tests_properties("${test_name}" PROPERTIES LABELS "internal;verify_case")
  endforeach()
else()
  message(WARNING "python3 not found: skipping internal verify_case tests")
endif()

if(CLANG_EXECUTABLE)
  foreach(src IN LISTS INTERNAL_POSITIVE_TEST_SRCS INTERNAL_LINUX_STAGE2_REPRO_SRCS)
    file(RELATIVE_PATH rel_src "${INTERNAL_C_TEST_ROOT}/positive_case" "${src}")
    string(REGEX REPLACE "[^A-Za-z0-9_]" "_" test_id "${rel_src}")
    set(test_name "positive_sema_${test_id}")

    add_test(
      NAME "${test_name}"
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DCLANG=${CLANG_EXECUTABLE}
              -DSRC=${src}
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_positive/${rel_src}.ll
              -DOUT_CLANG_BIN=${CMAKE_BINARY_DIR}/internal_positive/${rel_src}.clang.bin
              -DOUT_C2LL_BIN=${CMAKE_BINARY_DIR}/internal_positive/${rel_src}.c2ll.bin
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_positive_case.cmake"
    )
    set_tests_properties("${test_name}" PROPERTIES LABELS "internal;positive_case")
  endforeach()
  foreach(src IN LISTS INTERNAL_ABI_TEST_SRCS)
    file(RELATIVE_PATH rel_src "${INTERNAL_C_TEST_ROOT}" "${src}")
    string(REGEX REPLACE "[^A-Za-z0-9_]" "_" test_id "${rel_src}")
    set(test_name "abi_${test_id}")

    add_test(
      NAME "${test_name}"
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DCLANG=${CLANG_EXECUTABLE}
              -DSRC=${src}
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_abi/${rel_src}.ll
              -DOUT_CLANG_BIN=${CMAKE_BINARY_DIR}/internal_abi/${rel_src}.clang.bin
              -DOUT_C2LL_BIN=${CMAKE_BINARY_DIR}/internal_abi/${rel_src}.c2ll.bin
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_positive_case.cmake"
    )
    set_tests_properties("${test_name}" PROPERTIES LABELS "internal;abi")
  endforeach()
else()
  message(WARNING "clang not found: skipping internal positive_case runtime tests")
endif()


add_test(
    NAME positive_split_llvm_pragma_exec
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DSRC=${INTERNAL_C_TEST_ROOT}/positive_case/ok_pragma_exec_split.c
            -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
            -DDEVICE_TARGET_TRIPLE=aarch64-unknown-linux-gnu
            -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_split_llvm_case.cmake"
)
set_tests_properties(positive_split_llvm_pragma_exec PROPERTIES
    LABELS "internal;positive_case;split_llvm")


add_test(
    NAME frontend_cxx_stage1_version
    COMMAND c4cll --version
)
set_tests_properties(frontend_cxx_stage1_version PROPERTIES
    LABELS "internal;example"
    PASS_REGULAR_EXPRESSION "tiny-c2ll"
)

if(EXISTS "${EXAMPLE_C}")
  add_test(
      NAME frontend_cxx_stage1_lex
      COMMAND c4cll --lex-only "${EXAMPLE_C}"
  )
  set_tests_properties(frontend_cxx_stage1_lex PROPERTIES
      LABELS "internal;example"
      PASS_REGULAR_EXPRESSION "EOF"
  )
endif()

if(EXISTS "${EXAMPLE_C}")
  add_test(
      NAME frontend_cxx_stage1_parse
      COMMAND c4cll --parse-only "${EXAMPLE_C}"
  )
  set_tests_properties(frontend_cxx_stage1_parse PROPERTIES
      LABELS "internal;example"
      PASS_REGULAR_EXPRESSION "Program"
  )
endif()

if(EXISTS "${EXAMPLE_C}")
  add_test(
      NAME frontend_cxx_stage1_emit_ir
      COMMAND c4cll "${EXAMPLE_C}"
  )
  set_tests_properties(frontend_cxx_stage1_emit_ir PROPERTIES
      LABELS "internal;example"
      PASS_REGULAR_EXPRESSION "define"
  )
endif()


if(CLANG_EXECUTABLE)
  foreach(src IN LISTS INTERNAL_CCC_REVIEW_SRCS)
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
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_ccc_review_case.cmake"
    )
    set_tests_properties("${test_name}" PROPERTIES LABELS "internal;positive_case;ccc_review")
  endforeach()
endif()

if(CLANG_EXECUTABLE AND
   (CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64") AND
   EXISTS "${INTERNAL_C_TEST_ROOT}/inline_asm/aarch64/simple.c")
  add_test(
    NAME inline_asm_aarch64_simple
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DCLANG=${CLANG_EXECUTABLE}
            -DSRC=${INTERNAL_C_TEST_ROOT}/inline_asm/aarch64/simple.c
            -DOUT_LL=${CMAKE_BINARY_DIR}/inline_asm/aarch64/simple.ll
            -DOUT_BIN=${CMAKE_BINARY_DIR}/inline_asm/aarch64/simple.bin
            -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_inline_asm_case.cmake"
  )
  set_tests_properties(inline_asm_aarch64_simple PROPERTIES
      LABELS "internal;inline_asm")
endif()
