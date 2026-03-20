set(INTERNAL_CPP_TEST_ROOT "${PROJECT_SOURCE_DIR}/tests/cpp/internal")
set(INTERNAL_C_TEST_CMAKE_ROOT "${PROJECT_SOURCE_DIR}/tests/c/internal/cmake")

file(GLOB INTERNAL_CPP_POSITIVE_TEST_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_CPP_TEST_ROOT}/postive_case/*.cpp")
file(GLOB INTERNAL_CPP_NEGATIVE_TEST_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_CPP_TEST_ROOT}/negative_case/*.cpp")

set(CPP_POSITIVE_FRONTEND_STEMS
    if_constexpr_template_chain
    template_type_traits_builtin
    type_traits_builtin
)

set(CPP_POSITIVE_PARSE_STEMS
    operator_decl_subscript_parse
    operator_decl_deref_parse
    operator_decl_arrow_parse
    operator_decl_preinc_parse
    operator_decl_eq_parse
    operator_decl_bool_parse
    operator_decl_plus_minus_parse
    operator_decl_shift_qualified_parse
    template_anon_param_extern_cxx_parse
    template_bool_specialization_parse
    scope_resolution_expr_parse
    template_dependent_enum_parse
    template_type_context_nttp_parse
    namespace_basic_parse
    namespace_nested_parse
    namespace_three_level_parse
    namespace_anonymous_basic_parse
    namespace_anonymous_nested_parse
    namespace_global_qualifier_parse
    namespace_cross_namespace_lookup_parse
    namespace_global_qualified_self_parse
    using_alias_basic_parse
    using_scoped_typedef_parse
    using_declaration_namespace_parse
    using_namespace_directive_parse
    using_nested_namespace_parse
)

foreach(src IN LISTS INTERNAL_CPP_NEGATIVE_TEST_SRCS)
  get_filename_component(stem "${src}" NAME_WE)
  set(test_name "cpp_negative_tests_${stem}")
  add_test(
    NAME "${test_name}"
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DSRC=${src}
            -DENFORCE_NEGATIVE=ON
            -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_negative_case.cmake"
  )
  set_tests_properties("${test_name}" PROPERTIES LABELS "internal;negative_case;cpp")
endforeach()

if(CLANG_EXECUTABLE)
  foreach(src IN LISTS INTERNAL_CPP_POSITIVE_TEST_SRCS)
    file(RELATIVE_PATH rel_src "${INTERNAL_CPP_TEST_ROOT}/postive_case" "${src}")
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
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_positive_case.cmake"
    )
    set_tests_properties("${test_name}" PROPERTIES LABELS "internal;positive_case;cpp")
  endforeach()
else()
  message(WARNING "clang not found: skipping internal cpp positive_case runtime tests")
endif()

add_test(
  NAME cpp_hir_template_def_dump
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/template_func.cpp"
)
set_tests_properties(cpp_hir_template_def_dump PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "template add<typename T>.*instantiation"
)

add_test(
  NAME cpp_hir_consteval_template_dump
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/consteval_template.cpp"
)
set_tests_properties(cpp_hir_consteval_template_dump PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "template consteval square<typename T>"
)

add_test(
  NAME cpp_hir_template_call_info_dump
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/template_func.cpp"
)
set_tests_properties(cpp_hir_template_call_info_dump PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "add<int>\\(20, 22\\)"
)

add_test(
  NAME cpp_hir_consteval_call_info_dump
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/consteval_func.cpp"
)
set_tests_properties(cpp_hir_consteval_call_info_dump PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "consteval square\\(6\\) = 36"
)

add_test(
  NAME cpp_hir_consteval_template_call_info_dump
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/consteval_template.cpp"
)
set_tests_properties(cpp_hir_consteval_template_call_info_dump PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "consteval square<T=int>\\(6\\) = 36"
)

add_test(
  NAME cpp_hir_compile_time_reduction_stats
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/consteval_template.cpp"
)
set_tests_properties(cpp_hir_compile_time_reduction_stats PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "compile-time reduction:.*\\(converged\\)"
)

add_test(
  NAME cpp_hir_template_instantiation_resolved
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/template_func.cpp"
)
set_tests_properties(cpp_hir_template_instantiation_resolved PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "1 template call resolved.*\\(converged\\)"
)

add_test(
  NAME cpp_hir_consteval_reduction_verified
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/consteval_func.cpp"
)
set_tests_properties(cpp_hir_consteval_reduction_verified PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "1 consteval reduction.*\\(converged\\)"
)

add_test(
  NAME cpp_hir_consteval_template_reduction_verified
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/consteval_template.cpp"
)
set_tests_properties(cpp_hir_consteval_template_reduction_verified PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "1 consteval reduction.*\\(converged\\)"
)

add_test(
  NAME cpp_hir_fixpoint_convergence
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/consteval_nested_template.cpp"
)
set_tests_properties(cpp_hir_fixpoint_convergence PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "1 iteration, 6 template calls resolved, 8 consteval reductions \\(converged\\)"
)

add_test(
  NAME cpp_hir_deferred_template_instantiation
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/template_chain.cpp"
)
set_tests_properties(cpp_hir_deferred_template_instantiation PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "1 deferred instantiation.*\\(converged\\)"
)

add_test(
  NAME cpp_hir_deferred_consteval_chain
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/deferred_consteval_chain.cpp"
)
set_tests_properties(cpp_hir_deferred_consteval_chain PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "deferred consteval reduction.*\\(converged\\)"
)

add_test(
  NAME cpp_hir_deferred_consteval_multi
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/deferred_consteval_multi.cpp"
)
set_tests_properties(cpp_hir_deferred_consteval_multi PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "2 deferred consteval reduction.*\\(converged\\)"
)

add_test(
  NAME cpp_hir_specialization_key
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/template_func.cpp"
)
set_tests_properties(cpp_hir_specialization_key PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "key=add<T=int>"
)

add_test(
  NAME cpp_hir_materialization_stats
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/consteval_template.cpp"
)
set_tests_properties(cpp_hir_materialization_stats PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "materialization:.*function.*materialized"
)

add_test(
  NAME cpp_hir_materialization_boundary
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/materialization_boundary.cpp"
)
set_tests_properties(cpp_hir_materialization_boundary PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "2 compile-time only"
)

add_test(
  NAME cpp_hir_template_origin
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/materialization_boundary.cpp"
)
set_tests_properties(cpp_hir_template_origin PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "template<apply>"
)

add_test(
  NAME cpp_hir_spec_key_identity
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/specialization_identity.cpp"
)
set_tests_properties(cpp_hir_spec_key_identity PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "key=add<T=int>"
)

add_test(
  NAME cpp_hir_spec_key_distinct
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/specialization_identity.cpp"
)
set_tests_properties(cpp_hir_spec_key_distinct PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "key=add<T=double>"
)

add_test(
  NAME cpp_llvm_spec_key_metadata
  COMMAND c4cll "${INTERNAL_CPP_TEST_ROOT}/postive_case/specialization_identity.cpp"
)
set_tests_properties(cpp_llvm_spec_key_metadata PROPERTIES
  LABELS "internal;positive_case;cpp;llvm"
  PASS_REGULAR_EXPRESSION "spec-key: add<T=int>"
)

add_test(
  NAME cpp_llvm_spec_key_named_metadata
  COMMAND c4cll "${INTERNAL_CPP_TEST_ROOT}/postive_case/specialization_identity.cpp"
)
set_tests_properties(cpp_llvm_spec_key_named_metadata PROPERTIES
  LABELS "internal;positive_case;cpp;llvm"
  PASS_REGULAR_EXPRESSION "!c4c\\.specializations"
)

add_test(
  NAME cpp_llvm_spec_key_named_metadata_entry
  COMMAND c4cll "${INTERNAL_CPP_TEST_ROOT}/postive_case/specialization_identity.cpp"
)
set_tests_properties(cpp_llvm_spec_key_named_metadata_entry PROPERTIES
  LABELS "internal;positive_case;cpp;llvm"
  PASS_REGULAR_EXPRESSION "!\"add<T=int>\""
)
