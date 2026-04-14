set(INTERNAL_CPP_TEST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(INTERNAL_C_TEST_CMAKE_ROOT "${PROJECT_SOURCE_DIR}/tests/c/internal/cmake")

file(GLOB INTERNAL_CPP_POSITIVE_TEST_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_CPP_TEST_ROOT}/postive_case/*.cpp")
file(GLOB INTERNAL_CPP_NEGATIVE_TEST_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_CPP_TEST_ROOT}/negative_case/*.cpp")
file(GLOB_RECURSE INTERNAL_CPP_GENERATED_PARSE_ONLY_TEST_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_CPP_TEST_ROOT}/generated/parser_disambiguation_matrix/parse_only/*.cpp")
file(GLOB_RECURSE INTERNAL_CPP_GENERATED_COMPILE_POSITIVE_TEST_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_CPP_TEST_ROOT}/generated/parser_disambiguation_matrix/compile_positive/*.cpp")

list(APPEND INTERNAL_CPP_POSITIVE_TEST_SRCS ${INTERNAL_CPP_GENERATED_PARSE_ONLY_TEST_SRCS})
list(APPEND INTERNAL_CPP_POSITIVE_TEST_SRCS ${INTERNAL_CPP_GENERATED_COMPILE_POSITIVE_TEST_SRCS})

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
    cpp20_if_constexpr_concept_id_frontend
    free_operator_eq_overload_frontend
    ctor_init_piecewise_delegating_template_runtime
)

set(CPP_POSITIVE_PARSE_STEMS
    alignas_symbol_parse
    operator_decl_subscript_parse
    operator_decl_subscript_attr_parse
    operator_decl_bitwise_assign_parse
    noexcept_bool_qualified_template_call_parse
    qualified_template_call_expr_parse
    functional_cast_shift_expr_parse
    typedef_owned_functional_cast_parse
    qualified_known_type_arg_fast_path_parse
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
    operator_conversion_template_id_refqual_parse
    operator_shift_static_member_call_parse
    template_anon_param_extern_cxx_parse
    template_bool_specialization_parse
    local_value_shadows_tag_assign_expr_parse
    local_value_shadows_tag_compound_assign_expr_parse
    local_value_shadows_typedef_assign_expr_parse
    local_value_shadows_using_alias_assign_expr_parse
    ctor_init_dependent_typename_index_tuple_parse
    ctor_init_out_of_class_dependent_typename_index_tuple_parse
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
    template_template_scope_member_parse
    template_unresolved_param_type_parse
    qualified_template_unresolved_param_type_parse
    unresolved_by_value_param_type_parse
    template_friend_record_member_parse
    template_member_type_direct_parse
    template_member_typedef_cache_roundtrip_parse
    template_member_type_inherited_parse
    dependent_template_typename_member_parse
    record_base_template_member_alias_parse
    template_alias_nttp_expr_parse
    template_alias_nttp_expr_inherited_parse
    inherited_type_alias_base_member_lookup_parse
    namespaced_inherited_type_alias_base_member_lookup_parse
    template_argument_loop_staging_parse
    template_argument_expr_close_staging_parse
    template_declaration_prelude_cleanup_parse
    template_alias_qualified_typedef_resolution_parse
    template_visible_typedef_type_arg_parse
    template_specialization_member_typedef_trait_parse
    template_specialization_typedef_chain_parse
    template_specialization_visible_typedef_chain_parse
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
    namespace_using_decl_template_typedef_type_arg_parse
    using_namespace_directive_parse
    using_nested_namespace_parse
    template_conversion_operator_parse
    template_struct_specialization_parse
    forward_declared_record_specialization_parse
    forward_declared_self_type_local_parse
    forward_declared_self_type_local_qualified_parse
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
    friend_relational_operator_parse
    friend_spaceship_operator_parse
    if_condition_decl_parse
    for_decl_probe_rollback_parse
    free_function_record_ref_param_parse
    iterator_concepts_following_hash_base_parse
    stl_iterator_then_max_size_type_parse
    cpp20_requires_clause_parse
    cpp20_requires_clause_struct_decl_parse
    cpp20_trailing_requires_following_member_decl_parse
    cpp20_requires_trait_disjunction_member_parse
    cpp20_requires_concept_id_member_ctor_parse
    cpp20_requires_conjunction_member_parse
    cpp20_namespaced_concept_requires_parse
    cpp20_requires_requires_member_array_param_parse
    cpp20_requires_expression_if_constexpr_parse
    cpp20_if_likely_stmt_attr_parse
    eastl_slice7d_qualified_declarator_parse
    eastl_function_detail_allocator_member_call_parse
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
    qualified_variable_template_compare_parse
    qualified_trait_value_template_arg_parse
    qualified_template_call_template_arg_parse
    template_deduction_guide_rvalue_ref_parse
    template_known_type_arg_fast_path_parse
    step3_timeout_probe_baseline_parse
    tuple_element_alias_mix_parse
    qualified_template_operator_assign_expr_parse
    qualified_typedef_operator_assign_expr_parse
    global_qualified_member_pointer_template_owner_parse
    declarator_array_suffix_staging_parse
    declarator_grouped_suffix_staging_parse
    declarator_pointer_qualifier_staging_parse
    declarator_pointer_ref_qualifier_staging_parse
    declarator_parenthesized_fn_ptr_staging_parse
    declarator_member_fn_ptr_suffix_staging_parse
    c_style_cast_template_fn_ptr_return_type_parse
    c_style_cast_template_fn_ptr_lvalue_ref_return_type_parse
    c_style_cast_template_member_fn_ptr_const_parse
    c_style_cast_template_member_fn_ptr_ref_qual_parse
    c_style_cast_global_qualified_template_member_fn_ptr_const_parse
    c_style_cast_namespace_qualified_template_member_fn_ptr_const_parse
    c_style_cast_namespace_qualified_template_member_fn_ptr_ref_qual_parse
    c_style_cast_global_qualified_dependent_template_member_fn_ptr_const_parse
    c_style_cast_global_qualified_dependent_template_member_fn_ptr_ref_qual_parse
    c_style_cast_namespace_qualified_dependent_template_member_fn_ptr_ref_qual_parse
    declarator_parenthesized_member_pointer_prefix_parse
    declarator_parenthesized_pointer_inner_staging_parse
    declarator_parenthesized_member_pointer_finalization_parse
    declarator_normal_tail_staging_parse
    declarator_non_parenthesized_suffix_staging_parse
    declarator_plain_function_suffix_staging_parse
    c_style_cast_dependent_template_member_ptr_parse
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
    record_forward_specialization_decl_parse
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
    member_spaceship_operator_parse
    lambda_empty_capture_parse
    lambda_empty_capture_parens_parse
    lambda_ref_capture_parse
    lambda_copy_capture_parse
)

list(APPEND CPP_POSITIVE_FRONTEND_STEMS
    call_expr_ref_return_lvalue_frontend
    constrained_member_conversion_operator_frontend
    constrained_template_method_call_frontend
    eastl_probe_call_result_lvalue_frontend
    namespace_using_decl_typedef_and_value_frontend
    namespaced_function_param_shadow_frontend
    namespaced_out_of_class_method_context_frontend
    qualified_namespaced_out_of_class_method_context_frontend
    operator_new_delete_array_placement_overload_frontend
    template_inline_method_member_context_frontend
)

foreach(src IN LISTS INTERNAL_CPP_GENERATED_PARSE_ONLY_TEST_SRCS)
  get_filename_component(stem "${src}" NAME_WE)
  list(APPEND CPP_POSITIVE_PARSE_STEMS "${stem}")
endforeach()

foreach(src IN LISTS INTERNAL_CPP_GENERATED_COMPILE_POSITIVE_TEST_SRCS)
  get_filename_component(stem "${src}" NAME_WE)
  list(APPEND CPP_POSITIVE_FRONTEND_STEMS "${stem}")
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

add_subdirectory(postive_case)
add_subdirectory(parse_only_case)
add_subdirectory(negative_case)
add_subdirectory(hir_case)
add_subdirectory(consteval_case)

add_test(
  NAME cpp_eastl_piecewise_construct_parse_recipe
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DROOT=${PROJECT_SOURCE_DIR}
          -DSRC=${PROJECT_SOURCE_DIR}/tests/cpp/eastl/eastl_piecewise_construct_simple.cpp
          -DEXPECT_SUCCESS=ON
          -P "${PROJECT_SOURCE_DIR}/tests/cpp/eastl/run_eastl_parse_recipe.cmake"
)
set_tests_properties(cpp_eastl_piecewise_construct_parse_recipe PROPERTIES
  LABELS "internal;positive_case;cpp;workflow"
  TIMEOUT 30
)

add_test(
  NAME cpp_eastl_tuple_fwd_decls_parse_recipe
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DROOT=${PROJECT_SOURCE_DIR}
          -DSRC=${PROJECT_SOURCE_DIR}/tests/cpp/eastl/eastl_tuple_fwd_decls_simple.cpp
          -DEXPECT_SUCCESS=ON
          -P "${PROJECT_SOURCE_DIR}/tests/cpp/eastl/run_eastl_parse_recipe.cmake"
)
set_tests_properties(cpp_eastl_tuple_fwd_decls_parse_recipe PROPERTIES
  LABELS "internal;positive_case;cpp;workflow"
  TIMEOUT 30
)

add_test(
  NAME cpp_eastl_integer_sequence_parse_recipe
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DROOT=${PROJECT_SOURCE_DIR}
          -DSRC=${PROJECT_SOURCE_DIR}/tests/cpp/eastl/eastl_integer_sequence_simple.cpp
          -DEXPECT_SUCCESS=ON
          -P "${PROJECT_SOURCE_DIR}/tests/cpp/eastl/run_eastl_parse_recipe.cmake"
)
set_tests_properties(cpp_eastl_integer_sequence_parse_recipe PROPERTIES
  LABELS "internal;positive_case;cpp;workflow"
  TIMEOUT 30
)

add_test(
  NAME cpp_eastl_type_traits_parse_recipe
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DROOT=${PROJECT_SOURCE_DIR}
          -DSRC=${PROJECT_SOURCE_DIR}/tests/cpp/eastl/eastl_type_traits_simple.cpp
          -DEXPECT_SUCCESS=ON
          -P "${PROJECT_SOURCE_DIR}/tests/cpp/eastl/run_eastl_parse_recipe.cmake"
)
set_tests_properties(cpp_eastl_type_traits_parse_recipe PROPERTIES
  LABELS "internal;positive_case;cpp;workflow"
  TIMEOUT 30
)

add_test(
  NAME cpp_eastl_utility_parse_recipe
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DROOT=${PROJECT_SOURCE_DIR}
          -DSRC=${PROJECT_SOURCE_DIR}/tests/cpp/eastl/eastl_utility_simple.cpp
          -DEXPECT_SUCCESS=ON
          -P "${PROJECT_SOURCE_DIR}/tests/cpp/eastl/run_eastl_parse_recipe.cmake"
)
set_tests_properties(cpp_eastl_utility_parse_recipe PROPERTIES
  LABELS "internal;positive_case;cpp;workflow"
  TIMEOUT 30
  RUN_SERIAL TRUE
)

add_test(
  NAME cpp_eastl_vector_parse_recipe
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DROOT=${PROJECT_SOURCE_DIR}
          -DSRC=${PROJECT_SOURCE_DIR}/tests/cpp/eastl/eastl_vector_simple.cpp
          -DCASE_TIMEOUT_SEC=25
          -DEXPECT_SUCCESS=ON
          -P "${PROJECT_SOURCE_DIR}/tests/cpp/eastl/run_eastl_parse_recipe.cmake"
)
set_tests_properties(cpp_eastl_vector_parse_recipe PROPERTIES
  LABELS "internal;positive_case;cpp;workflow"
  TIMEOUT 45
  RUN_SERIAL TRUE
)

add_test(
  NAME cpp_eastl_memory_uses_allocator_parse_recipe
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DROOT=${PROJECT_SOURCE_DIR}
          -DSRC=${PROJECT_SOURCE_DIR}/tests/cpp/eastl/eastl_memory_uses_allocator_frontier.cpp
          -DCASE_TIMEOUT_SEC=25
          -DEXPECT_SUCCESS=ON
          -P "${PROJECT_SOURCE_DIR}/tests/cpp/eastl/run_eastl_parse_recipe.cmake"
)
set_tests_properties(cpp_eastl_memory_uses_allocator_parse_recipe PROPERTIES
  LABELS "internal;positive_case;cpp;workflow"
  TIMEOUT 35
  RUN_SERIAL TRUE
)

add_test(
  NAME cpp_std_vector_parse_debug_progress
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${PROJECT_SOURCE_DIR}/tests/cpp/std/std_vector_simple.cpp
          -DEXPECT_TIMEOUT_SEC=5
          "-DPARSER_DEBUG_ARGS:STRING=--parser-debug"
          "-DEXPECT_STDERR_SUBSTRING:STRING=[pdebug] progress"
          -P "${PROJECT_SOURCE_DIR}/tests/cpp/std/run_std_vector_parse_baseline.cmake"
)
set_tests_properties(cpp_std_vector_parse_debug_progress PROPERTIES
  LABELS "internal;positive_case;cpp;workflow;diagnostic_format"
  TIMEOUT 15
)

add_test(
  NAME cpp_std_vector_parse_debug_progress_file
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${PROJECT_SOURCE_DIR}/tests/cpp/std/std_vector_simple.cpp
          -DEXPECT_TIMEOUT_SEC=5
          "-DPARSER_DEBUG_ARGS:STRING=--parser-debug"
          "-DEXPECT_STDERR_SUBSTRING:STRING=file="
          -P "${PROJECT_SOURCE_DIR}/tests/cpp/std/run_std_vector_parse_baseline.cmake"
)
set_tests_properties(cpp_std_vector_parse_debug_progress_file PROPERTIES
  LABELS "internal;positive_case;cpp;workflow;diagnostic_format"
  TIMEOUT 15
)

set(_perf_guard "${PROJECT_SOURCE_DIR}/tests/cpp/internal/run_perf_guard.cmake")
set(_perf_gen  "${PROJECT_SOURCE_DIR}/tests/cpp/internal/perf_gen.py")

add_test(
  NAME cpp_typedef_owned_functional_cast_perf
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DBINARY_DIR=${PROJECT_BINARY_DIR}
          -DREPEAT_COUNT=5000
          -DEXPECT_TIMEOUT_SEC=5
          -DTEST_NAME=typedef_owned_functional_cast
          -DGENERATOR=${_perf_gen}
          -P "${_perf_guard}"
)
set_tests_properties(cpp_typedef_owned_functional_cast_perf PROPERTIES
  LABELS "internal;positive_case;cpp;workflow;parser_perf"
  TIMEOUT 15
)

add_test(
  NAME cpp_qualified_known_type_arg_perf
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DBINARY_DIR=${PROJECT_BINARY_DIR}
          -DREPEAT_COUNT=5000
          -DEXPECT_TIMEOUT_SEC=5
          -DTEST_NAME=qualified_known_type_arg
          -DGENERATOR=${_perf_gen}
          -P "${_perf_guard}"
)
set_tests_properties(cpp_qualified_known_type_arg_perf PROPERTIES
  LABELS "internal;positive_case;cpp;workflow;parser_perf"
  TIMEOUT 15
)

add_test(
  NAME cpp_qualified_template_call_template_arg_perf
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DBINARY_DIR=${PROJECT_BINARY_DIR}
          -DREPEAT_COUNT=5000
          -DEXPECT_TIMEOUT_SEC=5
          -DTEST_NAME=qualified_template_call_template_arg
          -DGENERATOR=${_perf_gen}
          -P "${_perf_guard}"
)
set_tests_properties(cpp_qualified_template_call_template_arg_perf PROPERTIES
  LABELS "internal;positive_case;cpp;workflow;parser_perf"
  TIMEOUT 15
)
