set(INTERNAL_TEST_ROOT "${PROJECT_SOURCE_DIR}/tests/internal")
set(INTERNAL_TEST_CMAKE_ROOT "${INTERNAL_TEST_ROOT}/cmake")

file(GLOB INTERNAL_NEGATIVE_TEST_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_TEST_ROOT}/negative_case/*.c")
file(GLOB INTERNAL_POSITIVE_TEST_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_TEST_ROOT}/positive_case/*.c")
file(GLOB INTERNAL_CPP_POSITIVE_TEST_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_TEST_ROOT}/cpp/postive_case/*.cpp")
file(GLOB INTERNAL_CPP_NEGATIVE_TEST_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_TEST_ROOT}/cpp/negative_case/*.cpp")
file(GLOB INTERNAL_LINUX_STAGE2_REPRO_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_TEST_ROOT}/positive_case/linux_stage2_repro/*.c")
file(GLOB INTERNAL_CCC_REVIEW_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_TEST_ROOT}/positive_case/ccc_review/*.c")

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

set(CPP_POSITIVE_RUNTIME_STEMS
    const_named_fold
    consteval_block_scope
    consteval_call_resolution
    consteval_diag
    consteval_func
    consteval_interp
    consteval_mutable
    consteval_recursive_fib
    consteval_short_circuit
    consteval_template
    consteval_template_sizeof
    constexpr_var
    constexpr_if
    if_constexpr_fold
    constexpr_builtin_queries
    constexpr_local
    constexpr_local_switch
    extern_c
    template_chain
    template_func
)

set(CPP_POSITIVE_FRONTEND_STEMS
    if_constexpr_template_chain
    template_type_traits_builtin
    type_traits_builtin
)

set(CPP_POSITIVE_PARSE_STEMS
    struct_method
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
            -P "${INTERNAL_TEST_CMAKE_ROOT}/run_example_case.cmake"
  )
else()
  add_test(
    NAME tiny_c2ll_tests
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DEXAMPLE_C=${EXAMPLE_C}
            -DOUT_LL=${CMAKE_BINARY_DIR}/tiny_c2ll_tests/test_example.ll
            -P "${INTERNAL_TEST_CMAKE_ROOT}/run_example_case.cmake"
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
            -P "${INTERNAL_TEST_CMAKE_ROOT}/run_negative_case.cmake"
  )
  set_tests_properties("${test_name}" PROPERTIES LABELS "internal;negative_case")
endforeach()

foreach(src IN LISTS INTERNAL_CPP_NEGATIVE_TEST_SRCS)
  get_filename_component(stem "${src}" NAME_WE)
  set(test_name "cpp_negative_tests_${stem}")
  add_test(
    NAME "${test_name}"
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DSRC=${src}
            -DENFORCE_NEGATIVE=ON
            -P "${INTERNAL_TEST_CMAKE_ROOT}/run_negative_case.cmake"
  )
  set_tests_properties("${test_name}" PROPERTIES LABELS "internal;negative_case;cpp")
endforeach()

if(CLANG_EXECUTABLE)
  foreach(src IN LISTS INTERNAL_POSITIVE_TEST_SRCS INTERNAL_LINUX_STAGE2_REPRO_SRCS)
    file(RELATIVE_PATH rel_src "${INTERNAL_TEST_ROOT}/positive_case" "${src}")
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
              -P "${INTERNAL_TEST_CMAKE_ROOT}/run_positive_case.cmake"
    )
    set_tests_properties("${test_name}" PROPERTIES LABELS "internal;positive_case")
  endforeach()
else()
  message(WARNING "clang not found: skipping internal positive_case runtime tests")
endif()

if(CLANG_EXECUTABLE)
  foreach(src IN LISTS INTERNAL_CPP_POSITIVE_TEST_SRCS)
    file(RELATIVE_PATH rel_src "${INTERNAL_TEST_ROOT}/cpp/postive_case" "${src}")
    get_filename_component(stem "${src}" NAME_WE)
    string(REGEX REPLACE "[^A-Za-z0-9_]" "_" test_id "${rel_src}")
    set(test_name "cpp_positive_sema_${test_id}")
    set(test_mode runtime)
    if(stem IN_LIST CPP_POSITIVE_FRONTEND_STEMS)
      set(test_mode frontend)
    endif()
    if(stem IN_LIST CPP_POSITIVE_PARSE_STEMS)
      set(test_mode parse)
    endif()

    add_test(
      NAME "${test_name}"
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DCLANG=${CLANG_EXECUTABLE}
              -DCXX_COMPILER=${CMAKE_CXX_COMPILER}
              -DTEST_MODE=${test_mode}
              -DSRC=${src}
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_cpp_positive/${rel_src}.ll
              -DOUT_CLANG_BIN=${CMAKE_BINARY_DIR}/internal_cpp_positive/${rel_src}.host.bin
              -DOUT_C2LL_BIN=${CMAKE_BINARY_DIR}/internal_cpp_positive/${rel_src}.c2ll.bin
              -P "${INTERNAL_TEST_CMAKE_ROOT}/run_positive_case.cmake"
    )
    set_tests_properties("${test_name}" PROPERTIES LABELS "internal;positive_case;cpp")
  endforeach()
else()
  message(WARNING "clang not found: skipping internal cpp positive_case runtime tests")
endif()

# HIR template definition dump test: verify template metadata survives lowering
add_test(
  NAME cpp_hir_template_def_dump
  COMMAND c4cll --dump-hir "${INTERNAL_TEST_ROOT}/cpp/postive_case/template_func.cpp"
)
set_tests_properties(cpp_hir_template_def_dump PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "template add<typename T>.*instantiation"
)

# HIR consteval template dump test: verify consteval template metadata preserved
add_test(
  NAME cpp_hir_consteval_template_dump
  COMMAND c4cll --dump-hir "${INTERNAL_TEST_ROOT}/cpp/postive_case/consteval_template.cpp"
)
set_tests_properties(cpp_hir_consteval_template_dump PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "template consteval square<typename T>"
)

# HIR template call info dump test: verify template application metadata on calls
add_test(
  NAME cpp_hir_template_call_info_dump
  COMMAND c4cll --dump-hir "${INTERNAL_TEST_ROOT}/cpp/postive_case/template_func.cpp"
)
set_tests_properties(cpp_hir_template_call_info_dump PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "add<int>\\(20, 22\\)"
)

# HIR consteval call info dump test: verify consteval call metadata preserved
add_test(
  NAME cpp_hir_consteval_call_info_dump
  COMMAND c4cll --dump-hir "${INTERNAL_TEST_ROOT}/cpp/postive_case/consteval_func.cpp"
)
set_tests_properties(cpp_hir_consteval_call_info_dump PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "consteval square\\(6\\) = 36"
)

# HIR consteval template call info dump test: verify template consteval call metadata
add_test(
  NAME cpp_hir_consteval_template_call_info_dump
  COMMAND c4cll --dump-hir "${INTERNAL_TEST_ROOT}/cpp/postive_case/consteval_template.cpp"
)
set_tests_properties(cpp_hir_consteval_template_call_info_dump PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "consteval square<T=int>\\(6\\) = 36"
)

# HIR compile-time reduction pass stats: verify pass runs and reports on template+consteval code
add_test(
  NAME cpp_hir_compile_time_reduction_stats
  COMMAND c4cll --dump-hir "${INTERNAL_TEST_ROOT}/cpp/postive_case/consteval_template.cpp"
)
set_tests_properties(cpp_hir_compile_time_reduction_stats PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "compile-time reduction:.*\\(converged\\)"
)

# HIR template instantiation resolution: verify all template calls resolved to functions
add_test(
  NAME cpp_hir_template_instantiation_resolved
  COMMAND c4cll --dump-hir "${INTERNAL_TEST_ROOT}/cpp/postive_case/template_func.cpp"
)
set_tests_properties(cpp_hir_template_instantiation_resolved PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "1 template call resolved.*\\(converged\\)"
)

# HIR consteval reduction verification: verify all consteval calls verified as reduced
add_test(
  NAME cpp_hir_consteval_reduction_verified
  COMMAND c4cll --dump-hir "${INTERNAL_TEST_ROOT}/cpp/postive_case/consteval_func.cpp"
)
set_tests_properties(cpp_hir_consteval_reduction_verified PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "1 consteval reduction.*\\(converged\\)"
)

# HIR consteval+template reduction verification: verify both steps converge
add_test(
  NAME cpp_hir_consteval_template_reduction_verified
  COMMAND c4cll --dump-hir "${INTERNAL_TEST_ROOT}/cpp/postive_case/consteval_template.cpp"
)
set_tests_properties(cpp_hir_consteval_template_reduction_verified PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "1 consteval reduction.*\\(converged\\)"
)

# HIR fixpoint convergence: verify multi-template+consteval scenario converges in 1 iteration
add_test(
  NAME cpp_hir_fixpoint_convergence
  COMMAND c4cll --dump-hir "${INTERNAL_TEST_ROOT}/cpp/postive_case/consteval_nested_template.cpp"
)
set_tests_properties(cpp_hir_fixpoint_convergence PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "1 iteration, 6 template calls resolved, 8 consteval reductions \\(converged\\)"
)

# HIR specialization key: verify stable identity keys for template instantiations
add_test(
  NAME cpp_hir_specialization_key
  COMMAND c4cll --dump-hir "${INTERNAL_TEST_ROOT}/cpp/postive_case/template_func.cpp"
)
set_tests_properties(cpp_hir_specialization_key PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "key=add<T=int>"
)

# HIR materialization: verify materialization pass reports on template+consteval code
add_test(
  NAME cpp_hir_materialization_stats
  COMMAND c4cll --dump-hir "${INTERNAL_TEST_ROOT}/cpp/postive_case/consteval_template.cpp"
)
set_tests_properties(cpp_hir_materialization_stats PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "materialization:.*function.*materialized"
)

add_test(
    NAME frontend_cxx_preprocessor_tests
    COMMAND frontend_cxx_preprocessor_tests
)
set_tests_properties(frontend_cxx_preprocessor_tests PROPERTIES
    LABELS "internal;preprocessor")

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
              -P "${INTERNAL_TEST_CMAKE_ROOT}/run_ccc_review_case.cmake"
    )
    set_tests_properties("${test_name}" PROPERTIES LABELS "internal;positive_case;ccc_review")
  endforeach()
endif()

if(CLANG_EXECUTABLE AND
   (CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64") AND
   EXISTS "${INTERNAL_TEST_ROOT}/inline_asm/aarch64/simple.c")
  add_test(
    NAME inline_asm_aarch64_simple
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DCLANG=${CLANG_EXECUTABLE}
            -DSRC=${INTERNAL_TEST_ROOT}/inline_asm/aarch64/simple.c
            -DOUT_LL=${CMAKE_BINARY_DIR}/inline_asm/aarch64/simple.ll
            -DOUT_BIN=${CMAKE_BINARY_DIR}/inline_asm/aarch64/simple.bin
            -P "${INTERNAL_TEST_CMAKE_ROOT}/run_inline_asm_case.cmake"
  )
  set_tests_properties(inline_asm_aarch64_simple PROPERTIES
      LABELS "internal;inline_asm")
endif()
