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
    trailing_return_type_runtime
    operator_call_rvalue_ref_runtime
    template_operator_call_rvalue_ref_runtime
    template_constexpr_member_runtime
)

set(CPP_POSITIVE_PARSE_STEMS
    alignas_symbol_parse
    operator_decl_subscript_parse
    operator_decl_deref_parse
    operator_decl_arrow_parse
    operator_decl_preinc_parse
    operator_decl_eq_parse
    operator_decl_bool_parse
    operator_decl_plus_minus_parse
    operator_decl_shift_qualified_parse
    operator_new_delete_overload_parse
    new_expr_basic_parse
    placement_new_expr_parse
    class_specific_new_delete_parse
    operator_conversion_out_of_class_parse
    operator_shift_static_member_call_parse
    template_anon_param_extern_cxx_parse
    template_bool_specialization_parse
    scope_resolution_expr_parse
    template_dependent_enum_parse
    template_dependent_enum_sizeof_parse
    template_type_context_nttp_parse
    template_qualified_nttp_parse
    template_variadic_and_nttp_parse
    template_variadic_mixed_parse
    template_variadic_nested_parse
    template_variadic_qualified_parse
    template_template_param_parse
    template_member_type_direct_parse
    template_member_type_inherited_parse
    template_alias_nttp_expr_parse
    template_alias_nttp_expr_inherited_parse
    template_argument_loop_staging_parse
    template_argument_expr_close_staging_parse
    member_template_decltype_default_parse
    member_template_decltype_overload_parse
    member_template_nested_call_default_parse
    templated_member_nested_scope_parse
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
    template_conversion_operator_parse
    template_struct_specialization_parse
    default_param_value_parse
    member_pointer_param_parse
    variadic_param_pack_declarator_parse
    eastl_slice4_typename_and_specialization_parse
    eastl_slice5_explicit_decltype_memfn
    eastl_slice7_piecewise_ctor_parse
    eastl_slice7b_template_using_alias_parse
    float_literal_suffixes_parse
    cpp11_attr_template_decl_parse
    access_labels_parse
    access_labels_treated_public_runtime
    friend_access_parse
    friend_inline_operator_parse
    eastl_slice7d_qualified_declarator_parse
    out_of_class_member_owner_scope_parse
    eastl_probe_pack_expansion_template_arg_parse
    eastl_probe_qualified_template_scope_parse
    template_typedef_nttp_variants_parse
    template_typename_typed_nttp_parse
    qualified_dependent_typename_global_parse
    qualified_cpp_base_type_dispatch_parse
    qualified_type_spelling_shared_parse
    qualified_type_resolution_dispatch_parse
    qualified_type_start_shared_probe_parse
    qualified_global_type_start_shared_probe_parse
    qualified_member_pointer_template_owner_parse
    qualified_member_function_pointer_template_owner_parse
    qualified_type_start_probe_parse
    qualified_operator_template_owner_parse
    global_qualified_member_pointer_template_owner_parse
    declarator_array_suffix_staging_parse
    declarator_grouped_suffix_staging_parse
    declarator_pointer_qualifier_staging_parse
    declarator_pointer_ref_qualifier_staging_parse
    declarator_parenthesized_fn_ptr_staging_parse
    declarator_member_fn_ptr_suffix_staging_parse
    declarator_parenthesized_member_pointer_prefix_parse
    declarator_parenthesized_pointer_inner_staging_parse
    declarator_parenthesized_member_pointer_finalization_parse
    declarator_normal_tail_staging_parse
    declarator_non_parenthesized_suffix_staging_parse
    declarator_plain_function_suffix_staging_parse
    record_nested_aggregate_member_parse
    record_member_typedef_using_parse
    record_member_enum_parse
    record_member_prelude_parse
    record_member_mixed_prelude_parse
    record_member_entry_parse
    record_member_special_member_parse
    record_member_method_field_parse
    record_member_dispatch_parse
    record_member_template_scope_cleanup_parse
    record_member_specialization_context_parse
    record_member_recovery_parse
    record_specialization_setup_parse
    record_prebody_setup_parse
    record_decl_attrs_prelude_parse
    record_tag_setup_parse
    record_definition_setup_parse
    record_body_loop_parse
    record_body_context_parse
    record_body_finalization_parse
    record_completion_handoff_parse
    record_body_state_bundle_parse
)

list(APPEND CPP_POSITIVE_FRONTEND_STEMS
    eastl_probe_call_result_lvalue_frontend
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
  PASS_REGULAR_EXPRESSION "1 iteration, 6 template calls resolved, 0 template types resolved, 8 consteval reductions \\(converged\\)"
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
  NAME cpp_hir_template_struct_registry_primary_only
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/template_struct_specialization_parse.cpp"
)
set_tests_properties(cpp_hir_template_struct_registry_primary_only PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  FAIL_REGULAR_EXPRESSION "struct static_min__spec_|struct remove_const__spec_|struct disjunction__spec_|struct complex_default__spec_"
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
