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
    builtin_trait_is_same_frontend
    trailing_return_type_runtime
    operator_call_rvalue_ref_runtime
    template_operator_call_rvalue_ref_runtime
    template_constexpr_member_runtime
    cpp20_out_of_class_trailing_requires_runtime
    cpp20_record_member_trailing_requires_frontend
)

set(CPP_POSITIVE_PARSE_STEMS
    alignas_symbol_parse
    operator_decl_subscript_parse
    operator_decl_deref_parse
    operator_decl_arrow_parse
    operator_arrow_explicit_member_call_parse
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
    template_unresolved_param_type_parse
    qualified_template_unresolved_param_type_parse
    unresolved_by_value_param_type_parse
    template_friend_record_member_parse
    template_member_type_direct_parse
    template_member_type_inherited_parse
    template_alias_nttp_expr_parse
    template_alias_nttp_expr_inherited_parse
    template_argument_loop_staging_parse
    template_argument_expr_close_staging_parse
    member_template_decltype_default_parse
    member_template_decltype_overload_parse
    member_template_sfinae_typename_prelude_parse
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
    forward_declared_record_specialization_parse
    default_param_value_parse
    member_pointer_param_parse
    param_leading_cpp11_attr_parse
    pseudo_destructor_if_else_parse
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
    if_condition_decl_parse
    free_function_record_ref_param_parse
    iterator_concepts_following_hash_base_parse
    cpp20_requires_clause_parse
    cpp20_requires_clause_struct_decl_parse
    cpp20_trailing_requires_following_member_decl_parse
    cpp20_requires_trait_disjunction_member_parse
    cpp20_requires_concept_id_member_ctor_parse
    cpp20_requires_conjunction_member_parse
    cpp20_namespaced_concept_requires_parse
    cpp20_requires_requires_member_array_param_parse
    cpp20_requires_expression_if_constexpr_parse
    eastl_slice7d_qualified_declarator_parse
    out_of_class_member_owner_scope_parse
    eastl_probe_pack_expansion_template_arg_parse
    eastl_probe_qualified_template_scope_parse
    sfinae_function_signature_patterns_parse
    sfinae_specialization_gating_parse
    sfinae_template_parameter_patterns_parse
    template_typedef_nttp_variants_parse
    template_typename_typed_nttp_parse
    qualified_dependent_typename_global_parse
    keyword_typename_final_parse
    keyword_noexcept_nullptr_parse
    noexcept_expr_parse
    keyword_access_virtual_parse
    keyword_friend_parse
    keyword_mutable_parse
    keyword_and_parse
    keyword_bitand_parse
    keyword_bitor_parse
    keyword_bitxor_parse
    keyword_compl_parse
    keyword_or_parse
    keyword_not_parse
    qualified_cpp_base_type_dispatch_parse
    gcc_type_trait_type_arg_parse
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
    record_member_special_dispatch_parse
    record_member_method_field_parse
    record_member_type_dispatch_parse
    record_member_dispatch_parse
    record_member_template_scope_cleanup_parse
    record_member_specialization_context_parse
    record_member_recovery_parse
    record_specialization_setup_parse
    record_prebody_setup_parse
    record_decl_attrs_prelude_parse
    record_base_clause_setup_parse
    record_tag_setup_parse
    record_definition_setup_parse
    record_definition_dispatch_parse
    record_body_loop_parse
    record_body_member_attempt_parse
    record_body_duplicate_checker_parse
    record_body_context_parse
    record_body_context_teardown_parse
    record_body_finalization_parse
    record_completion_handoff_parse
    record_definition_body_handoff_parse
    record_body_state_bundle_parse
    qualified_record_partial_specialization_parse
    record_final_specifier_parse
    lambda_empty_capture_parse
    lambda_empty_capture_parens_parse
    lambda_ref_capture_parse
    lambda_copy_capture_parse
)

list(APPEND CPP_POSITIVE_FRONTEND_STEMS
    call_expr_ref_return_lvalue_frontend
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
  NAME cpp_parse_lambda_empty_capture_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/lambda_empty_capture_parse.cpp"
)
set_tests_properties(cpp_parse_lambda_empty_capture_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Lambda\\(\\[\\]\\)"
)

add_test(
  NAME cpp_parse_lambda_empty_capture_parens_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/lambda_empty_capture_parens_parse.cpp"
)
set_tests_properties(cpp_parse_lambda_empty_capture_parens_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Lambda\\(\\[\\],\\(\\)\\)"
)

add_test(
  NAME cpp_parse_lambda_ref_capture_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/lambda_ref_capture_parse.cpp"
)
set_tests_properties(cpp_parse_lambda_ref_capture_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Lambda\\(\\[&\\]\\)"
)

add_test(
  NAME cpp_parse_lambda_copy_capture_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/lambda_copy_capture_parse.cpp"
)
set_tests_properties(cpp_parse_lambda_copy_capture_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Lambda\\(\\[=\\]\\)"
)

add_test(
  NAME cpp_parse_constructor_self_ref_param_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/constructor_self_ref_param_parse.cpp"
)
set_tests_properties(cpp_parse_constructor_self_ref_param_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Function\\(accept\\)"
)

add_test(
  NAME cpp_parse_unresolved_by_value_param_type_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/unresolved_by_value_param_type_parse.cpp"
)
set_tests_properties(cpp_parse_unresolved_by_value_param_type_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Function\\(accept\\)"
)

add_test(
  NAME cpp_parse_template_friend_record_member_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/template_friend_record_member_parse.cpp"
)
set_tests_properties(cpp_parse_template_friend_record_member_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Function\\(operator_postinc\\)"
)

add_test(
  NAME cpp_parse_forward_declared_record_specialization_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/forward_declared_record_specialization_parse.cpp"
)
set_tests_properties(cpp_parse_forward_declared_record_specialization_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "StructDef\\(struct Box__spec_[0-9]+\\) specialize<1>"
)

add_test(
  NAME cpp_parse_gcc_type_trait_type_arg_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/gcc_type_trait_type_arg_parse.cpp"
)
set_tests_properties(cpp_parse_gcc_type_trait_type_arg_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Decl\\(can_fill\\)"
)

add_test(
  NAME cpp_parse_trailing_requires_following_member_decl_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/cpp20_trailing_requires_following_member_decl_parse.cpp"
)
set_tests_properties(cpp_parse_trailing_requires_following_member_decl_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Decl\\(inner\\)"
)

add_test(
  NAME cpp_parse_record_member_recovery_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/record_member_recovery_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_record_member_recovery_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "Decl\\(kept\\)"
)

add_test(
  NAME cpp_parse_record_member_using_recovery_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/record_member_using_recovery_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_record_member_using_recovery_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "Decl\\(kept\\)"
)

add_test(
  NAME cpp_parse_record_member_requires_clause_recovery_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/record_member_requires_clause_recovery_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_record_member_requires_clause_recovery_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "Decl\\(kept\\)"
)

add_test(
  NAME cpp_parse_local_using_alias_recovery_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/local_using_alias_recovery_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_local_using_alias_recovery_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "InvalidStmt.*Decl\\(kept\\)|Decl\\(kept\\).*InvalidStmt"
)

add_test(
  NAME cpp_parse_top_level_storage_class_recovery_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_storage_class_recovery_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_storage_class_recovery_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(kept\\)"
  FAIL_REGULAR_EXPRESSION "Empty"
)

add_test(
  NAME cpp_parse_top_level_asm_recovery_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_asm_recovery_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_asm_recovery_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(kept\\)"
)

add_test(
  NAME cpp_parse_top_level_using_namespace_recovery_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_using_namespace_recovery_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_using_namespace_recovery_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(kept\\)"
)

add_test(
  NAME cpp_parse_top_level_using_alias_recovery_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_using_alias_recovery_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_using_alias_recovery_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(kept\\)"
)

add_test(
  NAME cpp_parse_top_level_using_decl_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_using_decl_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_using_decl_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(kept\\)"
  FAIL_REGULAR_EXPRESSION "Empty"
)

add_test(
  NAME cpp_parse_top_level_extern_c_empty_block_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_extern_c_empty_block_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_extern_c_empty_block_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(kept\\)"
  FAIL_REGULAR_EXPRESSION "Empty"
)

add_test(
  NAME cpp_parse_top_level_empty_namespace_block_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_empty_namespace_block_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_empty_namespace_block_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(kept\\)"
  FAIL_REGULAR_EXPRESSION "Empty"
)

add_test(
  NAME cpp_parse_top_level_class_recovery_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_class_recovery_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_class_recovery_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "StructDef\\(struct kept\\)"
  FAIL_REGULAR_EXPRESSION "Empty"
)

add_test(
  NAME cpp_parse_top_level_tag_decl_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_tag_decl_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_tag_decl_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(kept\\)"
  FAIL_REGULAR_EXPRESSION "Empty;Program\\(6 items\\)"
)

add_test(
  NAME cpp_parse_top_level_forward_tag_decl_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_forward_tag_decl_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_forward_tag_decl_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(kept\\)"
  FAIL_REGULAR_EXPRESSION "Empty;Program\\(3 items\\)"
)

add_test(
  NAME cpp_parse_top_level_tag_attr_decl_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_tag_attr_decl_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_tag_attr_decl_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(kept\\)"
  FAIL_REGULAR_EXPRESSION "Empty"
)

add_test(
  NAME cpp_parse_top_level_concept_decl_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_concept_decl_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_concept_decl_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(kept\\)"
  FAIL_REGULAR_EXPRESSION "Empty"
)

add_test(
  NAME cpp_parse_top_level_typedef_decl_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_typedef_decl_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_typedef_decl_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(kept\\)"
  FAIL_REGULAR_EXPRESSION "Empty"
)

add_test(
  NAME cpp_parse_top_level_pragma_pack_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_pragma_pack_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_pragma_pack_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(kept\\)"
  FAIL_REGULAR_EXPRESSION "Empty"
)

add_test(
  NAME cpp_parse_top_level_pragma_gcc_visibility_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_pragma_gcc_visibility_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_pragma_gcc_visibility_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(kept\\)"
  FAIL_REGULAR_EXPRESSION "Empty"
)

add_test(
  NAME cpp_parse_top_level_asm_decl_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_asm_decl_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_asm_decl_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(kept\\)"
  FAIL_REGULAR_EXPRESSION "Empty"
)

add_test(
  NAME cpp_lex_keyword_and_tokens
  COMMAND c4cll --lex-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/keyword_and_parse.cpp"
)
set_tests_properties(cpp_lex_keyword_and_tokens PROPERTIES
  LABELS "internal;positive_case;cpp;lex"
  PASS_REGULAR_EXPRESSION "AMP_AMP 'and'"
)

add_test(
  NAME cpp_parse_keyword_and_operator_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/keyword_and_parse.cpp"
)
set_tests_properties(cpp_parse_keyword_and_operator_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Function\\(operator_and\\)"
)

add_test(
  NAME cpp_lex_keyword_or_tokens
  COMMAND c4cll --lex-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/keyword_or_parse.cpp"
)
set_tests_properties(cpp_lex_keyword_or_tokens PROPERTIES
  LABELS "internal;positive_case;cpp;lex"
  PASS_REGULAR_EXPRESSION "PIPE_PIPE 'or'"
)

add_test(
  NAME cpp_parse_keyword_or_operator_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/keyword_or_parse.cpp"
)
set_tests_properties(cpp_parse_keyword_or_operator_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Function\\(operator_or\\)"
)

add_test(
  NAME cpp_lex_keyword_not_tokens
  COMMAND c4cll --lex-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/keyword_not_parse.cpp"
)
set_tests_properties(cpp_lex_keyword_not_tokens PROPERTIES
  LABELS "internal;positive_case;cpp;lex"
  PASS_REGULAR_EXPRESSION "BANG 'not'"
)

add_test(
  NAME cpp_parse_keyword_not_operator_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/keyword_not_parse.cpp"
)
set_tests_properties(cpp_parse_keyword_not_operator_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Function\\(operator_not\\)"
)

add_test(
  NAME cpp_lex_keyword_not_eq_tokens
  COMMAND c4cll --lex-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/keyword_not_eq_parse.cpp"
)
set_tests_properties(cpp_lex_keyword_not_eq_tokens PROPERTIES
  LABELS "internal;positive_case;cpp;lex"
  PASS_REGULAR_EXPRESSION "BANG_EQUAL 'not_eq'"
)

add_test(
  NAME cpp_parse_keyword_not_eq_operator_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/keyword_not_eq_parse.cpp"
)
set_tests_properties(cpp_parse_keyword_not_eq_operator_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Function\\(operator_neq\\)"
)

add_test(
  NAME cpp_lex_keyword_bitand_tokens
  COMMAND c4cll --lex-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/keyword_bitand_parse.cpp"
)
set_tests_properties(cpp_lex_keyword_bitand_tokens PROPERTIES
  LABELS "internal;positive_case;cpp;lex"
  PASS_REGULAR_EXPRESSION "AMP 'bitand'"
)

add_test(
  NAME cpp_parse_keyword_bitand_operator_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/keyword_bitand_parse.cpp"
)
set_tests_properties(cpp_parse_keyword_bitand_operator_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Function\\(operator_bitand\\)"
)

add_test(
  NAME cpp_lex_keyword_bitor_tokens
  COMMAND c4cll --lex-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/keyword_bitor_parse.cpp"
)
set_tests_properties(cpp_lex_keyword_bitor_tokens PROPERTIES
  LABELS "internal;positive_case;cpp;lex"
  PASS_REGULAR_EXPRESSION "PIPE 'bitor'"
)

add_test(
  NAME cpp_parse_keyword_bitor_operator_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/keyword_bitor_parse.cpp"
)
set_tests_properties(cpp_parse_keyword_bitor_operator_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Function\\(operator_bitor\\)"
)

add_test(
  NAME cpp_lex_keyword_bitxor_tokens
  COMMAND c4cll --lex-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/keyword_bitxor_parse.cpp"
)
set_tests_properties(cpp_lex_keyword_bitxor_tokens PROPERTIES
  LABELS "internal;positive_case;cpp;lex"
  PASS_REGULAR_EXPRESSION "CARET 'bitxor'"
)

add_test(
  NAME cpp_parse_keyword_bitxor_operator_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/keyword_bitxor_parse.cpp"
)
set_tests_properties(cpp_parse_keyword_bitxor_operator_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Function\\(operator_bitxor\\)"
)

add_test(
  NAME cpp_lex_keyword_compl_tokens
  COMMAND c4cll --lex-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/keyword_compl_parse.cpp"
)
set_tests_properties(cpp_lex_keyword_compl_tokens PROPERTIES
  LABELS "internal;positive_case;cpp;lex"
  PASS_REGULAR_EXPRESSION "TILDE 'compl'"
)

add_test(
  NAME cpp_parse_keyword_compl_operator_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/keyword_compl_parse.cpp"
)
set_tests_properties(cpp_parse_keyword_compl_operator_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Function\\(operator_bitnot\\)"
)

add_test(
  NAME cpp_lex_keyword_requires_tokens
  COMMAND c4cll --lex-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/cpp20_requires_clause_parse.cpp"
)
set_tests_properties(cpp_lex_keyword_requires_tokens PROPERTIES
  LABELS "internal;positive_case;cpp;lex"
  PASS_REGULAR_EXPRESSION "KW_requires 'requires'"
)

add_test(
  NAME cpp_hir_template_def_dump
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/template_func.cpp"
)
set_tests_properties(cpp_hir_template_def_dump PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "template add<typename T>.*instantiation"
)

add_test(
  NAME cpp_parser_debug_expr_stmt_stack
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_expr_stmt_stack.cpp
          -DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=parse_primary
          -DEXPECT_STACK_SUBSTRING:STRING=[pdebug] stack: -> parse_top_level -> parse_block -> parse_stmt -> parse_expr -> parse_assign_expr -> parse_ternary -> parse_unary -> parse_primary
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_expr_stmt_stack PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)

add_test(
  NAME cpp_parser_debug_record_member_stack
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_record_member_stack.cpp
          -DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=try_parse_record_method_or_field_member
          -DEXPECT_STACK_SUBSTRING:STRING=[pdebug] stack: -> parse_top_level -> try_parse_record_member_dispatch -> try_parse_record_method_or_field_member
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_record_member_stack PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)

add_test(
  NAME cpp_parser_debug_record_member_param_default_rank
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_record_member_param_default_rank.cpp
          -DEXPECT_ERROR_SUBSTRING:STRING=expected=RPAREN
          -DEXPECT_STACK_SUBSTRING:STRING=parse_param
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_record_member_param_default_rank PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)

add_test(
  NAME cpp_parser_debug_record_member_type_like_rank
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_record_member_type_like_rank.cpp
          -DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=try_parse_record_using_member
          -DEXPECT_STACK_SUBSTRING:STRING=[pdebug] stack: -> parse_top_level -> try_parse_record_member_dispatch -> try_parse_record_type_like_member_dispatch -> try_parse_record_using_member
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_record_member_type_like_rank PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)

add_test(
  NAME cpp_parser_debug_record_member_using_alias_leaf
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_record_member_using_alias_leaf.cpp
          -DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=try_parse_record_using_member
          -DEXPECT_STACK_SUBSTRING:STRING=[pdebug] stack: -> parse_top_level -> try_parse_record_member_dispatch -> try_parse_record_type_like_member_dispatch -> try_parse_record_using_member
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_record_member_using_alias_leaf PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)

add_test(
  NAME cpp_parser_debug_record_member_typedef_leaf
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_record_member_typedef_leaf.cpp
          -DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=try_parse_record_typedef_member
          -DEXPECT_STACK_SUBSTRING:STRING=[pdebug] stack: -> parse_top_level -> try_parse_record_member_dispatch -> try_parse_record_type_like_member_dispatch -> try_parse_record_typedef_member
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_record_member_typedef_leaf PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)


add_test(
  NAME cpp_parser_debug_qualified_type_top_level_params
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_qualified_type_top_level_params.cpp
          -DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=parse_top_level_parameter_list
          -DEXPECT_STACK_SUBSTRING:STRING=[pdebug] stack: -> parse_top_level -> try_parse_cpp_scoped_base_type -> try_parse_qualified_base_type -> parse_top_level_parameter_list
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_qualified_type_top_level_params PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)

add_test(
  NAME cpp_parser_debug_qualified_type_template_arg_stack
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_qualified_type_template_arg_stack.cpp
          -DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=parse_top_level_parameter_list
          "-DEXPECT_STACK_SUBSTRING:STRING=[pdebug] stack: -> parse_top_level -> try_parse_cpp_scoped_base_type -> try_parse_qualified_base_type -> consume_qualified_type_spelling_with_typename -> consume_qualified_type_spelling -> parse_next_template_argument -> try_parse_template_type_arg -> try_parse_cpp_scoped_base_type -> try_parse_qualified_base_type -> consume_qualified_type_spelling_with_typename -> consume_qualified_type_spelling -> parse_top_level_parameter_list"
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_qualified_type_template_arg_stack PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)

add_test(
  NAME cpp_parser_debug_qualified_type_dependent_typename_stack
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_qualified_type_dependent_typename_stack.cpp
          -DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=parse_top_level_parameter_list
          -DEXPECT_STACK_SUBSTRING:STRING=[pdebug] stack: -> parse_top_level -> parse_next_template_argument -> try_parse_template_type_arg -> try_parse_cpp_scoped_base_type -> parse_dependent_typename_specifier -> parse_top_level_parameter_list
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_qualified_type_dependent_typename_stack PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)

add_test(
  NAME cpp_parser_debug_qualified_type_typename_spelling_stack
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_qualified_type_typename_spelling_stack.cpp
          -DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=parse_top_level_parameter_list
          -DEXPECT_STACK_SUBSTRING:STRING=[pdebug] stack: -> parse_top_level -> parse_next_template_argument -> try_parse_template_type_arg -> try_parse_cpp_scoped_base_type -> parse_dependent_typename_specifier -> consume_qualified_type_spelling_with_typename -> parse_top_level_parameter_list
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_qualified_type_typename_spelling_stack PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)

add_test(
  NAME cpp_parser_debug_qualified_type_spelling_stack
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_qualified_type_spelling_stack.cpp
          "-DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=parse_top_level_parameter_list"
          "-DEXPECT_STACK_SUBSTRING:STRING=[pdebug] stack: -> parse_top_level -> try_parse_cpp_scoped_base_type -> try_parse_qualified_base_type -> consume_qualified_type_spelling_with_typename -> consume_qualified_type_spelling -> parse_next_template_argument -> try_parse_template_type_arg -> parse_top_level_parameter_list"
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_qualified_type_spelling_stack PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
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
  NAME cpp_hir_initial_program_seed_realization
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/materialization_boundary.cpp"
)
set_tests_properties(cpp_hir_initial_program_seed_realization PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "compile-time reduction: 1 iteration, 1 template call resolved, 0 template types resolved, 4 consteval reductions \\(converged\\)"
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
  NAME cpp_hir_template_member_owner_resolution
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/template_member_owner_resolution.cpp"
)
set_tests_properties(cpp_hir_template_member_owner_resolution PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "decl w: struct wrap_T_int"
)

add_test(
  NAME cpp_hir_template_struct_inherited_method_binding
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/template_struct_inherited_method_hir.cpp"
)
set_tests_properties(cpp_hir_template_struct_inherited_method_binding PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "struct Base_T_int.*fn Wrap_T_int__sum\\(this: struct Wrap_T_int\\*\\) -> int"
)

add_test(
  NAME cpp_hir_template_struct_field_array_extent
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/template_struct_nttp.cpp"
)
set_tests_properties(cpp_hir_template_struct_field_array_extent PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "field data: int\\[4\\] llvm_idx=0 offset=0 size=16 align=4"
)

add_test(
  NAME cpp_hir_template_member_owner_chain
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/template_member_owner_chain_hir.cpp"
)
set_tests_properties(cpp_hir_template_member_owner_chain PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "field value: int llvm_idx=0 offset=0 size=4 align=4"
)

add_test(
  NAME cpp_hir_template_member_owner_decl_and_cast
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/template_member_owner_decl_and_cast_hir.cpp"
)
set_tests_properties(cpp_hir_template_member_owner_decl_and_cast PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "decl local: int = input#P0.*return \\(\\(int\\)local#L0\\)"
)

add_test(
  NAME cpp_hir_template_function_signature_binding
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/template_fn_struct.cpp"
)
set_tests_properties(cpp_hir_template_function_signature_binding PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "fn make_pair_i\\(a: int, b: int\\) -> struct Pair_T_int"
)

add_test(
  NAME cpp_hir_template_method_signature_binding
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/template_struct_method_tpl_return.cpp"
)
set_tests_properties(cpp_hir_template_method_signature_binding PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "fn Transformer_T_int__make_scaled_pair\\(this: struct Transformer_T_int\\*, x: int\\) -> struct Pair_T_int"
)

add_test(
  NAME cpp_hir_template_function_pack_signature_binding
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/template_function_pack_signature_hir.cpp"
)
set_tests_properties(cpp_hir_template_function_pack_signature_binding PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "fn first_plus_last_.*\\(args__pack0: int, args__pack1: long\\) -> int"
)

add_test(
  NAME cpp_hir_template_function_recursive_body_binding
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/template_function_recursive_body_hir.cpp"
)
set_tests_properties(cpp_hir_template_function_recursive_body_binding PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "return countdown<int>\\(\\(n#P0 - 1\\)\\)"
)

add_test(
  NAME cpp_hir_defaulted_destructor_member_teardown
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/default_special_members.cpp"
)
set_tests_properties(cpp_hir_defaulted_destructor_member_teardown PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "WithDtor__dtor\\(\\(&this#P0->inner\\)\\)"
)

add_test(
  NAME cpp_hir_template_deferred_nttp_expr
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/template_deferred_nttp_expr_hir.cpp"
)
set_tests_properties(cpp_hir_template_deferred_nttp_expr PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "field data: int\\[3\\].*size=12 align=4"
)

add_test(
  NAME cpp_hir_template_deferred_nttp_arith_expr
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/template_deferred_nttp_arith_expr_hir.cpp"
)
set_tests_properties(cpp_hir_template_deferred_nttp_arith_expr PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "field data: int\\[5\\].*size=20 align=4"
)

add_test(
  NAME cpp_hir_template_deferred_nttp_paren_expr
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/template_deferred_nttp_paren_expr_hir.cpp"
)
set_tests_properties(cpp_hir_template_deferred_nttp_paren_expr PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "field data: int\\[3\\].*size=12 align=4"
)

add_test(
  NAME cpp_hir_template_deferred_nttp_bool_expr
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/template_deferred_nttp_bool_expr_hir.cpp"
)
set_tests_properties(cpp_hir_template_deferred_nttp_bool_expr PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "field data: int\\[1\\].*size=4 align=4"
)

add_test(
  NAME cpp_hir_template_deferred_nttp_logic_expr
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/template_deferred_nttp_logic_expr_hir.cpp"
)
set_tests_properties(cpp_hir_template_deferred_nttp_logic_expr PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "field data: int\\[1\\].*size=4 align=4"
)

add_test(
  NAME cpp_hir_template_deferred_nttp_true_expr
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/template_deferred_nttp_true_expr_hir.cpp"
)
set_tests_properties(cpp_hir_template_deferred_nttp_true_expr PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "field data: int\\[1\\].*size=4 align=4"
)

add_test(
  NAME cpp_hir_template_deferred_nttp_number_expr
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/template_deferred_nttp_number_expr_hir.cpp"
)
set_tests_properties(cpp_hir_template_deferred_nttp_number_expr PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "field data: int\\[4\\].*size=16 align=4"
)

add_test(
  NAME cpp_hir_template_alias_deferred_nttp_static_member
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/template_alias_deferred_nttp_static_member_hir.cpp"
)
set_tests_properties(cpp_hir_template_alias_deferred_nttp_static_member PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "if \\(\\(!1\\)\\) -> block#[0-9]+"
)

add_test(
  NAME cpp_hir_template_deferred_nttp_static_member_expr
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/template_deferred_nttp_static_member_expr_hir.cpp"
)
set_tests_properties(cpp_hir_template_deferred_nttp_static_member_expr PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "field data: int\\[5\\].*size=20 align=4"
)

add_test(
  NAME cpp_hir_template_deferred_nttp_sizeof_pack_expr
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/template_deferred_nttp_sizeof_pack_expr_hir.cpp"
)
set_tests_properties(cpp_hir_template_deferred_nttp_sizeof_pack_expr PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "field data: int\\[3\\].*size=12 align=4"
)

add_test(
  NAME cpp_hir_record_packed_aligned_layout
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/record_packed_aligned_layout_hir.cpp"
)
set_tests_properties(cpp_hir_record_packed_aligned_layout PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "struct LayoutProbe size=16 align=8.*field a: char llvm_idx=0 offset=0 size=1 align=1.*field b: int llvm_idx=1 offset=1 size=4 align=1.*field data: int\\[2\\] llvm_idx=2 offset=5 size=8 align=1"
)

add_test(
  NAME cpp_hir_record_field_array_layout
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/record_field_array_layout_hir.cpp"
)
set_tests_properties(cpp_hir_record_field_array_layout PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "struct FieldArrayLayout size=20 align=4.*field tag: char llvm_idx=0 offset=0 size=1 align=1.*field data: int\\[3\\] llvm_idx=1 offset=4 size=12 align=4.*field tail: short llvm_idx=2 offset=16 size=2 align=2"
)

add_test(
  NAME cpp_hir_builtin_layout_query_sizeof_type
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/builtin_layout_queries_hir.cpp"
)
set_tests_properties(cpp_hir_builtin_layout_query_sizeof_type PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "return 32"
)

add_test(
  NAME cpp_hir_builtin_layout_query_alignof_type
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/builtin_layout_queries_hir.cpp"
)
set_tests_properties(cpp_hir_builtin_layout_query_alignof_type PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "fn alignof_querybox\\(\\) -> int"
)

add_test(
  NAME cpp_hir_builtin_layout_query_alignof_expr
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/builtin_layout_queries_hir.cpp"
)
set_tests_properties(cpp_hir_builtin_layout_query_alignof_expr PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "decl box: struct QueryBox"
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

add_test(
  NAME cpp_llvm_initializer_list_runtime_materialization
  COMMAND c4cll "${INTERNAL_CPP_TEST_ROOT}/postive_case/eastl_probe_initializer_list_runtime.cpp"
)
set_tests_properties(cpp_llvm_initializer_list_runtime_materialization PROPERTIES
  LABELS "internal;positive_case;cpp;llvm"
  PASS_REGULAR_EXPRESSION "alloca \\[3 x i32\\]"
)
