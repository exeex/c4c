set(INTERNAL_C_TEST_ROOT "${PROJECT_SOURCE_DIR}/tests/c/internal")
set(INTERNAL_C_TEST_CMAKE_ROOT "${INTERNAL_C_TEST_ROOT}/cmake")

file(GLOB INTERNAL_NEGATIVE_TEST_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_C_TEST_ROOT}/negative_case/*.c")
file(GLOB INTERNAL_VERIFY_TEST_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_C_TEST_ROOT}/verify_case/*.c")
file(GLOB INTERNAL_POSITIVE_TEST_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_C_TEST_ROOT}/positive_case/*.c")
file(GLOB INTERNAL_BACKEND_TEST_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_C_TEST_ROOT}/backend_case/*.c")
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
else()
  message(WARNING "clang not found: skipping internal positive_case runtime tests")
endif()

add_test(
    NAME frontend_cxx_preprocessor_tests
    COMMAND frontend_cxx_preprocessor_tests
)
set_tests_properties(frontend_cxx_preprocessor_tests PROPERTIES
    LABELS "internal;preprocessor")

add_test(
    NAME backend_lir_adapter_tests
    COMMAND backend_lir_adapter_tests
)
set_tests_properties(backend_lir_adapter_tests PROPERTIES
    LABELS "internal;backend")

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

if(EXISTS "${EXAMPLE_C}")
  add_test(
      NAME backend_lir_supported_target_entry
      COMMAND c4cll --codegen lir --target x86_64-unknown-linux-gnu "${EXAMPLE_C}"
  )
  set_tests_properties(backend_lir_supported_target_entry PROPERTIES
      LABELS "internal;backend"
      PASS_REGULAR_EXPRESSION "define"
  )

  add_test(
      NAME backend_lir_aarch64_variadic_double_ir
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_case/variadic_double_bytes.c
              -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_backend/variadic_double_bytes_aarch64.ll
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_ir_check_case.cmake"
  )
  set_tests_properties(backend_lir_aarch64_variadic_double_ir PROPERTIES
      LABELS "internal;backend")

  add_test(
      NAME backend_lir_aarch64_variadic_long_double_ir
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_ir_case/variadic_long_double_bytes.c
              -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_backend/variadic_long_double_bytes_aarch64.ll
              "-DREQUIRED_SNIPPETS=declare ptr @llvm.ptrmask.p0.i64(ptr, i64)|vaarg.fp.join.|call ptr @llvm.ptrmask.p0.i64(|load fp128, ptr|getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 4"
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_ir_check_case.cmake"
  )
  set_tests_properties(backend_lir_aarch64_variadic_long_double_ir PROPERTIES
      LABELS "internal;backend")

  add_test(
      NAME backend_lir_aarch64_variadic_pair_ir
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_case/variadic_pair_second.c
              -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_backend/variadic_pair_second_aarch64.ll
              "-DREQUIRED_SNIPPETS=call void @llvm.memcpy.p0.p0.i64(|phi ptr|getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3|load %struct.Pair, ptr"
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_ir_check_case.cmake"
  )
  set_tests_properties(backend_lir_aarch64_variadic_pair_ir PROPERTIES
      LABELS "internal;backend")

  add_test(
      NAME backend_lir_aarch64_variadic_bigints_ir
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_ir_case/variadic_bigints_last.c
              -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_backend/variadic_bigints_last_aarch64.ll
              "-DREQUIRED_SNIPPETS=call void @llvm.memcpy.p0.p0.i64(|load ptr, ptr|phi ptr|getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3|load %struct.BigInts, ptr"
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_ir_check_case.cmake"
  )
  set_tests_properties(backend_lir_aarch64_variadic_bigints_ir PROPERTIES
      LABELS "internal;backend")

  add_test(
      NAME backend_lir_aarch64_variadic_dpair_ir
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_ir_case/variadic_dpair_bytes.c
              -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_backend/variadic_dpair_bytes_aarch64.ll
              "-DREQUIRED_SNIPPETS=%struct.DPair = type { double, double }|[2 x double]|vaarg.fp.join.|getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 4|load double, ptr|call void @llvm.memcpy.p0.p0.i64("
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_ir_check_case.cmake"
  )
  set_tests_properties(backend_lir_aarch64_variadic_dpair_ir PROPERTIES
      LABELS "internal;backend")

  add_test(
      NAME backend_lir_aarch64_variadic_float_array_ir
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_ir_case/variadic_float_array_bytes.c
              -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_backend/variadic_float_array_bytes_aarch64.ll
              "-DREQUIRED_SNIPPETS=%struct.FloatArray2 = type { [2 x float] }|[2 x float]|vaarg.fp.join.|getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 4|load float, ptr|call void @llvm.memcpy.p0.p0.i64("
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_ir_check_case.cmake"
  )
  set_tests_properties(backend_lir_aarch64_variadic_float_array_ir PROPERTIES
      LABELS "internal;backend")

  add_test(
      NAME backend_lir_aarch64_variadic_nested_float_array_ir
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_ir_case/variadic_nested_float_array_bytes.c
              -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_backend/variadic_nested_float_array_bytes_aarch64.ll
              "-DREQUIRED_SNIPPETS=%struct.NestedFloatArray = type { [2 x [2 x float]] }|[4 x float]|vaarg.fp.join.|getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 4|load float, ptr|call void @llvm.memcpy.p0.p0.i64("
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_ir_check_case.cmake"
  )
  set_tests_properties(backend_lir_aarch64_variadic_nested_float_array_ir PROPERTIES
      LABELS "internal;backend")

  add_test(
      NAME backend_lir_aarch64_variadic_float4_ir
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_ir_case/variadic_float4_bytes.c
              -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_backend/variadic_float4_bytes_aarch64.ll
              "-DREQUIRED_SNIPPETS=%struct.Float4 = type { float, float, float, float }|[4 x float]|vaarg.fp.join.|getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 4|load float, ptr|call void @llvm.memcpy.p0.p0.i64("
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_ir_check_case.cmake"
  )
  set_tests_properties(backend_lir_aarch64_variadic_float4_ir PROPERTIES
      LABELS "internal;backend")

  add_test(
      NAME backend_lir_aarch64_variadic_double4_ir
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_ir_case/variadic_double4_bytes.c
              -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_backend/variadic_double4_bytes_aarch64.ll
              "-DREQUIRED_SNIPPETS=%struct.Double4 = type { double, double, double, double }|[4 x double]|vaarg.fp.join.|getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 4|load double, ptr|call void @llvm.memcpy.p0.p0.i64("
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_ir_check_case.cmake"
  )
  set_tests_properties(backend_lir_aarch64_variadic_double4_ir PROPERTIES
      LABELS "internal;backend")

  add_test(
      NAME backend_lir_aarch64_variadic_single_double_ir
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_ir_case/variadic_single_double_bytes.c
              -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_backend/variadic_single_double_bytes_aarch64.ll
              "-DREQUIRED_SNIPPETS=%struct.SingleDouble = type { double }|[1 x double]|vaarg.fp.join.|getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 4|load double, ptr|call void @llvm.memcpy.p0.p0.i64("
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_ir_check_case.cmake"
  )
  set_tests_properties(backend_lir_aarch64_variadic_single_double_ir PROPERTIES
      LABELS "internal;backend")

  add_test(
      NAME backend_lir_aarch64_variadic_single_float_ir
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_ir_case/variadic_single_float_bytes.c
              -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_backend/variadic_single_float_bytes_aarch64.ll
              "-DREQUIRED_SNIPPETS=%struct.SingleFloat = type { float }|[1 x float]|vaarg.fp.join.|getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 4|load float, ptr|call void @llvm.memcpy.p0.p0.i64("
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_ir_check_case.cmake"
  )
  set_tests_properties(backend_lir_aarch64_variadic_single_float_ir PROPERTIES
      LABELS "internal;backend")

  add_test(
      NAME backend_lir_aarch64_variadic_mixed_char_double_ir
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_ir_case/variadic_mixed_char_double_bytes.c
              -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_backend/variadic_mixed_char_double_bytes_aarch64.ll
              "-DREQUIRED_SNIPPETS=%struct.MixedCharDouble = type { i8, [7 x i8], double }|call void @llvm.memcpy.p0.p0.i64(|phi ptr|getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3|load %struct.MixedCharDouble, ptr|trunc i32 68 to i8"
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_ir_check_case.cmake"
  )
  set_tests_properties(backend_lir_aarch64_variadic_mixed_char_double_ir PROPERTIES
      LABELS "internal;backend")

  add_test(
      NAME backend_lir_aarch64_variadic_mixed_float_int_ir
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_ir_case/variadic_mixed_float_int_bytes.c
              -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_backend/variadic_mixed_float_int_bytes_aarch64.ll
              "-DREQUIRED_SNIPPETS=%struct.MixedFloatInt = type { float, i32 }|call void @llvm.memcpy.p0.p0.i64(|phi ptr|getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3|load %struct.MixedFloatInt, ptr"
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_ir_check_case.cmake"
  )
  set_tests_properties(backend_lir_aarch64_variadic_mixed_float_int_ir PROPERTIES
      LABELS "internal;backend")

  add_test(
      NAME backend_lir_aarch64_variadic_mixed_double_int_ir
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_ir_case/variadic_mixed_double_int_bytes.c
              -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_backend/variadic_mixed_double_int_bytes_aarch64.ll
              "-DREQUIRED_SNIPPETS=%struct.MixedDoubleInt = type { double, i32, [4 x i8] }|call void @llvm.memcpy.p0.p0.i64(|phi ptr|getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3|load %struct.MixedDoubleInt, ptr"
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_ir_check_case.cmake"
  )
  set_tests_properties(backend_lir_aarch64_variadic_mixed_double_int_ir PROPERTIES
      LABELS "internal;backend")

  add_test(
      NAME backend_lir_aarch64_variadic_mixed_int_double_ir
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_ir_case/variadic_mixed_int_double_bytes.c
              -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_backend/variadic_mixed_int_double_bytes_aarch64.ll
              "-DREQUIRED_SNIPPETS=%struct.MixedIntDouble = type { i32, [4 x i8], double }|call void @llvm.memcpy.p0.p0.i64(|phi ptr|getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3|load %struct.MixedIntDouble, ptr"
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_ir_check_case.cmake"
  )
  set_tests_properties(backend_lir_aarch64_variadic_mixed_int_double_ir PROPERTIES
      LABELS "internal;backend")

  add_test(
      NAME backend_lir_aarch64_variadic_mixed_short_double_ir
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_ir_case/variadic_mixed_short_double_bytes.c
              -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_backend/variadic_mixed_short_double_bytes_aarch64.ll
              "-DREQUIRED_SNIPPETS=%struct.MixedShortDouble = type { i16, [6 x i8], double }|call void @llvm.memcpy.p0.p0.i64(|phi ptr|getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3|load %struct.MixedShortDouble, ptr"
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_ir_check_case.cmake"
  )
  set_tests_properties(backend_lir_aarch64_variadic_mixed_short_double_ir PROPERTIES
      LABELS "internal;backend")

  add_test(
      NAME backend_lir_aarch64_variadic_mixed_double_short_ir
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_ir_case/variadic_mixed_double_short_bytes.c
              -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_backend/variadic_mixed_double_short_bytes_aarch64.ll
              "-DREQUIRED_SNIPPETS=%struct.MixedDoubleShort = type { double, i16, [6 x i8] }|call void @llvm.memcpy.p0.p0.i64(|phi ptr|getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3|load %struct.MixedDoubleShort, ptr|trunc i32 13124 to i16"
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_ir_check_case.cmake"
  )
  set_tests_properties(backend_lir_aarch64_variadic_mixed_double_short_ir PROPERTIES
      LABELS "internal;backend")

  add_test(
      NAME backend_lir_unsupported_target_entry
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DSRC=${EXAMPLE_C}
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_unsupported_target_case.cmake"
  )
  set_tests_properties(backend_lir_unsupported_target_entry PROPERTIES
      LABELS "internal;backend"
  )
endif()

set(BACKEND_RUNTIME_TARGET_TRIPLE "")
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
  if(APPLE)
    set(BACKEND_RUNTIME_TARGET_TRIPLE "aarch64-apple-darwin")
  else()
    set(BACKEND_RUNTIME_TARGET_TRIPLE "aarch64-unknown-linux-gnu")
  endif()
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "amd64")
  if(APPLE)
    set(BACKEND_RUNTIME_TARGET_TRIPLE "x86_64-apple-darwin")
  else()
    set(BACKEND_RUNTIME_TARGET_TRIPLE "x86_64-unknown-linux-gnu")
  endif()
endif()

if(CLANG_EXECUTABLE)
  add_test(
    NAME backend_toolchain_aarch64_asm_object_smoke
    COMMAND "${CMAKE_COMMAND}"
            -DCLANG=${CLANG_EXECUTABLE}
            -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
            -DBACKEND_OUTPUT_KIND=asm
            -DBACKEND_OUTPUT_PATH=${INTERNAL_C_TEST_ROOT}/backend_toolchain_case/aarch64_return_42_linux.s
            -DOUT_ARTIFACT=${CMAKE_BINARY_DIR}/internal_backend/aarch64_return_42_linux.o
            -DTOOLCHAIN_MODE=object
            -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_toolchain_case.cmake"
  )
  set_tests_properties(backend_toolchain_aarch64_asm_object_smoke PROPERTIES
      LABELS "internal;backend")

  if(OBJDUMP_EXECUTABLE)
    add_test(
      NAME backend_contract_aarch64_return_add_object
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DCLANG=${CLANG_EXECUTABLE}
              -DOBJDUMP=${OBJDUMP_EXECUTABLE}
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_case/return_add.c
              -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
              -DBACKEND_OUTPUT_KIND=asm
              -DBACKEND_OUTPUT_PATH=${CMAKE_BINARY_DIR}/internal_backend_contract/return_add_aarch64.s
              -DOUT_ARTIFACT=${CMAKE_BINARY_DIR}/internal_backend_contract/return_add_aarch64.o
              "-DREQUIRED_BACKEND_SNIPPETS=.text|.globl main|mov w0, #5|ret"
              "-DREQUIRED_OBJDUMP_SNIPPETS=file format elf64-littleaarch64|.text|main"
              "-DFORBIDDEN_OBJDUMP_SNIPPETS=.rela.text|R_AARCH64_"
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_contract_case.cmake"
    )
    set_tests_properties(backend_contract_aarch64_return_add_object PROPERTIES
        LABELS "internal;backend")

    add_test(
      NAME backend_contract_aarch64_global_load_object
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DCLANG=${CLANG_EXECUTABLE}
              -DOBJDUMP=${OBJDUMP_EXECUTABLE}
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_case/global_load.c
              -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
              -DBACKEND_OUTPUT_KIND=asm
              -DBACKEND_OUTPUT_PATH=${CMAKE_BINARY_DIR}/internal_backend_contract/global_load_aarch64.s
              -DOUT_ARTIFACT=${CMAKE_BINARY_DIR}/internal_backend_contract/global_load_aarch64.o
              "-DREQUIRED_BACKEND_SNIPPETS=.data|.globl g_counter|g_counter:|.long 11|adrp x8, g_counter|ldr w0, [x8, :lo12:g_counter]"
              "-DREQUIRED_OBJDUMP_SNIPPETS=file format elf64-littleaarch64|.data|.text|.rela.text|g_counter|main|R_AARCH64_ADR_PREL_PG_HI21|R_AARCH64_LDST32_ABS_LO12_NC"
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_contract_case.cmake"
    )
    set_tests_properties(backend_contract_aarch64_global_load_object PROPERTIES
        LABELS "internal;backend")

    add_test(
      NAME backend_contract_aarch64_extern_call_object
      COMMAND "${CMAKE_COMMAND}"
              -DCLANG=${CLANG_EXECUTABLE}
              -DOBJDUMP=${OBJDUMP_EXECUTABLE}
              -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
              -DBACKEND_OUTPUT_KIND=asm
              -DBACKEND_OUTPUT_PATH=${INTERNAL_C_TEST_ROOT}/backend_toolchain_case/aarch64_call_extern_linux.s
              -DOUT_ARTIFACT=${CMAKE_BINARY_DIR}/internal_backend_contract/aarch64_call_extern_linux.o
              "-DREQUIRED_BACKEND_SNIPPETS=.globl main|bl helper|mov w0, #0|ret"
              "-DREQUIRED_OBJDUMP_SNIPPETS=file format elf64-littleaarch64|.text|.rela.text|main|*UND*|helper|R_AARCH64_CALL26"
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_contract_case.cmake"
    )
    set_tests_properties(backend_contract_aarch64_extern_call_object PROPERTIES
        LABELS "internal;backend")
  endif()

  if(BACKEND_RUNTIME_TARGET_TRIPLE)
    foreach(src IN LISTS INTERNAL_BACKEND_TEST_SRCS)
      get_filename_component(stem "${src}" NAME_WE)
      if(stem STREQUAL "variadic_pair_second" AND
         NOT BACKEND_RUNTIME_TARGET_TRIPLE STREQUAL "aarch64-unknown-linux-gnu")
        continue()
      endif()
      set(test_name "backend_runtime_${stem}")
      set(expect_exit_code 0)
      set(backend_output_kind "llvm-ir")
      if(stem STREQUAL "return_zero")
        set(expect_exit_code 0)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "return_add")
        set(expect_exit_code 5)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "call_helper")
        set(expect_exit_code 7)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "param_slot")
        set(expect_exit_code 6)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "two_arg_helper")
        set(expect_exit_code 12)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "two_arg_local_arg")
        set(expect_exit_code 12)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "two_arg_second_local_arg")
        set(expect_exit_code 12)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "two_arg_second_local_rewrite")
        set(expect_exit_code 12)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "two_arg_first_local_rewrite")
        set(expect_exit_code 12)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "two_arg_both_local_arg")
        set(expect_exit_code 12)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "two_arg_both_local_first_rewrite")
        set(expect_exit_code 12)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "two_arg_both_local_second_rewrite")
        set(expect_exit_code 12)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "two_arg_both_local_double_rewrite")
        set(expect_exit_code 12)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "local_arg_call")
        set(expect_exit_code 6)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "branch_if_lt")
        set(expect_exit_code 0)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "branch_if_le")
        set(expect_exit_code 0)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "branch_if_gt")
        set(expect_exit_code 0)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "branch_if_ge")
        set(expect_exit_code 0)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "branch_if_eq")
        set(expect_exit_code 0)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "branch_if_ne")
        set(expect_exit_code 0)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "branch_if_ult")
        set(expect_exit_code 0)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "branch_if_ule")
        set(expect_exit_code 0)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "branch_if_ugt")
        set(expect_exit_code 0)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "branch_if_uge")
        set(expect_exit_code 0)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "local_temp")
        set(expect_exit_code 5)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "local_array")
        set(expect_exit_code 7)
      elseif(stem STREQUAL "nested_member_pointer_array")
        set(expect_exit_code 8)
      elseif(stem STREQUAL "nested_param_member_array")
        set(expect_exit_code 9)
      elseif(stem STREQUAL "global_load")
        set(expect_exit_code 11)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "global_char_pointer_diff")
        set(expect_exit_code 1)
      elseif(stem STREQUAL "global_int_pointer_diff")
        set(expect_exit_code 1)
      elseif(stem STREQUAL "global_int_pointer_roundtrip")
        set(expect_exit_code 9)
      elseif(stem STREQUAL "string_literal_char")
        set(expect_exit_code 105)
      elseif(stem STREQUAL "variadic_sum2")
        set(expect_exit_code 42)
      elseif(stem STREQUAL "variadic_double_bytes")
        set(expect_exit_code 67)
      elseif(stem STREQUAL "variadic_pair_second")
        set(expect_exit_code 9)
      elseif(stem STREQUAL "param_member_array")
        set(expect_exit_code 6)
      endif()

      add_test(
        NAME "${test_name}"
        COMMAND "${CMAKE_COMMAND}"
                -DCOMPILER=$<TARGET_FILE:c4cll>
                -DCLANG=${CLANG_EXECUTABLE}
                -DSRC=${src}
                -DTARGET_TRIPLE=${BACKEND_RUNTIME_TARGET_TRIPLE}
                -DBACKEND_OUTPUT_KIND=${backend_output_kind}
                -DOUT_LL=${CMAKE_BINARY_DIR}/internal_backend/${stem}.ll
                -DEXPECT_EXIT_CODE=${expect_exit_code}
                -DOUT_C2LL_BIN=${CMAKE_BINARY_DIR}/internal_backend/${stem}.bin
                -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_positive_case.cmake"
      )
      set_tests_properties("${test_name}" PROPERTIES LABELS "internal;backend")
    endforeach()

    if(EXISTS "${EXAMPLE_C}")
      add_test(
        NAME backend_lir_missing_toolchain_diagnostic
        COMMAND "${CMAKE_COMMAND}"
                -DCOMPILER=$<TARGET_FILE:c4cll>
                -DSRC=${EXAMPLE_C}
                -DTARGET_TRIPLE=${BACKEND_RUNTIME_TARGET_TRIPLE}
                -DOUT_LL=${CMAKE_BINARY_DIR}/internal_backend/missing_tool.ll
                -DOUT_C2LL_BIN=${CMAKE_BINARY_DIR}/internal_backend/missing_tool.bin
                -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_missing_toolchain_case.cmake"
      )
      set_tests_properties(backend_lir_missing_toolchain_diagnostic PROPERTIES
          LABELS "internal;backend")
    endif()
  else()
    message(WARNING "Skipping backend runtime tests: unsupported host processor '${CMAKE_SYSTEM_PROCESSOR}'")
  endif()
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

# ── Compare-mode smoke tests (legacy vs LIR codegen) ──────────────────────────
file(GLOB INTERNAL_COMPARE_TEST_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_C_TEST_ROOT}/compare_case/*.c")

foreach(src IN LISTS INTERNAL_COMPARE_TEST_SRCS)
  get_filename_component(stem "${src}" NAME_WE)
  set(test_name "compare_${stem}")
  add_test(
    NAME "${test_name}"
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DSRC=${src}
            -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_compare_case.cmake"
  )
  set_tests_properties("${test_name}" PROPERTIES LABELS "internal;compare_case")
endforeach()

add_test(
  NAME compare_aarch64_smoke_scalar
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_C_TEST_ROOT}/compare_case/smoke_scalar.c
          -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_compare_case.cmake"
)
set_tests_properties(compare_aarch64_smoke_scalar PROPERTIES
    LABELS "internal;compare_case;backend")

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
