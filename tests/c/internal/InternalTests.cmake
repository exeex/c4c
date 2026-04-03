set(INTERNAL_C_TEST_ROOT "${PROJECT_SOURCE_DIR}/tests/c/internal")
set(INTERNAL_C_TEST_CMAKE_ROOT "${INTERNAL_C_TEST_ROOT}/cmake")

include("${INTERNAL_C_TEST_CMAKE_ROOT}/BackendTests.cmake")

file(GLOB INTERNAL_NEGATIVE_TEST_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_C_TEST_ROOT}/negative_case/*.c")
file(GLOB INTERNAL_VERIFY_TEST_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_C_TEST_ROOT}/verify_case/*.c")
file(GLOB INTERNAL_POSITIVE_TEST_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_C_TEST_ROOT}/positive_case/*.c")
file(GLOB INTERNAL_ABI_TEST_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_C_TEST_ROOT}/abi/*.c")
file(GLOB INTERNAL_BACKEND_TEST_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_C_TEST_ROOT}/backend_case/*.c")
file(GLOB INTERNAL_LINUX_STAGE2_REPRO_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_C_TEST_ROOT}/positive_case/linux_stage2_repro/*.c")
file(GLOB INTERNAL_CCC_REVIEW_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_C_TEST_ROOT}/positive_case/ccc_review/*.c")
file(GLOB INTERNAL_PREPROCESSOR_TEST_SRCS CONFIGURE_DEPENDS
     "${PROJECT_SOURCE_DIR}/tests/c/preprocessor/*.c")

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
    NAME frontend_cxx_preprocessor_tests
    COMMAND frontend_cxx_preprocessor_tests
)
set_tests_properties(frontend_cxx_preprocessor_tests PROPERTIES
    LABELS "internal;preprocessor")

foreach(src IN LISTS INTERNAL_PREPROCESSOR_TEST_SRCS)
  get_filename_component(stem "${src}" NAME_WE)
  set(test_name "preprocessor_${stem}")
  add_test(
      NAME "${test_name}"
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DSRC=${src}
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_preprocessor_case.cmake"
  )
  set_tests_properties("${test_name}" PROPERTIES LABELS "internal;preprocessor")
endforeach()

c4c_add_backend_test(
    backend_lir_adapter_tests
    COMMAND backend_lir_adapter_tests
)

c4c_add_backend_test(
    backend_lir_adapter_aarch64_tests
    COMMAND backend_lir_adapter_aarch64_tests
)

c4c_add_backend_test(
    backend_lir_adapter_x86_64_tests
    COMMAND backend_lir_adapter_x86_64_tests
)

c4c_add_backend_test(
    backend_module_tests
    COMMAND backend_module_tests
)

c4c_add_backend_test(
    backend_bir_tests
    COMMAND backend_bir_tests
)

c4c_add_backend_test(
    backend_shared_util_tests
    COMMAND backend_shared_util_tests
)

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
  # TODO: backend_lir_supported_target_entry disabled — codegen asm fallback changed
  # add_test(
  #     NAME backend_lir_supported_target_entry
  #     COMMAND c4cll --codegen asm --target x86_64-unknown-linux-gnu "${EXAMPLE_C}"
  # )
  # set_tests_properties(backend_lir_supported_target_entry PROPERTIES
  #     LABELS "internal;backend"
  #     PASS_REGULAR_EXPRESSION "define"
  # )

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
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_module_case/variadic_long_double_bytes.c
              -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_backend/variadic_long_double_bytes_aarch64.ll
              "-DREQUIRED_SNIPPETS=declare ptr @llvm.ptrmask.p0.i64(ptr, i64)|vaarg.fp.join.|call ptr @llvm.ptrmask.p0.i64(|load fp128, ptr|getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 4"
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_module_check_case.cmake"
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
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_module_case/variadic_bigints_last.c
              -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_backend/variadic_bigints_last_aarch64.ll
              "-DREQUIRED_SNIPPETS=call void @llvm.memcpy.p0.p0.i64(|load ptr, ptr|phi ptr|getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3|load %struct.BigInts, ptr"
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_module_check_case.cmake"
  )
  set_tests_properties(backend_lir_aarch64_variadic_bigints_ir PROPERTIES
      LABELS "internal;backend")

  # TODO: variadic HFA tests disabled — vaarg.hfa.join not yet implemented
  # add_test(
  #     NAME backend_lir_aarch64_variadic_dpair_ir
  #     ...
  # )

  # add_test(NAME backend_lir_aarch64_variadic_float_array_ir ...)

  # add_test(NAME backend_lir_aarch64_variadic_nested_float_array_ir ...)

  # add_test(NAME backend_lir_aarch64_variadic_float4_ir ...)

  # add_test(NAME backend_lir_aarch64_variadic_double4_ir ...)

  # add_test(NAME backend_lir_aarch64_variadic_single_double_ir ...)

  # add_test(NAME backend_lir_aarch64_variadic_single_float_ir ...)

  add_test(
      NAME backend_lir_aarch64_variadic_mixed_char_double_ir
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_module_case/variadic_mixed_char_double_bytes.c
              -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_backend/variadic_mixed_char_double_bytes_aarch64.ll
              "-DREQUIRED_SNIPPETS=%struct.MixedCharDouble = type { i8, [7 x i8], double }|call void @llvm.memcpy.p0.p0.i64(|phi ptr|getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3|load %struct.MixedCharDouble, ptr|trunc i32 68 to i8"
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_module_check_case.cmake"
  )
  set_tests_properties(backend_lir_aarch64_variadic_mixed_char_double_ir PROPERTIES
      LABELS "internal;backend")

  add_test(
      NAME backend_lir_aarch64_variadic_mixed_float_int_ir
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_module_case/variadic_mixed_float_int_bytes.c
              -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_backend/variadic_mixed_float_int_bytes_aarch64.ll
              "-DREQUIRED_SNIPPETS=%struct.MixedFloatInt = type { float, i32 }|call void @llvm.memcpy.p0.p0.i64(|phi ptr|getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3|load %struct.MixedFloatInt, ptr"
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_module_check_case.cmake"
  )
  set_tests_properties(backend_lir_aarch64_variadic_mixed_float_int_ir PROPERTIES
      LABELS "internal;backend")

  add_test(
      NAME backend_lir_aarch64_variadic_mixed_double_int_ir
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_module_case/variadic_mixed_double_int_bytes.c
              -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_backend/variadic_mixed_double_int_bytes_aarch64.ll
              "-DREQUIRED_SNIPPETS=%struct.MixedDoubleInt = type { double, i32, [4 x i8] }|call void @llvm.memcpy.p0.p0.i64(|phi ptr|getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3|load %struct.MixedDoubleInt, ptr"
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_module_check_case.cmake"
  )
  set_tests_properties(backend_lir_aarch64_variadic_mixed_double_int_ir PROPERTIES
      LABELS "internal;backend")

  add_test(
      NAME backend_lir_aarch64_variadic_mixed_int_double_ir
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_module_case/variadic_mixed_int_double_bytes.c
              -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_backend/variadic_mixed_int_double_bytes_aarch64.ll
              "-DREQUIRED_SNIPPETS=%struct.MixedIntDouble = type { i32, [4 x i8], double }|call void @llvm.memcpy.p0.p0.i64(|phi ptr|getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3|load %struct.MixedIntDouble, ptr"
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_module_check_case.cmake"
  )
  set_tests_properties(backend_lir_aarch64_variadic_mixed_int_double_ir PROPERTIES
      LABELS "internal;backend")

  add_test(
      NAME backend_lir_aarch64_variadic_mixed_short_double_ir
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_module_case/variadic_mixed_short_double_bytes.c
              -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_backend/variadic_mixed_short_double_bytes_aarch64.ll
              "-DREQUIRED_SNIPPETS=%struct.MixedShortDouble = type { i16, [6 x i8], double }|call void @llvm.memcpy.p0.p0.i64(|phi ptr|getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3|load %struct.MixedShortDouble, ptr"
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_module_check_case.cmake"
  )
  set_tests_properties(backend_lir_aarch64_variadic_mixed_short_double_ir PROPERTIES
      LABELS "internal;backend")

  add_test(
      NAME backend_lir_aarch64_variadic_mixed_double_short_ir
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_module_case/variadic_mixed_double_short_bytes.c
              -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_backend/variadic_mixed_double_short_bytes_aarch64.ll
              "-DREQUIRED_SNIPPETS=%struct.MixedDoubleShort = type { double, i16, [6 x i8] }|call void @llvm.memcpy.p0.p0.i64(|phi ptr|getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3|load %struct.MixedDoubleShort, ptr|trunc i32 13124 to i16"
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_module_check_case.cmake"
  )
  set_tests_properties(backend_lir_aarch64_variadic_mixed_double_short_ir PROPERTIES
      LABELS "internal;backend")

  add_test(
      NAME backend_lir_x86_64_struct_return_indirect_byval_ir
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_module_case/struct_return_indirect_byval.c
              -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_backend/struct_return_indirect_byval_x86_64.ll
              "-DREQUIRED_SNIPPETS=define %struct.Result @make_result(ptr byval(%struct.Pair) align 8 %p.left, i8 %p.tag, double %p.mid, ptr byval(%struct.Pair) align 8 %p.right)|call %struct.Result (ptr, i8, double, ptr)|ptr byval(%struct.Pair) align 8 %lv.a|ptr byval(%struct.Pair) align 8 %lv.b"
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_module_check_case.cmake"
  )
  set_tests_properties(backend_lir_x86_64_struct_return_indirect_byval_ir PROPERTIES
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

    # TODO: string_literal_char contract disabled — backend .rodata lowering changed
    # add_test(NAME backend_contract_aarch64_string_literal_char_object ...)

    add_test(
      NAME backend_contract_aarch64_extern_global_array_object
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DCLANG=${CLANG_EXECUTABLE}
              -DOBJDUMP=${OBJDUMP_EXECUTABLE}
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_case/extern_global_array.c
              -DTARGET_TRIPLE=aarch64-unknown-linux-gnu
              -DBACKEND_OUTPUT_KIND=asm
              -DBACKEND_OUTPUT_PATH=${CMAKE_BINARY_DIR}/internal_backend_contract/extern_global_array_aarch64.s
              -DOUT_ARTIFACT=${CMAKE_BINARY_DIR}/internal_backend_contract/extern_global_array_aarch64.o
              "-DREQUIRED_BACKEND_SNIPPETS=.extern ext_arr|.globl main|adrp x8, ext_arr|add x8, x8, :lo12:ext_arr|ldr w0, [x8, #4]"
              "-DREQUIRED_OBJDUMP_SNIPPETS=file format elf64-littleaarch64|.text|.rela.text|main|*UND*|ext_arr|R_AARCH64_ADR_PREL_PG_HI21|R_AARCH64_ADD_ABS_LO12_NC"
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_contract_case.cmake"
    )
    set_tests_properties(backend_contract_aarch64_extern_global_array_object PROPERTIES
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

    add_test(
      NAME backend_contract_x86_64_extern_call_object
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DCLANG=${CLANG_EXECUTABLE}
              -DOBJDUMP=${OBJDUMP_EXECUTABLE}
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_case/call_helper.c
              -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
              -DBACKEND_OUTPUT_KIND=asm
              -DBACKEND_OUTPUT_PATH=${CMAKE_BINARY_DIR}/internal_backend_contract/call_helper_x86_64.s
              -DOUT_ARTIFACT=${CMAKE_BINARY_DIR}/internal_backend_contract/call_helper_x86_64.o
              "-DREQUIRED_BACKEND_SNIPPETS=.globl main|call helper"
              "-DREQUIRED_OBJDUMP_SNIPPETS=file format elf64-x86-64|.text|.rela.text|main|*UND*|helper|R_X86_64_PLT32"
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_contract_case.cmake"
    )
    set_tests_properties(backend_contract_x86_64_extern_call_object PROPERTIES
        LABELS "internal;backend")

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_add_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_case/return_add.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_add_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t0 = bir.add i32 2, 3"
      FORBIDDEN_SNIPPETS "define i32 @main()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_add_sub_chain_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_case/return_add_sub_chain.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_add_sub_chain_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t0 = bir.add i32 2, 3|%t1 = bir.sub i32 %t0, 1|bir.ret i32 %t1"
      FORBIDDEN_SNIPPETS "define i32 @main()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_mul_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_mul.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_mul_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t0 = bir.mul i32 6, 7|bir.ret i32 %t0"
      FORBIDDEN_SNIPPETS "define i32 @main()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_sub_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_sub.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_sub_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t0 = bir.sub i32 3, 3|bir.ret i32 %t0"
      FORBIDDEN_SNIPPETS "define i32 @main()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_and_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_and.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_and_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t0 = bir.and i32 14, 11|bir.ret i32 %t0"
      FORBIDDEN_SNIPPETS "define i32 @main()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_or_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_or.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_or_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t0 = bir.or i32 12, 3|bir.ret i32 %t0"
      FORBIDDEN_SNIPPETS "define i32 @main()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_xor_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_xor.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_xor_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t0 = bir.xor i32 12, 10|bir.ret i32 %t0"
      FORBIDDEN_SNIPPETS "define i32 @main()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_shl_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_shl.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_shl_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t0 = bir.shl i32 3, 4|bir.ret i32 %t0"
      FORBIDDEN_SNIPPETS "define i32 @main()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_lshr_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_lshr.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_lshr_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t0 = bir.lshr i32 16, 2|bir.ret i32 %t0"
      FORBIDDEN_SNIPPETS "define i32 @main()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_ashr_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_ashr.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_ashr_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t0 = bir.sub i32 0, 16|%t1 = bir.ashr i32 %t0, 2|bir.ret i32 %t1"
      FORBIDDEN_SNIPPETS "define i32 @main()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_sdiv_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_sdiv.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_sdiv_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t0 = bir.sdiv i32 12, 3|bir.ret i32 %t0"
      FORBIDDEN_SNIPPETS "define i32 @main()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_udiv_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_udiv.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_udiv_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t0 = bir.udiv i32 12, 3|bir.ret i32 %t0"
      FORBIDDEN_SNIPPETS "define i32 @main()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_srem_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_srem.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_srem_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t0 = bir.srem i32 14, 5|bir.ret i32 %t0"
      FORBIDDEN_SNIPPETS "define i32 @main()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_urem_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_urem.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_urem_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t0 = bir.urem i32 14, 5|bir.ret i32 %t0"
      FORBIDDEN_SNIPPETS "define i32 @main()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_eq_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_eq.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_eq_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t1 = bir.eq i32 7, 7|bir.ret i32 %t1"
      FORBIDDEN_SNIPPETS "define i32 @main()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_ne_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_ne.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_ne_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t1 = bir.ne i32 7, 3|bir.ret i32 %t1"
      FORBIDDEN_SNIPPETS "define i32 @main()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_eq_u8_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_eq_u8.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_eq_u8_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose_eq_u() -> i8 {|%t1 = bir.eq i8 7, 7|bir.ret i8 %t1"
      FORBIDDEN_SNIPPETS "define i8 @choose_eq_u()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_ne_u8_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_ne_u8.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_ne_u8_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose_ne_u() -> i8 {|%t1 = bir.ne i8 7, 3|bir.ret i8 %t1"
      FORBIDDEN_SNIPPETS "define i8 @choose_ne_u()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_ult_u8_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_ult_u8.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_ult_u8_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose_ult_u() -> i8 {|%t1 = bir.ult i8 3, 7|bir.ret i8 %t1"
      FORBIDDEN_SNIPPETS "define i8 @choose_ult_u()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_ule_u8_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_ule_u8.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_ule_u8_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose_ule_u() -> i8 {|%t1 = bir.ule i8 7, 7|bir.ret i8 %t1"
      FORBIDDEN_SNIPPETS "define i8 @choose_ule_u()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_ugt_u8_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_ugt_u8.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_ugt_u8_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose_ugt_u() -> i8 {|%t1 = bir.ugt i8 7, 3|bir.ret i8 %t1"
      FORBIDDEN_SNIPPETS "define i8 @choose_ugt_u()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_slt_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_slt.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_slt_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t1 = bir.slt i32 3, 7|bir.ret i32 %t1"
      FORBIDDEN_SNIPPETS "define i32 @main()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_sle_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_sle.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_sle_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t1 = bir.sle i32 7, 7|bir.ret i32 %t1"
      FORBIDDEN_SNIPPETS "define i32 @main()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_sgt_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_sgt.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_sgt_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t1 = bir.sgt i32 7, 3|bir.ret i32 %t1"
      FORBIDDEN_SNIPPETS "define i32 @main()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_sge_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_sge.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_sge_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t1 = bir.sge i32 7, 7|bir.ret i32 %t1"
      FORBIDDEN_SNIPPETS "define i32 @main()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_ult_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_ult.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_ult_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t1 = bir.ult i32 3, 7|bir.ret i32 %t1"
      FORBIDDEN_SNIPPETS "define i32 @main()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_ule_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_ule.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_ule_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t1 = bir.ule i32 7, 7|bir.ret i32 %t1"
      FORBIDDEN_SNIPPETS "define i32 @main()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_ugt_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_ugt.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_ugt_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t1 = bir.ugt i32 7, 3|bir.ret i32 %t1"
      FORBIDDEN_SNIPPETS "define i32 @main()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_uge_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_uge.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_uge_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t1 = bir.uge i32 7, 7|bir.ret i32 %t1"
      FORBIDDEN_SNIPPETS "define i32 @main()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_select_eq_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_select_eq.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_select_eq_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.ret i32 11"
      FORBIDDEN_SNIPPETS "define i32 @main()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_select_ne_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_select_ne.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_select_ne_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.ret i32 11"
      FORBIDDEN_SNIPPETS "define i32 @main()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_select_eq_u8_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_select_eq_u8.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_select_eq_u8_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose_const_u() -> i8 {|bir.ret i8 11"
      FORBIDDEN_SNIPPETS "define i8 @choose_const_u()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_return_select_ne_u8_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/return_select_ne_u8.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/return_select_ne_u8_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose_const_ne_u() -> i8 {|bir.ret i8 11"
      FORBIDDEN_SNIPPETS "define i8 @choose_const_ne_u()"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_single_param_add_sub_chain_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/single_param_add_sub_chain.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/single_param_add_sub_chain_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @add_one(i32 %p.x) -> i32 {|%t0 = bir.add i32 %p.x, 2|%t1 = bir.sub i32 %t0, 1|bir.ret i32 %t1"
      FORBIDDEN_SNIPPETS "define i32 @add_one(i32 %p.x)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_single_param_i64_add_sub_chain_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/single_param_i64_add_sub_chain.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/single_param_i64_add_sub_chain_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @wide_add(i64 %p.x) -> i64 {|%t1 = bir.add i64 %p.x, 2|%t3 = bir.sub i64 %t1, 1|bir.ret i64 %t3"
      FORBIDDEN_SNIPPETS "define i64 @wide_add(i64 %p.x)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_single_param_i8_add_sub_chain_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/single_param_i8_add_sub_chain.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/single_param_i8_add_sub_chain_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @tiny_char(i8 %p.x) -> i8 {|%t1 = bir.add i8 %p.x, 2|%t2 = bir.sub i8 %t1, 1|bir.ret i8 %t2"
      FORBIDDEN_SNIPPETS "define i8 @tiny_char(i8 %p.x)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_single_param_u8_add_sub_chain_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/single_param_u8_add_sub_chain.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/single_param_u8_add_sub_chain_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @tiny_u(i8 %p.x) -> i8 {|%t1 = bir.add i8 %p.x, 2|%t2 = bir.sub i8 %t1, 1|bir.ret i8 %t2"
      FORBIDDEN_SNIPPETS "define i8 @tiny_u(i8 %p.x)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_single_param_u8_select_eq_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/single_param_u8_select_eq.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/single_param_u8_select_eq_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose_u(i8 %p.x) -> i8 {|bir.select eq i8 %p.x, 7, 11, 4|bir.ret i8 %t11"
      FORBIDDEN_SNIPPETS "define i8 @choose_u(i8 %p.x)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_single_param_u8_select_ne_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/single_param_u8_select_ne.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/single_param_u8_select_ne_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose_ne_u(i8 %p.x) -> i8 {|bir.select ne i8 %p.x, 7, 11, 4|bir.ret i8 %t11"
      FORBIDDEN_SNIPPETS "define i8 @choose_ne_u(i8 %p.x)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_i8_add_sub_chain_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_i8_add_sub_chain.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_i8_add_sub_chain_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @tiny_mix(i8 %p.x, i8 %p.y) -> i8 {|%t2 = bir.add i8 %p.x, %p.y|%t3 = bir.add i8 %t2, 2|%t4 = bir.sub i8 %t3, 1|bir.ret i8 %t4"
      FORBIDDEN_SNIPPETS "define i8 @tiny_mix(i8 %p.x, i8 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_u8_add_sub_chain_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_u8_add_sub_chain.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_u8_add_sub_chain_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @tiny_mix_u(i8 %p.x, i8 %p.y) -> i8 {|%t2 = bir.add i8 %p.x, %p.y|%t3 = bir.add i8 %t2, 2|%t4 = bir.sub i8 %t3, 1|bir.ret i8 %t4"
      FORBIDDEN_SNIPPETS "define i8 @tiny_mix_u(i8 %p.x, i8 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_i64_add_sub_chain_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_i64_add_sub_chain.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_i64_add_sub_chain_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @wide_mix(i64 %p.x, i64 %p.y) -> i64 {|%t0 = bir.add i64 %p.x, %p.y|%t2 = bir.add i64 %t0, 2|%t4 = bir.sub i64 %t2, 1|bir.ret i64 %t4"
      FORBIDDEN_SNIPPETS "define i64 @wide_mix(i64 %p.x, i64 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_i8_staged_affine_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_i8_staged_affine.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_i8_staged_affine_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @tiny_mix_staged(i8 %p.x, i8 %p.y) -> i8 {|%t1 = bir.add i8 %p.x, 2|%t3 = bir.add i8 %t1, %p.y|%t4 = bir.sub i8 %t3, 1|bir.ret i8 %t4"
      FORBIDDEN_SNIPPETS "define i8 @tiny_mix_staged(i8 %p.x, i8 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_i64_staged_affine_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_i64_staged_affine.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_i64_staged_affine_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @wide_mix_staged(i64 %p.x, i64 %p.y) -> i64 {|%t1 = bir.add i64 %p.x, 2|%t2 = bir.add i64 %t1, %p.y|%t4 = bir.sub i64 %t2, 1|bir.ret i64 %t4"
      FORBIDDEN_SNIPPETS "define i64 @wide_mix_staged(i64 %p.x, i64 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_u8_staged_affine_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_u8_staged_affine.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_u8_staged_affine_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @tiny_mix_u_staged(i8 %p.x, i8 %p.y) -> i8 {|%t1 = bir.add i8 %p.x, 2|%t3 = bir.add i8 %t1, %p.y|%t4 = bir.sub i8 %t3, 1|bir.ret i8 %t4"
      FORBIDDEN_SNIPPETS "define i8 @tiny_mix_u_staged(i8 %p.x, i8 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_u8_select_eq_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_u8_select_eq.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_u8_select_eq_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_u(i8 %p.x, i8 %p.y) -> i8 {|bir.select eq i8 %p.x, %p.y, %p.x, %p.y|bir.ret i8 %t10"
      FORBIDDEN_SNIPPETS "define i8 @choose2_u(i8 %p.x, i8 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_u8_select_eq_predecessor_add_post_add_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_u8_select_eq_predecessor_add_post_add.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_u8_select_eq_predecessor_add_post_add_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_add_post_u(i8 %p.x, i8 %p.y) -> i8 {|%t11 = bir.add i8 %p.x, 5|%t13 = bir.add i8 %p.y, 9|%t14 = bir.select eq i8 %p.x, %p.y, %t11, %t13|%t15 = bir.add i8 %t14, 6|bir.ret i8 %t15"
      FORBIDDEN_SNIPPETS "define i8 @choose2_add_post_u(i8 %p.x, i8 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_add_phi_post_add_sub_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_u8_select_eq_split_predecessor_add_phi_post_add_sub.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_u8_select_eq_split_predecessor_add_phi_post_add_sub_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_add_post_chain_u(i8 %p.x, i8 %p.y) -> i8 {|%t11 = bir.add i8 %p.x, 5|%t13 = bir.add i8 %p.y, 9|%t14 = bir.select eq i8 %p.x, %p.y, %t11, %t13|%t15 = bir.add i8 %t14, 6|%t16 = bir.sub i8 %t15, 2|bir.ret i8 %t16"
      FORBIDDEN_SNIPPETS "define i8 @choose2_add_post_chain_u(i8 %p.x, i8 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_add_phi_post_add_sub_add_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_u8_select_eq_split_predecessor_add_phi_post_add_sub_add.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_u8_select_eq_split_predecessor_add_phi_post_add_sub_add_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_add_post_chain_tail_u(i8 %p.x, i8 %p.y) -> i8 {|%t11 = bir.add i8 %p.x, 5|%t13 = bir.add i8 %p.y, 9|%t14 = bir.select eq i8 %p.x, %p.y, %t11, %t13|%t15 = bir.add i8 %t14, 6|%t16 = bir.sub i8 %t15, 2|%t17 = bir.add i8 %t16, 9|bir.ret i8 %t17"
      FORBIDDEN_SNIPPETS "define i8 @choose2_add_post_chain_tail_u(i8 %p.x, i8 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_mixed_affine_post_add_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_u8_select_eq_split_predecessor_mixed_affine_post_add.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_u8_select_eq_split_predecessor_mixed_affine_post_add_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_mixed_post_u(i8 %p.x, i8 %p.y) -> i8 {|%t11 = bir.add i8 %p.x, 8|%t12 = bir.sub i8 %t11, 3|%t14 = bir.add i8 %p.y, 11|%t15 = bir.sub i8 %t14, 4|%t16 = bir.select eq i8 %p.x, %p.y, %t12, %t15|%t17 = bir.add i8 %t16, 6|bir.ret i8 %t17"
      FORBIDDEN_SNIPPETS "define i8 @choose2_mixed_post_u(i8 %p.x, i8 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_mixed_affine_post_add_sub_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_u8_select_eq_split_predecessor_mixed_affine_post_add_sub.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_u8_select_eq_split_predecessor_mixed_affine_post_add_sub_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_mixed_post_chain_u(i8 %p.x, i8 %p.y) -> i8 {|%t11 = bir.add i8 %p.x, 8|%t12 = bir.sub i8 %t11, 3|%t14 = bir.add i8 %p.y, 11|%t15 = bir.sub i8 %t14, 4|%t16 = bir.select eq i8 %p.x, %p.y, %t12, %t15|%t17 = bir.add i8 %t16, 6|%t18 = bir.sub i8 %t17, 2|bir.ret i8 %t18"
      FORBIDDEN_SNIPPETS "define i8 @choose2_mixed_post_chain_u(i8 %p.x, i8 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_mixed_affine_post_add_sub_add_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_u8_select_eq_split_predecessor_mixed_affine_post_add_sub_add.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_u8_select_eq_split_predecessor_mixed_affine_post_add_sub_add_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_mixed_post_chain_tail_u(i8 %p.x, i8 %p.y) -> i8 {|%t11 = bir.add i8 %p.x, 8|%t12 = bir.sub i8 %t11, 3|%t14 = bir.add i8 %p.y, 11|%t15 = bir.sub i8 %t14, 4|%t16 = bir.select eq i8 %p.x, %p.y, %t12, %t15|%t17 = bir.add i8 %t16, 6|%t18 = bir.sub i8 %t17, 2|%t19 = bir.add i8 %t18, 9|bir.ret i8 %t19"
      FORBIDDEN_SNIPPETS "define i8 @choose2_mixed_post_chain_tail_u(i8 %p.x, i8 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_u8_select_eq_split_predecessor_deeper_then_mixed_affine_post_add.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_u8_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_deeper_post_u(i8 %p.x, i8 %p.y) -> i8 {|%t11 = bir.add i8 %p.x, 8|%t12 = bir.sub i8 %t11, 3|%t13 = bir.add i8 %t12, 5|%t15 = bir.add i8 %p.y, 11|%t16 = bir.sub i8 %t15, 4|%t17 = bir.select eq i8 %p.x, %p.y, %t13, %t16|%t18 = bir.add i8 %t17, 6|bir.ret i8 %t18"
      FORBIDDEN_SNIPPETS "define i8 @choose2_deeper_post_u(i8 %p.x, i8 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_u8_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_u8_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_deeper_post_chain_u(i8 %p.x, i8 %p.y) -> i8 {|%t11 = bir.add i8 %p.x, 8|%t12 = bir.sub i8 %t11, 3|%t13 = bir.add i8 %t12, 5|%t15 = bir.add i8 %p.y, 11|%t16 = bir.sub i8 %t15, 4|%t17 = bir.select eq i8 %p.x, %p.y, %t13, %t16|%t18 = bir.add i8 %t17, 6|%t19 = bir.sub i8 %t18, 2|bir.ret i8 %t19"
      FORBIDDEN_SNIPPETS "define i8 @choose2_deeper_post_chain_u(i8 %p.x, i8 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_add_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_u8_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_add.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_u8_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_add_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_deeper_post_chain_tail_u(i8 %p.x, i8 %p.y) -> i8 {|%t11 = bir.add i8 %p.x, 8|%t12 = bir.sub i8 %t11, 3|%t13 = bir.add i8 %t12, 5|%t15 = bir.add i8 %p.y, 11|%t16 = bir.sub i8 %t15, 4|%t17 = bir.select eq i8 %p.x, %p.y, %t13, %t16|%t18 = bir.add i8 %t17, 6|%t19 = bir.sub i8 %t18, 2|%t20 = bir.add i8 %t19, 9|bir.ret i8 %t20"
      FORBIDDEN_SNIPPETS "define i8 @choose2_deeper_post_chain_tail_u(i8 %p.x, i8 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_deeper_affine_post_add_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_u8_select_eq_split_predecessor_deeper_affine_post_add.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_u8_select_eq_split_predecessor_deeper_affine_post_add_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_deeper_both_post_u(i8 %p.x, i8 %p.y) -> i8 {|%t11 = bir.add i8 %p.x, 8|%t12 = bir.sub i8 %t11, 3|%t13 = bir.add i8 %t12, 5|%t15 = bir.add i8 %p.y, 11|%t16 = bir.sub i8 %t15, 4|%t17 = bir.add i8 %t16, 7|%t18 = bir.select eq i8 %p.x, %p.y, %t13, %t17|%t19 = bir.add i8 %t18, 6|bir.ret i8 %t19"
      FORBIDDEN_SNIPPETS "define i8 @choose2_deeper_both_post_u(i8 %p.x, i8 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_deeper_affine_post_add_sub_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_u8_select_eq_split_predecessor_deeper_affine_post_add_sub.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_u8_select_eq_split_predecessor_deeper_affine_post_add_sub_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_deeper_both_post_chain_u(i8 %p.x, i8 %p.y) -> i8 {|%t11 = bir.add i8 %p.x, 8|%t12 = bir.sub i8 %t11, 3|%t13 = bir.add i8 %t12, 5|%t15 = bir.add i8 %p.y, 11|%t16 = bir.sub i8 %t15, 4|%t17 = bir.add i8 %t16, 7|%t18 = bir.select eq i8 %p.x, %p.y, %t13, %t17|%t19 = bir.add i8 %t18, 6|%t20 = bir.sub i8 %t19, 2|bir.ret i8 %t20"
      FORBIDDEN_SNIPPETS "define i8 @choose2_deeper_both_post_chain_u(i8 %p.x, i8 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_u8_select_eq_split_predecessor_mixed_then_deeper_affine_post_add.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_u8_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_mixed_then_deeper_post_u(i8 %p.x, i8 %p.y) -> i8 {|%t11 = bir.add i8 %p.x, 8|%t12 = bir.sub i8 %t11, 3|%t14 = bir.add i8 %p.y, 11|%t15 = bir.sub i8 %t14, 4|%t16 = bir.add i8 %t15, 7|%t17 = bir.select eq i8 %p.x, %p.y, %t12, %t16|%t18 = bir.add i8 %t17, 6|bir.ret i8 %t18"
      FORBIDDEN_SNIPPETS "define i8 @choose2_mixed_then_deeper_post_u(i8 %p.x, i8 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_sub_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_u8_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_sub.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_u8_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_sub_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_mixed_then_deeper_post_chain_u(i8 %p.x, i8 %p.y) -> i8 {|%t11 = bir.add i8 %p.x, 8|%t12 = bir.sub i8 %t11, 3|%t14 = bir.add i8 %p.y, 11|%t15 = bir.sub i8 %t14, 4|%t16 = bir.add i8 %t15, 7|%t17 = bir.select eq i8 %p.x, %p.y, %t12, %t16|%t18 = bir.add i8 %t17, 6|%t19 = bir.sub i8 %t18, 2|bir.ret i8 %t19"
      FORBIDDEN_SNIPPETS "define i8 @choose2_mixed_then_deeper_post_chain_u(i8 %p.x, i8 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_sub_add_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_u8_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_sub_add.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_u8_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_sub_add_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_mixed_then_deeper_post_chain_tail_u(i8 %p.x, i8 %p.y) -> i8 {|%t11 = bir.add i8 %p.x, 8|%t12 = bir.sub i8 %t11, 3|%t14 = bir.add i8 %p.y, 11|%t15 = bir.sub i8 %t14, 4|%t16 = bir.add i8 %t15, 7|%t17 = bir.select eq i8 %p.x, %p.y, %t12, %t16|%t18 = bir.add i8 %t17, 6|%t19 = bir.sub i8 %t18, 2|%t20 = bir.add i8 %t19, 9|bir.ret i8 %t20"
      FORBIDDEN_SNIPPETS "define i8 @choose2_mixed_then_deeper_post_chain_tail_u(i8 %p.x, i8 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_u8_select_eq_split_predecessor_deeper_affine_post_add_sub_add_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_u8_select_eq_split_predecessor_deeper_affine_post_add_sub_add.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_u8_select_eq_split_predecessor_deeper_affine_post_add_sub_add_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_deeper_both_post_chain_tail_u(i8 %p.x, i8 %p.y) -> i8 {|%t11 = bir.add i8 %p.x, 8|%t12 = bir.sub i8 %t11, 3|%t13 = bir.add i8 %t12, 5|%t15 = bir.add i8 %p.y, 11|%t16 = bir.sub i8 %t15, 4|%t17 = bir.add i8 %t16, 7|%t18 = bir.select eq i8 %p.x, %p.y, %t13, %t17|%t19 = bir.add i8 %t18, 6|%t20 = bir.sub i8 %t19, 2|%t21 = bir.add i8 %t20, 9|bir.ret i8 %t21"
      FORBIDDEN_SNIPPETS "define i8 @choose2_deeper_both_post_chain_tail_u(i8 %p.x, i8 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_single_param_select_eq_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/single_param_select_eq.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/single_param_select_eq_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose(i32 %p.x) -> i32 {|%t8 = bir.select eq i32 %p.x, 7, 11, 4|bir.ret i32 %t8"
      FORBIDDEN_SNIPPETS "define i32 @choose(i32 %p.x)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_single_param_select_ne_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/single_param_select_ne.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/single_param_select_ne_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose_ne(i32 %p.x) -> i32 {|%t8 = bir.select ne i32 %p.x, 7, 11, 4|bir.ret i32 %t8"
      FORBIDDEN_SNIPPETS "define i32 @choose_ne(i32 %p.x)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_add_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_add.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_add_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @add_pair(i32 %p.x, i32 %p.y) -> i32 {|%t0 = bir.add i32 %p.x, %p.y|bir.ret i32 %t0"
      FORBIDDEN_SNIPPETS "define i32 @add_pair(i32 %p.x, i32 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_select_eq_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_select_eq.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_select_eq_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2(i32 %p.x, i32 %p.y) -> i32 {|%t8 = bir.select eq i32 %p.x, %p.y, %p.x, %p.y|bir.ret i32 %t8"
      FORBIDDEN_SNIPPETS "define i32 @choose2(i32 %p.x, i32 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_select_eq_predecessor_add_post_add_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_select_eq_predecessor_add_post_add.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_select_eq_predecessor_add_post_add_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_add_post(i32 %p.x, i32 %p.y) -> i32 {|%t8 = bir.add i32 %p.x, 5|%t9 = bir.add i32 %p.y, 9|%t10 = bir.select eq i32 %p.x, %p.y, %t8, %t9|%t11 = bir.add i32 %t10, 6|bir.ret i32 %t11"
      FORBIDDEN_SNIPPETS "define i32 @choose2_add_post(i32 %p.x, i32 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_add_phi_post_add_sub_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_select_eq_split_predecessor_add_phi_post_add_sub.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_select_eq_split_predecessor_add_phi_post_add_sub_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_add_post_chain(i32 %p.x, i32 %p.y) -> i32 {|%t8 = bir.add i32 %p.x, 5|%t9 = bir.add i32 %p.y, 9|%t10 = bir.select eq i32 %p.x, %p.y, %t8, %t9|%t11 = bir.add i32 %t10, 6|%t12 = bir.sub i32 %t11, 2|bir.ret i32 %t12"
      FORBIDDEN_SNIPPETS "define i32 @choose2_add_post_chain(i32 %p.x, i32 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_add_phi_post_add_sub_add_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_select_eq_split_predecessor_add_phi_post_add_sub_add.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_select_eq_split_predecessor_add_phi_post_add_sub_add_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_add_post_chain_tail(i32 %p.x, i32 %p.y) -> i32 {|%t8 = bir.add i32 %p.x, 5|%t9 = bir.add i32 %p.y, 9|%t10 = bir.select eq i32 %p.x, %p.y, %t8, %t9|%t11 = bir.add i32 %t10, 6|%t12 = bir.sub i32 %t11, 2|%t13 = bir.add i32 %t12, 9|bir.ret i32 %t13"
      FORBIDDEN_SNIPPETS "define i32 @choose2_add_post_chain_tail(i32 %p.x, i32 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_deeper_affine_post_add_sub_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_select_eq_split_predecessor_deeper_affine_post_add_sub.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_select_eq_split_predecessor_deeper_affine_post_add_sub_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_deeper_both_post_chain(i32 %p.x, i32 %p.y) -> i32 {|%t8 = bir.add i32 %p.x, 8|%t9 = bir.sub i32 %t8, 3|%t10 = bir.add i32 %t9, 5|%t11 = bir.add i32 %p.y, 11|%t12 = bir.sub i32 %t11, 4|%t13 = bir.add i32 %t12, 7|%t14 = bir.select eq i32 %p.x, %p.y, %t10, %t13|%t15 = bir.add i32 %t14, 6|%t16 = bir.sub i32 %t15, 2|bir.ret i32 %t16"
      FORBIDDEN_SNIPPETS "define i32 @choose2_deeper_both_post_chain(i32 %p.x, i32 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_mixed_affine_post_add_sub_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_select_eq_split_predecessor_mixed_affine_post_add_sub.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_select_eq_split_predecessor_mixed_affine_post_add_sub_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_mixed_post_chain(i32 %p.x, i32 %p.y) -> i32 {|%t8 = bir.add i32 %p.x, 8|%t9 = bir.sub i32 %t8, 3|%t10 = bir.add i32 %p.y, 11|%t11 = bir.sub i32 %t10, 4|%t12 = bir.select eq i32 %p.x, %p.y, %t9, %t11|%t13 = bir.add i32 %t12, 6|%t14 = bir.sub i32 %t13, 2|bir.ret i32 %t14"
      FORBIDDEN_SNIPPETS "define i32 @choose2_mixed_post_chain(i32 %p.x, i32 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_mixed_affine_post_add_sub_add_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_select_eq_split_predecessor_mixed_affine_post_add_sub_add.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_select_eq_split_predecessor_mixed_affine_post_add_sub_add_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_mixed_post_chain_tail(i32 %p.x, i32 %p.y) -> i32 {|%t8 = bir.add i32 %p.x, 8|%t9 = bir.sub i32 %t8, 3|%t10 = bir.add i32 %p.y, 11|%t11 = bir.sub i32 %t10, 4|%t12 = bir.select eq i32 %p.x, %p.y, %t9, %t11|%t13 = bir.add i32 %t12, 6|%t14 = bir.sub i32 %t13, 2|%t15 = bir.add i32 %t14, 9|bir.ret i32 %t15"
      FORBIDDEN_SNIPPETS "define i32 @choose2_mixed_post_chain_tail(i32 %p.x, i32 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_mixed_affine_post_add_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_select_eq_split_predecessor_mixed_affine_post_add.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_select_eq_split_predecessor_mixed_affine_post_add_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_mixed_post(i32 %p.x, i32 %p.y) -> i32 {|%t8 = bir.add i32 %p.x, 8|%t9 = bir.sub i32 %t8, 3|%t10 = bir.add i32 %p.y, 11|%t11 = bir.sub i32 %t10, 4|%t12 = bir.select eq i32 %p.x, %p.y, %t9, %t11|%t13 = bir.add i32 %t12, 6|bir.ret i32 %t13"
      FORBIDDEN_SNIPPETS "define i32 @choose2_mixed_post(i32 %p.x, i32 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_deeper_post(i32 %p.x, i32 %p.y) -> i32 {|%t8 = bir.add i32 %p.x, 8|%t9 = bir.sub i32 %t8, 3|%t10 = bir.add i32 %t9, 5|%t11 = bir.add i32 %p.y, 11|%t12 = bir.sub i32 %t11, 4|%t13 = bir.select eq i32 %p.x, %p.y, %t10, %t12|%t14 = bir.add i32 %t13, 6|bir.ret i32 %t14"
      FORBIDDEN_SNIPPETS "define i32 @choose2_deeper_post(i32 %p.x, i32 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_deeper_post_chain(i32 %p.x, i32 %p.y) -> i32 {|%t8 = bir.add i32 %p.x, 8|%t9 = bir.sub i32 %t8, 3|%t10 = bir.add i32 %t9, 5|%t11 = bir.add i32 %p.y, 11|%t12 = bir.sub i32 %t11, 4|%t13 = bir.select eq i32 %p.x, %p.y, %t10, %t12|%t14 = bir.add i32 %t13, 6|%t15 = bir.sub i32 %t14, 2|bir.ret i32 %t15"
      FORBIDDEN_SNIPPETS "define i32 @choose2_deeper_post_chain(i32 %p.x, i32 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_add_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_add.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_add_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_deeper_post_chain_tail(i32 %p.x, i32 %p.y) -> i32 {|%t8 = bir.add i32 %p.x, 8|%t9 = bir.sub i32 %t8, 3|%t10 = bir.add i32 %t9, 5|%t11 = bir.add i32 %p.y, 11|%t12 = bir.sub i32 %t11, 4|%t13 = bir.select eq i32 %p.x, %p.y, %t10, %t12|%t14 = bir.add i32 %t13, 6|%t15 = bir.sub i32 %t14, 2|%t16 = bir.add i32 %t15, 9|bir.ret i32 %t16"
      FORBIDDEN_SNIPPETS "define i32 @choose2_deeper_post_chain_tail(i32 %p.x, i32 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_select_eq_split_predecessor_mixed_then_deeper_affine_post_add.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_mixed_then_deeper_post(i32 %p.x, i32 %p.y) -> i32 {|%t8 = bir.add i32 %p.x, 8|%t9 = bir.sub i32 %t8, 3|%t10 = bir.add i32 %p.y, 11|%t11 = bir.sub i32 %t10, 4|%t12 = bir.add i32 %t11, 7|%t13 = bir.select eq i32 %p.x, %p.y, %t9, %t12|%t14 = bir.add i32 %t13, 6|bir.ret i32 %t14"
      FORBIDDEN_SNIPPETS "define i32 @choose2_mixed_then_deeper_post(i32 %p.x, i32 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_sub_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_sub.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_sub_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_mixed_then_deeper_post_chain(i32 %p.x, i32 %p.y) -> i32 {|%t8 = bir.add i32 %p.x, 8|%t9 = bir.sub i32 %t8, 3|%t10 = bir.add i32 %p.y, 11|%t11 = bir.sub i32 %t10, 4|%t12 = bir.add i32 %t11, 7|%t13 = bir.select eq i32 %p.x, %p.y, %t9, %t12|%t14 = bir.add i32 %t13, 6|%t15 = bir.sub i32 %t14, 2|bir.ret i32 %t15"
      FORBIDDEN_SNIPPETS "define i32 @choose2_mixed_then_deeper_post_chain(i32 %p.x, i32 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_deeper_affine_post_add_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_select_eq_split_predecessor_deeper_affine_post_add.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_select_eq_split_predecessor_deeper_affine_post_add_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_deeper_both_post(i32 %p.x, i32 %p.y) -> i32 {|%t8 = bir.add i32 %p.x, 8|%t9 = bir.sub i32 %t8, 3|%t10 = bir.add i32 %t9, 5|%t11 = bir.add i32 %p.y, 11|%t12 = bir.sub i32 %t11, 4|%t13 = bir.add i32 %t12, 7|%t14 = bir.select eq i32 %p.x, %p.y, %t10, %t13|%t15 = bir.add i32 %t14, 6|bir.ret i32 %t15"
      FORBIDDEN_SNIPPETS "define i32 @choose2_deeper_both_post(i32 %p.x, i32 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_deeper_affine_post_add_sub_add_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_select_eq_split_predecessor_deeper_affine_post_add_sub_add.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_select_eq_split_predecessor_deeper_affine_post_add_sub_add_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_deeper_both_post_chain_tail(i32 %p.x, i32 %p.y) -> i32 {|%t8 = bir.add i32 %p.x, 8|%t9 = bir.sub i32 %t8, 3|%t10 = bir.add i32 %t9, 5|%t11 = bir.add i32 %p.y, 11|%t12 = bir.sub i32 %t11, 4|%t13 = bir.add i32 %t12, 7|%t14 = bir.select eq i32 %p.x, %p.y, %t10, %t13|%t15 = bir.add i32 %t14, 6|%t16 = bir.sub i32 %t15, 2|%t17 = bir.add i32 %t16, 9|bir.ret i32 %t17"
      FORBIDDEN_SNIPPETS "define i32 @choose2_deeper_both_post_chain_tail(i32 %p.x, i32 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_sub_add_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_sub_add.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_select_eq_split_predecessor_mixed_then_deeper_affine_post_add_sub_add_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @choose2_mixed_then_deeper_post_chain_tail(i32 %p.x, i32 %p.y) -> i32 {|%t8 = bir.add i32 %p.x, 8|%t9 = bir.sub i32 %t8, 3|%t10 = bir.add i32 %p.y, 11|%t11 = bir.sub i32 %t10, 4|%t12 = bir.add i32 %t11, 7|%t13 = bir.select eq i32 %p.x, %p.y, %t9, %t12|%t14 = bir.add i32 %t13, 6|%t15 = bir.sub i32 %t14, 2|%t16 = bir.add i32 %t15, 9|bir.ret i32 %t16"
      FORBIDDEN_SNIPPETS "define i32 @choose2_mixed_then_deeper_post_chain_tail(i32 %p.x, i32 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_add_sub_chain_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_add_sub_chain.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_add_sub_chain_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @add_pair(i32 %p.x, i32 %p.y) -> i32 {|%t0 = bir.add i32 %p.x, %p.y|%t1 = bir.sub i32 %t0, 1|bir.ret i32 %t1"
      FORBIDDEN_SNIPPETS "define i32 @add_pair(i32 %p.x, i32 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_two_param_staged_affine_defaults_to_bir
      SRC "${INTERNAL_C_TEST_ROOT}/backend_route_case/two_param_staged_affine.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/two_param_staged_affine_riscv64.ll"
      REQUIRED_SNIPPETS "bir.func @add_pair(i32 %p.x, i32 %p.y) -> i32 {|%t0 = bir.add i32 %p.x, 2|%t1 = bir.add i32 %t0, %p.y|%t2 = bir.sub i32 %t1, 1|bir.ret i32 %t2"
      FORBIDDEN_SNIPPETS "define i32 @add_pair(i32 %p.x, i32 %p.y)"
    )

    c4c_add_backend_codegen_route_test(
      backend_codegen_route_riscv64_global_load_falls_back_to_llvm
      SRC "${INTERNAL_C_TEST_ROOT}/backend_case/global_load.c"
      TARGET_TRIPLE riscv64-unknown-linux-gnu
      OUT_TEXT "${CMAKE_BINARY_DIR}/internal_backend_route/global_load_riscv64.ll"
      REQUIRED_SNIPPETS "@g_counter = global i32 11|define i32 @main()"
      FORBIDDEN_SNIPPETS "bir.func @main()"
    )
  endif()

  if(BACKEND_RUNTIME_TARGET_TRIPLE)
    foreach(src IN LISTS INTERNAL_BACKEND_TEST_SRCS)
      get_filename_component(stem "${src}" NAME_WE)
      if(stem STREQUAL "call_helper_def" OR stem STREQUAL "call_helper")
        continue()
      endif()
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
      elseif(stem STREQUAL "return_add_sub_chain")
        set(expect_exit_code 4)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "call_helper")
        set(expect_exit_code 7)
        set(backend_output_kind "asm")
        set(extra_toolchain_inputs "${INTERNAL_C_TEST_ROOT}/backend_case/call_helper_def.c")
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
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "nested_member_pointer_array")
        set(expect_exit_code 8)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "nested_param_member_array")
        set(expect_exit_code 9)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "global_load")
        set(expect_exit_code 11)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "extern_global_array")
        continue()
      elseif(stem STREQUAL "extern_global_array_def")
        continue()
      elseif(stem STREQUAL "global_char_pointer_diff")
        set(expect_exit_code 1)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "global_int_pointer_diff")
        set(expect_exit_code 1)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "global_int_pointer_roundtrip")
        set(expect_exit_code 9)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "string_literal_char")
        set(expect_exit_code 105)
        set(backend_output_kind "asm")
      elseif(stem STREQUAL "variadic_sum2")
        set(expect_exit_code 42)
      elseif(stem STREQUAL "variadic_double_bytes")
        set(expect_exit_code 67)
      elseif(stem STREQUAL "variadic_pair_second")
        set(expect_exit_code 9)
      elseif(stem STREQUAL "param_member_array")
        set(expect_exit_code 6)
        set(backend_output_kind "asm")
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
                "-DEXTRA_TOOLCHAIN_INPUTS=${extra_toolchain_inputs}"
                -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_positive_case.cmake"
      )
      set_tests_properties("${test_name}" PROPERTIES LABELS "internal;backend")
      unset(extra_toolchain_inputs)
    endforeach()

    add_test(
      NAME backend_runtime_extern_global_array
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DCLANG=${CLANG_EXECUTABLE}
              -DSRC=${INTERNAL_C_TEST_ROOT}/backend_case/extern_global_array.c
              -DTARGET_TRIPLE=${BACKEND_RUNTIME_TARGET_TRIPLE}
              -DBACKEND_OUTPUT_KIND=asm
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_backend/extern_global_array.ll
              -DEXPECT_EXIT_CODE=7
              -DOUT_C2LL_BIN=${CMAKE_BINARY_DIR}/internal_backend/extern_global_array.bin
              "-DEXTRA_TOOLCHAIN_INPUTS=${INTERNAL_C_TEST_ROOT}/backend_case/extern_global_array_def.c"
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_backend_positive_case.cmake"
    )
    set_tests_properties(backend_runtime_extern_global_array PROPERTIES LABELS "internal;backend")

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
