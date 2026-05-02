# HIR dump tests using hir_case/ source files -- extracted from root CMakeLists.txt

add_test(
  NAME cpp_hir_if_constexpr_branch_unlocks_later
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/if_constexpr_branch_unlocks_later_hir.cpp"
)
set_tests_properties(cpp_hir_if_constexpr_branch_unlocks_later PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "consteval choose_tile<T=struct TileShape>\\(\\) = 128.*global tile: const int const = 128"
)

add_test(
  NAME cpp_hir_multistage_shape_chain
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/multistage_shape_chain_hir.cpp"
)
set_tests_properties(cpp_hir_multistage_shape_chain PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "consteval vec_cost<T=struct F32x8>\\(\\) = 24.*global cost: const int const = 24"
)

add_test(
  NAME cpp_hir_template_struct_inherited_method_binding
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/template_struct_inherited_method_hir.cpp"
)
set_tests_properties(cpp_hir_template_struct_inherited_method_binding PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "fn Wrap_T_int__sum_const\\(this: struct Wrap_T_int\\*\\) -> int"
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
  NAME cpp_hir_template_alias_member_owner
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/template_alias_member_owner_hir.cpp"
)
set_tests_properties(cpp_hir_template_alias_member_owner PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "fn project_i\\(value: int\\) -> \\?.*decl local: \\? = value#P0.*return local#L0"
)

add_test(
  NAME cpp_hir_template_member_owner_field_and_local
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/template_member_owner_field_and_local_hir.cpp"
)
set_tests_properties(cpp_hir_template_member_owner_field_and_local PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "field value: int llvm_idx=0 offset=0 size=4 align=4.*decl local: int = input#P0"
)

add_test(
  NAME cpp_hir_template_member_owner_signature_local
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/template_member_owner_signature_local_hir.cpp"
)
set_tests_properties(cpp_hir_template_member_owner_signature_local PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "fn bounce_i\\(input: int\\) -> int.*decl local: int = input#P0.*return local#L0"
)

add_test(
  NAME cpp_hir_expr_call_member_helper
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/hir_expr_call_member_helper_hir.cpp"
)
set_tests_properties(cpp_hir_expr_call_member_helper PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "Box__Box\\(\\(&local#L0\\), \\(&3\\)\\).*Box__read\\(\\(&local#L0\\)\\).*decl __ctor_tmp_1: struct Box.*Box__operator_call\\(\\(&__ctor_tmp_1#L1\\), \\(&__rref_arg_tmp#L2\\)\\)"
)

add_test(
  NAME cpp_hir_function_return_temporary_member_call
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/function_return_temporary_member_call_hir.cpp"
)
set_tests_properties(cpp_hir_function_return_temporary_member_call PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "decl local: struct Box.*Box__Box\\(\\(&local#L1\\), \\(&3\\)\\).*decl __member_call_tmp_[0-9]+: struct Box = make_box\\(\\).*return \\(Box__read\\(\\(&local#L1\\)\\) \\+ Box__read__rref\\(\\(&__member_call_tmp_[0-9]+#L[0-9]+\\)\\)\\)"
)

add_test(
  NAME cpp_hir_expr_object_materialization_helper
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/hir_expr_object_materialization_helper_hir.cpp"
)
set_tests_properties(cpp_hir_expr_object_materialization_helper PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "decl local: struct Box.*Box__Box\\(\\(&local#L0\\), \\(&3\\)\\).*decl <clit>: const int\\[3\\] = 0.*decl __init_list_arg_[0-9]+: struct std::initializer_list_T_int = 0.*decl __new_tmp_[0-9]+: struct Box\\* = \\(\\(struct Box\\*\\)operator_new\\(sizeof\\(struct Box\\)\\)\\).*Box__Box\\(__new_tmp_[0-9]+#L[0-9]+, 5\\).*operator_delete\\(\\(\\(void\\*\\)heap#L[0-9]+\\)\\)"
)

add_test(
  NAME cpp_hir_expr_operator_member_helper
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/hir_expr_operator_member_helper_hir.cpp"
)
set_tests_properties(cpp_hir_expr_operator_member_helper PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "Wrapper__operator_bool\\(\\(&w#L1\\)\\).*Wrapper__operator_arrow\\(\\(&w#L1\\)\\).*return Wrapper__operator_arrow\\(\\(&w#L1\\)\\)->value"
)

add_test(
  NAME cpp_hir_expr_scalar_control_helper
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/hir_expr_scalar_control_helper_hir.cpp"
)
set_tests_properties(cpp_hir_expr_scalar_control_helper PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "decl local: int = \\(input#P1 \\+ bias\\).*\\(local#L1 \\+= this#P0->base\\{owner=Acc\\}\\).*decl __tern_tmp: int = 0.*decl picked: int = __tern_tmp#L3.*\\(\\(__real__z#L6\\) = 3\\).*\\(\\(__imag__z#L6\\) = 4\\).*return .*values#L7\\[1\\].*sizeof\\(values#L7\\[0\\]\\)"
)

add_test(
  NAME cpp_hir_stmt_control_flow_helper
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/hir_stmt_control_flow_helper_hir.cpp"
)
set_tests_properties(cpp_hir_stmt_control_flow_helper PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "decl total: int = 0.*if \\(\\(limit#P0 > 3\\)\\) then=block#.*else block#.*\\(total#L0 = limit#P0\\).*\\(total#L0 = 3\\).*while \\(\\(total#L0 < 8\\)\\) body=block#.*\\(total#L0 = \\(total#L0 \\+ 1\\)\\).*for \\(decl i: int = 0; \\(i#L1 < 2\\); \\(i#L1 = \\(i#L1 \\+ 1\\)\\)\\) body=block#.*\\(total#L0 = \\(total#L0 \\+ i#L1\\)\\).*do block#.*while \\(\\(total#L0 > 8\\)\\).*return total#L0"
)

add_test(
  NAME cpp_hir_stmt_range_for_helper
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/hir_stmt_range_for_helper_hir.cpp"
)
set_tests_properties(cpp_hir_stmt_range_for_helper PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "decl __range_begin: struct Iter = Bag__begin_const\\(\\(&bag#P0\\)\\).*decl __range_end: struct Iter = Bag__end_const\\(\\(&bag#P0\\)\\).*for \\(; Iter__operator_neq\\(\\(&__range_begin#L1\\), __range_end#L2\\); Iter__operator_preinc\\(\\(&__range_begin#L1\\)\\)\\) body=block#.*decl value: int = Iter__operator_deref_const\\(\\(&__range_begin#L1\\)\\).*return total#L0"
)

add_test(
  NAME cpp_hir_stmt_switch_helper
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/hir_stmt_switch_helper_hir.cpp"
)
set_tests_properties(cpp_hir_stmt_switch_helper PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "decl first: const int = 1.*decl second: const int = \\(first#L0 \\+ 1\\).*switch \\(x#P0\\) body=block#.*return result#L2.*case 1:.*\\(result#L2 = 10\\).*break.*case 2:.*\\(result#L2 = 20\\).*break.*default:.*\\(result#L2 = 30\\).*break"
)

add_test(
  NAME cpp_hir_template_method_param_reentrant_lowering
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/template_struct_method_param_reentrant_hir.cpp"
)
set_tests_properties(cpp_hir_template_method_param_reentrant_lowering PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "fn Outer_T_int__absorb\\(this: struct Outer_T_int\\*, inner: struct Inner_T_int\\) -> void"
)

add_test(
  NAME cpp_hir_template_struct_body_instantiation
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/template_struct_body_instantiation_hir.cpp"
)
set_tests_properties(cpp_hir_template_struct_body_instantiation PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "struct Buffer_T_int_N_3 size=20 align=4.*field data: int\\[3\\] llvm_idx=0 offset=4 size=12 align=4.*field tail: int llvm_idx=1 offset=16 size=4 align=4.*fn Buffer_T_int_N_3__sum_const\\(this: struct Buffer_T_int_N_3\\*\\) -> int.*return \\(\\(this#P0->value\\{owner=Buffer_T_int_N_3\\} \\+ this#P0->data\\{owner=Buffer_T_int_N_3\\}\\[1\\]\\) \\+ this#P0->tail\\{owner=Buffer_T_int_N_3\\}\\)"
)

add_test(
  NAME cpp_hir_template_struct_arg_materialization
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/template_struct_arg_materialization_hir.cpp"
)
set_tests_properties(cpp_hir_template_struct_arg_materialization PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "struct Holder_T_short_N_2 size=4 align=2.*field data: short\\[2\\] llvm_idx=0 offset=0 size=4 align=2.*struct Holder_T_int_N_4 size=16 align=4.*field data: int\\[4\\] llvm_idx=0 offset=0 size=16 align=4"
)

add_test(
  NAME cpp_hir_template_function_deduction_binding
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/template_function_deduction_binding_hir.cpp"
)
set_tests_properties(cpp_hir_template_function_deduction_binding PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "template forward_pick<typename T> \\[2 instantiations\\].*forward_pick_Ttag_ctx-1_local_text[0-9]+_ref \\{T=struct Probe&\\} key=forward_pick<T=struct\\.tag\\.ctx-1\\.local\\.text[0-9]+&>.*forward_pick_Ttag_ctx-1_local_text[0-9]+ \\{T=struct Probe\\} key=forward_pick<T=struct\\.tag\\.ctx-1\\.local\\.text[0-9]+>.*template read_ptr<typename T> \\[1 instantiation\\].*read_ptr_Ttag_ctx-1_local_text[0-9]+ \\{T=struct Probe\\} key=read_ptr<T=struct\\.tag\\.ctx-1\\.local\\.text[0-9]+>"
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
  NAME cpp_hir_stmt_local_decl_helper
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/hir_stmt_local_decl_helper_hir.cpp"
)
set_tests_properties(cpp_hir_stmt_local_decl_helper PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "decl seed: struct Box.*Box__Box\\(\\(&seed#L0\\)\\).*decl copy: struct Box.*Box__Box__1\\(\\(&copy#L1\\), \\(&seed#L0\\)\\).*decl data: int\\[3\\] = 0.*\\(data#L2\\[0\\] = 1\\).*\\(data#L2\\[1\\] = 2\\).*\\(data#L2\\[2\\] = 3\\).*decl __rref_tmp_[0-9]+: int = 7.*decl moved: int\\*&& = \\(&__rref_tmp_[0-9]+#L4\\).*return \\(\\(copy#L1.value\\{owner=Box\\} \\+ data#L2\\[1\\]\\) \\+ \\(\\*moved#L3\\)\\)"
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
  NAME cpp_hir_template_deferred_nttp_cast_static_member_expr
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/template_deferred_nttp_cast_static_member_expr_hir.cpp"
)
set_tests_properties(cpp_hir_template_deferred_nttp_cast_static_member_expr PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "field data: int\\[2\\].*size=8 align=4"
)

add_test(
  NAME cpp_hir_template_inherited_member_typedef_trait
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/template_inherited_member_typedef_trait_hir.cpp"
)
set_tests_properties(cpp_hir_template_inherited_member_typedef_trait PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "global ns::is_reference_v_T_Tns::add_lvalue_reference_T_int: const bool.* const = 1"
)

add_test(
  NAME cpp_hir_template_value_arg_static_member_trait
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/template_value_arg_static_member_trait_hir.cpp"
)
set_tests_properties(cpp_hir_template_value_arg_static_member_trait PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "field data: int\\[4\\].*size=16 align=4"
)

add_test(
  NAME cpp_hir_template_global_specialization
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/template_global_specialization_hir.cpp"
)
set_tests_properties(cpp_hir_template_global_specialization PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "global ns::value_T_i: int.* = 4"
)

add_test(
  NAME cpp_hir_module_decl_lookup_structured_mirror
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/module_decl_lookup_structured_mirror_hir.cpp"
)
set_tests_properties(cpp_hir_module_decl_lookup_structured_mirror PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "module decl lookup hits.*global api::value via global-id#[0-9]+.*function api::bump via structured#[0-9]+.*global api::value: int ns=api\\(ctx=[0-9]+\\) = 7.*fn api::bump\\(x: int\\) -> int ns=api\\(ctx=[0-9]+\\).*return \\(x#P0 \\+ api::value#G[0-9]+ lookup=global:global-id#[0-9]+\\).*decl local: int = api::value#G[0-9]+ ns=api\\(ctx=[0-9]+\\) lookup=global:global-id#[0-9]+.*return api::bump ns=api\\(ctx=[0-9]+\\) lookup=function:link-name#[0-9]+\\(local#L[0-9]+\\)"
  FAIL_REGULAR_EXPRESSION "module decl lookup parity mismatches"
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
  PASS_REGULAR_EXPRESSION "fn alignof_querybox\\(\\) -> int.*return 16"
)

add_test(
  NAME cpp_hir_builtin_layout_query_alignof_expr
  COMMAND c4cll --dump-hir "${PROJECT_SOURCE_DIR}/tests/cpp/internal/hir_case/builtin_layout_queries_hir.cpp"
)
set_tests_properties(cpp_hir_builtin_layout_query_alignof_expr PROPERTIES
  LABELS "internal;positive_case;cpp;hir"
  PASS_REGULAR_EXPRESSION "decl box: struct QueryBox.*return 16"
)
