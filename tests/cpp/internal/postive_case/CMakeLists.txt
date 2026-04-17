# Positive case dump tests (parse / lex / hir / llvm) -- extracted from root CMakeLists.txt

add_test(
  NAME cpp_parse_lambda_empty_capture_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/lambda_empty_capture_parse.cpp"
)
set_tests_properties(cpp_parse_lambda_empty_capture_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Lambda\\(\\[\\]\\)"
)

add_test(
  NAME cpp_parse_lambda_empty_capture_parens_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/lambda_empty_capture_parens_parse.cpp"
)
set_tests_properties(cpp_parse_lambda_empty_capture_parens_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Lambda\\(\\[\\],\\(\\)\\)"
)

add_test(
  NAME cpp_parse_lambda_ref_capture_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/lambda_ref_capture_parse.cpp"
)
set_tests_properties(cpp_parse_lambda_ref_capture_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Lambda\\(\\[&\\]\\)"
)

add_test(
  NAME cpp_parse_lambda_copy_capture_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/lambda_copy_capture_parse.cpp"
)
set_tests_properties(cpp_parse_lambda_copy_capture_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Lambda\\(\\[=\\]\\)"
)

add_test(
  NAME cpp_parse_constructor_self_ref_param_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/constructor_self_ref_param_parse.cpp"
)
set_tests_properties(cpp_parse_constructor_self_ref_param_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Function\\(accept\\)"
)

add_test(
  NAME cpp_parse_unresolved_by_value_param_type_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/unresolved_by_value_param_type_parse.cpp"
)
set_tests_properties(cpp_parse_unresolved_by_value_param_type_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Function\\(accept\\)"
)

add_test(
  NAME cpp_parse_template_template_scope_member_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/template_template_scope_member_parse.cpp"
)
set_tests_properties(cpp_parse_template_template_scope_member_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Decl\\(value\\)"
)

add_test(
  NAME cpp_parse_template_friend_record_member_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/template_friend_record_member_parse.cpp"
)
set_tests_properties(cpp_parse_template_friend_record_member_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Function\\(operator_postinc\\)"
)

add_test(
  NAME cpp_parse_friend_spaceship_operator_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/friend_spaceship_operator_parse.cpp"
)
set_tests_properties(cpp_parse_friend_spaceship_operator_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Decl\\(marker\\)"
)

add_test(
  NAME cpp_parse_member_spaceship_operator_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/member_spaceship_operator_parse.cpp"
)
set_tests_properties(cpp_parse_member_spaceship_operator_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Function\\(operator_spaceship\\)"
)

add_test(
  NAME cpp_parse_forward_declared_record_specialization_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/forward_declared_record_specialization_parse.cpp"
)
set_tests_properties(cpp_parse_forward_declared_record_specialization_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "StructDef\\(struct Box__spec_[0-9]+\\) specialize<1>"
)

add_test(
  NAME cpp_parse_forward_declared_self_type_local_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/forward_declared_self_type_local_parse.cpp"
)
set_tests_properties(cpp_parse_forward_declared_self_type_local_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Function\\(operator_mul_assign\\)"
)

add_test(
  NAME cpp_parse_forward_declared_self_type_local_qualified_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/forward_declared_self_type_local_qualified_parse.cpp"
)
set_tests_properties(cpp_parse_forward_declared_self_type_local_qualified_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Function\\(operator_mul_assign\\)"
)

add_test(
  NAME cpp_parse_gcc_type_trait_type_arg_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/gcc_type_trait_type_arg_parse.cpp"
)
set_tests_properties(cpp_parse_gcc_type_trait_type_arg_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Decl\\(can_fill\\)"
)

add_test(
  NAME cpp_parse_qualified_template_operator_assign_expr_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/qualified_template_operator_assign_expr_parse.cpp"
)
set_tests_properties(cpp_parse_qualified_template_operator_assign_expr_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Function\\(operator_assign\\)"
)

add_test(
  NAME cpp_parse_record_template_enum_underlying_typedef_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/record_template_enum_underlying_typedef_parse.cpp"
)
set_tests_properties(cpp_parse_record_template_enum_underlying_typedef_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Decl\\(kind\\)"
)

add_test(
  NAME cpp_parse_trailing_requires_following_member_decl_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/cpp20_trailing_requires_following_member_decl_parse.cpp"
)
set_tests_properties(cpp_parse_trailing_requires_following_member_decl_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Decl\\(inner\\)"
)

add_test(
  NAME cpp_lex_keyword_and_tokens
  COMMAND c4cll --lex-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/keyword_and_parse.cpp"
)
set_tests_properties(cpp_lex_keyword_and_tokens PROPERTIES
  LABELS "internal;positive_case;cpp;lex"
  PASS_REGULAR_EXPRESSION "AMP_AMP 'and'"
)

add_test(
  NAME cpp_parse_keyword_and_operator_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/keyword_and_parse.cpp"
)
set_tests_properties(cpp_parse_keyword_and_operator_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Function\\(operator_and\\)"
)

add_test(
  NAME cpp_lex_keyword_or_tokens
  COMMAND c4cll --lex-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/keyword_or_parse.cpp"
)
set_tests_properties(cpp_lex_keyword_or_tokens PROPERTIES
  LABELS "internal;positive_case;cpp;lex"
  PASS_REGULAR_EXPRESSION "PIPE_PIPE 'or'"
)

add_test(
  NAME cpp_parse_keyword_or_operator_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/keyword_or_parse.cpp"
)
set_tests_properties(cpp_parse_keyword_or_operator_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Function\\(operator_or\\)"
)

add_test(
  NAME cpp_lex_keyword_not_tokens
  COMMAND c4cll --lex-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/keyword_not_parse.cpp"
)
set_tests_properties(cpp_lex_keyword_not_tokens PROPERTIES
  LABELS "internal;positive_case;cpp;lex"
  PASS_REGULAR_EXPRESSION "BANG 'not'"
)

add_test(
  NAME cpp_parse_keyword_not_operator_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/keyword_not_parse.cpp"
)
set_tests_properties(cpp_parse_keyword_not_operator_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Function\\(operator_not\\)"
)

add_test(
  NAME cpp_lex_keyword_not_eq_tokens
  COMMAND c4cll --lex-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/keyword_not_eq_parse.cpp"
)
set_tests_properties(cpp_lex_keyword_not_eq_tokens PROPERTIES
  LABELS "internal;positive_case;cpp;lex"
  PASS_REGULAR_EXPRESSION "BANG_EQUAL 'not_eq'"
)

add_test(
  NAME cpp_parse_keyword_not_eq_operator_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/keyword_not_eq_parse.cpp"
)
set_tests_properties(cpp_parse_keyword_not_eq_operator_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Function\\(operator_neq\\)"
)

add_test(
  NAME cpp_lex_keyword_bitand_tokens
  COMMAND c4cll --lex-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/keyword_bitand_parse.cpp"
)
set_tests_properties(cpp_lex_keyword_bitand_tokens PROPERTIES
  LABELS "internal;positive_case;cpp;lex"
  PASS_REGULAR_EXPRESSION "AMP 'bitand'"
)

add_test(
  NAME cpp_parse_keyword_bitand_operator_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/keyword_bitand_parse.cpp"
)
set_tests_properties(cpp_parse_keyword_bitand_operator_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Function\\(operator_bitand\\)"
)

add_test(
  NAME cpp_lex_keyword_bitor_tokens
  COMMAND c4cll --lex-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/keyword_bitor_parse.cpp"
)
set_tests_properties(cpp_lex_keyword_bitor_tokens PROPERTIES
  LABELS "internal;positive_case;cpp;lex"
  PASS_REGULAR_EXPRESSION "PIPE 'bitor'"
)

add_test(
  NAME cpp_parse_keyword_bitor_operator_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/keyword_bitor_parse.cpp"
)
set_tests_properties(cpp_parse_keyword_bitor_operator_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Function\\(operator_bitor\\)"
)

add_test(
  NAME cpp_lex_keyword_bitxor_tokens
  COMMAND c4cll --lex-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/keyword_bitxor_parse.cpp"
)
set_tests_properties(cpp_lex_keyword_bitxor_tokens PROPERTIES
  LABELS "internal;positive_case;cpp;lex"
  PASS_REGULAR_EXPRESSION "CARET 'bitxor'"
)

add_test(
  NAME cpp_parse_keyword_bitxor_operator_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/keyword_bitxor_parse.cpp"
)
set_tests_properties(cpp_parse_keyword_bitxor_operator_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Function\\(operator_bitxor\\)"
)

add_test(
  NAME cpp_lex_keyword_compl_tokens
  COMMAND c4cll --lex-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/keyword_compl_parse.cpp"
)
set_tests_properties(cpp_lex_keyword_compl_tokens PROPERTIES
  LABELS "internal;positive_case;cpp;lex"
  PASS_REGULAR_EXPRESSION "TILDE 'compl'"
)

add_test(
  NAME cpp_parse_keyword_compl_operator_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/keyword_compl_parse.cpp"
)
set_tests_properties(cpp_parse_keyword_compl_operator_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Function\\(operator_bitnot\\)"
)

add_test(
  NAME cpp_lex_keyword_requires_tokens
  COMMAND c4cll --lex-only "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/cpp20_requires_clause_parse.cpp"
)
set_tests_properties(cpp_lex_keyword_requires_tokens PROPERTIES
  LABELS "internal;positive_case;cpp;lex"
  PASS_REGULAR_EXPRESSION "KW_requires 'requires'"
)

add_test(
  NAME cpp_hir_template_def_dump
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/template_func.cpp"
)
set_tests_properties(cpp_hir_template_def_dump PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "template add<typename T>.*instantiation"
)

add_test(
  NAME cpp_hir_consteval_template_dump
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/consteval_template.cpp"
)
set_tests_properties(cpp_hir_consteval_template_dump PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "template consteval square<typename T>"
)

add_test(
  NAME cpp_hir_template_call_info_dump
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/template_func.cpp"
)
set_tests_properties(cpp_hir_template_call_info_dump PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "add<int>\\(20, 22\\)"
)

add_test(
  NAME cpp_hir_consteval_call_info_dump
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/consteval_func.cpp"
)
set_tests_properties(cpp_hir_consteval_call_info_dump PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "consteval square\\(6\\) = 36"
)

add_test(
  NAME cpp_hir_consteval_template_call_info_dump
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/consteval_template.cpp"
)
set_tests_properties(cpp_hir_consteval_template_call_info_dump PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "consteval square<T=int>\\(6\\) = 36"
)

add_test(
  NAME cpp_hir_compile_time_reduction_stats
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/consteval_template.cpp"
)
set_tests_properties(cpp_hir_compile_time_reduction_stats PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "compile-time reduction:.*\\(converged\\)"
)

add_test(
  NAME cpp_hir_template_instantiation_resolved
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/template_func.cpp"
)
set_tests_properties(cpp_hir_template_instantiation_resolved PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "1 template call resolved.*\\(converged\\)"
)

add_test(
  NAME cpp_hir_consteval_reduction_verified
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/consteval_func.cpp"
)
set_tests_properties(cpp_hir_consteval_reduction_verified PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "1 consteval reduction.*\\(converged\\)"
)

add_test(
  NAME cpp_hir_consteval_template_reduction_verified
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/consteval_template.cpp"
)
set_tests_properties(cpp_hir_consteval_template_reduction_verified PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "1 consteval reduction.*\\(converged\\)"
)

add_test(
  NAME cpp_hir_fixpoint_convergence
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/consteval_nested_template.cpp"
)
set_tests_properties(cpp_hir_fixpoint_convergence PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "1 iteration, 6 template calls resolved, 0 template types resolved, 8 consteval reductions \\(converged\\)"
)

add_test(
  NAME cpp_hir_deferred_template_instantiation
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/template_chain.cpp"
)
set_tests_properties(cpp_hir_deferred_template_instantiation PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "1 deferred instantiation.*\\(converged\\)"
)

add_test(
  NAME cpp_hir_deferred_consteval_chain
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/deferred_consteval_chain.cpp"
)
set_tests_properties(cpp_hir_deferred_consteval_chain PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "deferred consteval reduction.*\\(converged\\)"
)

add_test(
  NAME cpp_hir_deferred_consteval_multi
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/deferred_consteval_multi.cpp"
)
set_tests_properties(cpp_hir_deferred_consteval_multi PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "2 deferred consteval reduction.*\\(converged\\)"
)

add_test(
  NAME cpp_hir_deferred_consteval_incomplete_type
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/deferred_consteval_incomplete_type.cpp"
)
set_tests_properties(cpp_hir_deferred_consteval_incomplete_type PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "consteval get_size<T=struct TensorDesc>\\(\\) = 64.*1 consteval reduction.*\\(converged\\)"
)

add_test(
  NAME cpp_hir_late_specialization_unlocks_wrapper
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/late_specialization_unlocks_wrapper.cpp"
)
set_tests_properties(cpp_hir_late_specialization_unlocks_wrapper PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "consteval schedule\\(\\) = 42.*1 consteval reduction.*\\(converged\\)"
)

add_test(
  NAME cpp_hir_specialization_key
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/template_func.cpp"
)
set_tests_properties(cpp_hir_specialization_key PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "key=add<T=int>"
)

add_test(
  NAME cpp_hir_materialization_stats
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/consteval_template.cpp"
)
set_tests_properties(cpp_hir_materialization_stats PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "materialization:.*function.*materialized"
)

add_test(
  NAME cpp_hir_materialization_boundary
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/materialization_boundary.cpp"
)
set_tests_properties(cpp_hir_materialization_boundary PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "2 compile-time only"
)

add_test(
  NAME cpp_hir_initial_program_seed_realization
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/materialization_boundary.cpp"
)
set_tests_properties(cpp_hir_initial_program_seed_realization PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "compile-time reduction: 1 iteration, 1 template call resolved, 0 template types resolved, 4 consteval reductions \\(converged\\)"
)

add_test(
  NAME cpp_hir_template_origin
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/materialization_boundary.cpp"
)
set_tests_properties(cpp_hir_template_origin PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "template<apply>"
)

add_test(
  NAME cpp_hir_template_struct_registry_primary_only
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/parse/template_struct_specialization_parse.cpp"
)
set_tests_properties(cpp_hir_template_struct_registry_primary_only PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  FAIL_REGULAR_EXPRESSION "struct static_min__spec_|struct remove_const__spec_|struct disjunction__spec_|struct complex_default__spec_"
)

add_test(
  NAME cpp_hir_template_member_owner_resolution
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/template_member_owner_resolution.cpp"
)
set_tests_properties(cpp_hir_template_member_owner_resolution PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "decl w: struct wrap_T_int"
)

add_test(
  NAME cpp_hir_template_struct_field_array_extent
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/template_struct_nttp.cpp"
)
set_tests_properties(cpp_hir_template_struct_field_array_extent PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "field data: int\\[4\\] llvm_idx=0 offset=0 size=16 align=4"
)

add_test(
  NAME cpp_hir_template_function_signature_binding
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/template_fn_struct.cpp"
)
set_tests_properties(cpp_hir_template_function_signature_binding PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "fn make_pair_i\\(a: int, b: int\\) -> struct Pair_T_int"
)

add_test(
  NAME cpp_hir_template_method_signature_binding
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/template_struct_method_tpl_return.cpp"
)
set_tests_properties(cpp_hir_template_method_signature_binding PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "fn Transformer_T_int__make_scaled_pair\\(this: struct Transformer_T_int\\*, x: int\\) -> struct Pair_T_int"
)

add_test(
  NAME cpp_hir_defaulted_destructor_member_teardown
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/default_special_members.cpp"
)
set_tests_properties(cpp_hir_defaulted_destructor_member_teardown PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "WithDtor__dtor\\(\\(&this#P0->inner\\{owner=Outer\\}\\)\\)"
)

add_test(
  NAME cpp_hir_spec_key_identity
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/specialization_identity.cpp"
)
set_tests_properties(cpp_hir_spec_key_identity PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "key=add<T=int>"
)

add_test(
  NAME cpp_hir_spec_key_distinct
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/specialization_identity.cpp"
)
set_tests_properties(cpp_hir_spec_key_distinct PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "key=add<T=double>"
)

add_test(
  NAME cpp_llvm_spec_key_metadata
  COMMAND c4cll "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/specialization_identity.cpp"
)
set_tests_properties(cpp_llvm_spec_key_metadata PROPERTIES
  LABELS "internal;positive_case;cpp;llvm"
  PASS_REGULAR_EXPRESSION "spec-key: add<T=int>"
)

add_test(
  NAME cpp_llvm_spec_key_named_metadata
  COMMAND c4cll "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/specialization_identity.cpp"
)
set_tests_properties(cpp_llvm_spec_key_named_metadata PROPERTIES
  LABELS "internal;positive_case;cpp;llvm"
  PASS_REGULAR_EXPRESSION "!c4c\\.specializations"
)

add_test(
  NAME cpp_llvm_spec_key_named_metadata_entry
  COMMAND c4cll "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/specialization_identity.cpp"
)
set_tests_properties(cpp_llvm_spec_key_named_metadata_entry PROPERTIES
  LABELS "internal;positive_case;cpp;llvm"
  PASS_REGULAR_EXPRESSION "!\"add<T=int>\""
)

add_test(
  NAME cpp_hir_forward_pick_ref_specialization_identity
  COMMAND c4cll --dump-hir "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/template_forward_pick_specialization_identity.cpp"
)
set_tests_properties(cpp_hir_forward_pick_ref_specialization_identity PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "template forward_pick<typename T> \\[2 instantiations\\].*-> forward_pick_i_ref \\{T=int&\\} key=forward_pick<T=int&>.*-> forward_pick_i \\{T=int\\} key=forward_pick<T=int>"
)

add_test(
  NAME cpp_llvm_forward_pick_specialization_metadata
  COMMAND c4cll "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/template_forward_pick_specialization_identity.cpp"
)
set_tests_properties(cpp_llvm_forward_pick_specialization_metadata PROPERTIES
  LABELS "internal;positive_case;cpp;llvm"
  PASS_REGULAR_EXPRESSION "!c4c\\.specializations = !\\{!0, !1\\}"
)

add_test(
  NAME cpp_llvm_forward_pick_ref_specialization_metadata_entry
  COMMAND c4cll "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/template_forward_pick_specialization_identity.cpp"
)
set_tests_properties(cpp_llvm_forward_pick_ref_specialization_metadata_entry PROPERTIES
  LABELS "internal;positive_case;cpp;llvm"
  PASS_REGULAR_EXPRESSION "!\"forward_pick<T=int&>\""
)

add_test(
  NAME cpp_llvm_forward_pick_value_specialization_metadata_entry
  COMMAND c4cll "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/template_forward_pick_specialization_identity.cpp"
)
set_tests_properties(cpp_llvm_forward_pick_value_specialization_metadata_entry PROPERTIES
  LABELS "internal;positive_case;cpp;llvm"
  PASS_REGULAR_EXPRESSION "!\"forward_pick<T=int>\""
)

add_test(
  NAME cpp_llvm_initializer_list_runtime_materialization
  COMMAND c4cll "${INTERNAL_CPP_TEST_ROOT}/postive_case/runtime/eastl_probe_initializer_list_runtime.cpp"
)
set_tests_properties(cpp_llvm_initializer_list_runtime_materialization PROPERTIES
  LABELS "internal;positive_case;cpp;llvm"
  PASS_REGULAR_EXPRESSION "alloca \\[3 x i32\\]"
)
