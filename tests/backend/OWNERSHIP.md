# Backend Test Ownership Inventory

This inventory classifies backend tests and fixtures using the source idea
categories: `bir-live`, `mir-live`, `shared-fixture`, `legacy`,
`deprecated`, and `delete`.

The ownership rule is assertion-target based. BIR text, prepared-BIR behavior,
and semantic route observations are BIR-owned. Target MIR records, AArch64
machine nodes, machine printers, and AArch64 `.c -> .s` smoke proof are
MIR-owned. Files in `tests/backend/case/` are fixture inputs only; their
presence is not active backend coverage unless a CMake test consumes them.

## BIR-Live

- `tests/backend/bir/backend_prepare_*_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`
- `tests/backend/bir/backend_x86_prepared_handoff_label_authority_test.cpp`
- `tests/backend/bir/backend_x86_route_debug_test.cpp`, gated by
  `C4C_ENABLE_X86_BACKEND_TESTS`
- `tests/backend/bir/backend_x86_handoff_boundary*_test.cpp`, gated by
  `C4C_ENABLE_X86_BACKEND_TESTS`
- `backend_cli_dump_bir_*` and `backend_cli_dump_prepared_bir_*` CTest entries
  in `tests/backend/bir/CMakeLists.txt`
- `backend_codegen_route_*_observe_semantic_bir` CTest entries in
  `tests/backend/bir/CMakeLists.txt`
- `backend_runtime_byval_helper_*` CTest entries in
  `tests/backend/bir/CMakeLists.txt`

## MIR-Live

- `tests/backend/mir/backend_aarch64_*_test.cpp`
- `backend_cli_aarch64_asm_no_machine_nodes_fails`
- `backend_cli_aarch64_asm_external_return_zero_smoke`
- `backend_cli_aarch64_asm_external_return_add_smoke`
- `tests/backend/case/aarch64_no_selected_machine_nodes.c`
- `tests/backend/case/aarch64_return_zero_smoke.c`
- `tests/backend/case/return_add.c`, as an input fixture for the live AArch64
  external return-add smoke test

## Shared-Fixture

These files are input data for live backend tests. They remain under
`tests/backend/case/` so BIR and MIR CMake owners can reference them without
treating fixture location as proof ownership.

- `aggregate_member_function_pointer_call.c`
- `aggregate_param_return_pair.c`
- `aggregate_param_return_pair_fn_param.c`
- `aggregate_return_pair.c`
- `builtin_abs_i32.c`
- `builtin_abs_i64.c`
- `builtin_fabs_f32.c`
- `builtin_fabs_f64.c`
- `builtin_fabsl.c`
- `builtin_memcpy_local_i32_array.c`
- `builtin_memcpy_local_i32_array_to_pair.c`
- `builtin_memcpy_local_pair.c`
- `builtin_memcpy_local_pair_to_i32_array.c`
- `builtin_memcpy_nested_i32_array_field.c`
- `builtin_memcpy_nested_pair_field.c`
- `builtin_memcpy_prefix_local_i32_array_to_pair.c`
- `builtin_memcpy_prefix_local_i32_subarray_to_nested_pair_field.c`
- `builtin_memcpy_prefix_local_pair_to_i32_array.c`
- `builtin_memset_local_i32_array.c`
- `builtin_memset_local_pair.c`
- `builtin_memset_nested_i32_array_field.c`
- `builtin_memset_nonzero_local_i32_array.c`
- `builtin_memset_nonzero_local_i32_scalar.c`
- `builtin_memset_nonzero_local_pair.c`
- `builtin_memset_nonzero_nested_i32_array_field.c`
- `builtin_memset_nonzero_nested_i32_scalar_field.c`
- `builtin_memset_nonzero_prefix_local_i32_subarray.c`
- `builtin_memset_nonzero_prefix_nested_i32_subarray_field.c`
- `byval_helper_mixed_hfa_payload.c`
- `byval_helper_payload_8_to_13.c`
- `byval_helper_payload_9_to_14.c`
- `call_helper.c`
- `defined_global_struct_store.c`
- `function_pointer_param_direct_arg.c`
- `global_aggregate_array_function_pointer_call.c`
- `global_aggregate_function_pointer_call.c`
- `global_function_pointer_value_call.c`
- `global_nested_aggregate_array_function_pointer_call.c`
- `global_nested_aggregate_function_pointer_call.c`
- `global_struct_array_read.c`
- `global_struct_array_store.c`
- `indirect_aggregate_param_return_pair.c`
- `indirect_variadic_pair_second.c`
- `indirect_variadic_sum2.c`
- `inline_asm_input_i32.c`
- `inline_asm_input_local_ptr.c`
- `inline_asm_input_ptr.c`
- `inline_asm_input_two_i32.c`
- `inline_asm_nop.c`
- `inline_asm_output_i32.c`
- `inline_asm_output_ptr.c`
- `inline_asm_output_readwrite_i32.c`
- `inline_asm_output_readwrite_ptr.c`
- `local_aggregate_member_offsets.c`
- `local_arg_call.c`
- `local_array.c`
- `local_direct_dynamic_member_array_load.c`
- `local_direct_dynamic_member_array_store.c`
- `local_direct_dynamic_struct_array_call.c`
- `local_dynamic_member_array.c`
- `local_dynamic_member_array_store.c`
- `local_pointer_deref.c`
- `nested_global_struct_array_read.c`
- `nested_global_struct_array_store.c`
- `nested_member_pointer_array.c`
- `nested_param_member_array.c`
- `nested_pointer_param_dynamic_member_array_load.c`
- `packed_local_member_offsets.c`
- `param_member_array.c`
- `pointer_param_direct_global_arg.c`
- `pointer_param_global_pointer_value_arg.c`
- `pointer_param_loaded_char_deref.c`
- `pointer_return_direct.c`
- `pointer_return_fn_param.c`
- `pointer_return_global_pointer_value.c`
- `three_way_phi_merge_post_add_sub.c`
- `variadic_double_bytes.c`
- `variadic_pair_second.c`
- `variadic_sum2.c`
- `variadic_va_copy_accumulate.c`
- `vla_goto_stackrestore.c`

## Legacy

These fixtures are retained source corpus inventory. They are not active BIR
or MIR proof unless a future CMake test consumes them under an explicit owner.

- `anonymous_global_struct_fields.c`
- `branch_if_eq.c`
- `branch_if_ge.c`
- `branch_if_gt.c`
- `branch_if_le.c`
- `branch_if_lt.c`
- `branch_if_ne.c`
- `branch_if_uge.c`
- `branch_if_ugt.c`
- `branch_if_ule.c`
- `branch_if_ult.c`
- `call_helper_def.c`
- `defined_global_array.c`
- `defined_global_array_pointer.c`
- `defined_global_array_pointer_store.c`
- `defined_global_array_store.c`
- `defined_pointer_global_array.c`
- `defined_pointer_global_array_offset.c`
- `defined_pointer_global_array_store.c`
- `defined_pointer_global_nested_struct_array.c`
- `defined_pointer_global_nested_struct_array_object_alias.c`
- `defined_pointer_global_pointer.c`
- `defined_pointer_global_pointer_offset.c`
- `defined_pointer_global_pointer_pointer.c`
- `defined_pointer_global_pointer_pointer_value_store.c`
- `defined_pointer_global_pointer_store.c`
- `defined_pointer_global_pointer_value_store.c`
- `defined_pointer_global_root_array_struct_field.c`
- `defined_pointer_global_root_array_struct_field_object_alias.c`
- `defined_pointer_global_string_pointer.c`
- `defined_pointer_global_string_pointer_store.c`
- `defined_pointer_global_struct_array.c`
- `defined_pointer_global_struct_array_object_alias.c`
- `defined_string_global_char.c`
- `defined_string_global_store.c`
- `extern_global_array.c`
- `extern_global_array_def.c`
- `extern_global_array_store.c`
- `global_char_pointer_diff.c`
- `global_int_pointer_diff.c`
- `global_int_pointer_pointer_roundtrip_store.c`
- `global_int_pointer_roundtrip.c`
- `global_int_pointer_roundtrip_store.c`
- `global_load.c`
- `global_load_zero_init.c`
- `global_nested_struct_array_alias_store.c`
- `global_nested_struct_array_read.c`
- `global_nested_struct_array_store.c`
- `global_nested_struct_pointer_array_alias_store.c`
- `global_nested_struct_pointer_array_read.c`
- `global_nested_struct_pointer_array_store.c`
- `global_store.c`
- `global_struct_pointer_array_alias_init.c`
- `global_struct_pointer_array_alias_store.c`
- `global_struct_pointer_array_object_alias_init.c`
- `global_struct_pointer_array_read.c`
- `global_struct_pointer_array_store.c`
- `local_temp.c`
- `named_global_struct_designated_init.c`
- `named_pointer_global_struct_designated_init.c`
- `named_pointer_global_struct_pointer_alias_init.c`
- `named_pointer_global_struct_pointer_field_alias_store.c`
- `named_pointer_global_struct_pointer_field_rewrite.c`
- `named_pointer_global_struct_pointer_object_alias_init.c`
- `named_pointer_global_struct_pointer_store.c`
- `nested_global_struct_array_alias_store.c`
- `nested_global_struct_pointer_alias_init.c`
- `nested_global_struct_pointer_alias_store.c`
- `nested_global_struct_pointer_object_alias_init.c`
- `nested_global_struct_pointer_read.c`
- `nested_global_struct_pointer_rewrite.c`
- `nested_global_struct_pointer_store.c`
- `param_slot.c`
- `return_add_sub_chain.c`
- `return_zero.c`
- `string_literal_char.c`
- `two_arg_both_local_arg.c`
- `two_arg_both_local_double_rewrite.c`
- `two_arg_both_local_first_rewrite.c`
- `two_arg_both_local_second_rewrite.c`
- `two_arg_first_local_rewrite.c`
- `two_arg_helper.c`
- `two_arg_local_arg.c`
- `two_arg_second_local_arg.c`
- `two_arg_second_local_rewrite.c`

## Deprecated

The `--dump-mir` and `--trace-mir` x86 debug-route CTest placeholders were
deprecated because their assertions checked `route owner: x86/debug`,
`module emitter: x86/module`, and contract-first debug text. They did not prove
AArch64 MIR, target MIR records, machine-node lowering, or machine-printer
behavior. They also stayed disabled by default through
`C4C_ENABLE_X86_BACKEND_TESTS`, which made them misleading as MIR coverage.

## Delete

The following disabled root `tests/backend/CMakeLists.txt` placeholders were
removed. Deletion reason for each entry: stale x86/debug route placeholder for
`--dump-mir` or `--trace-mir`; not an active MIR contract; not converted into
`tests/backend/mir` because it does not prove target MIR behavior.

- `backend_cli_dump_mir_is_nonfatal_trace_shell`
- `backend_cli_trace_mir_reports_lane_detail`
- `backend_cli_dump_mir_focus_function_filters_00204`
- `backend_cli_trace_mir_focus_function_filters_00204`
- `backend_cli_dump_mir_focus_block_entry_00204`
- `backend_cli_trace_mir_focus_block_entry_00204`
- `backend_cli_dump_mir_focus_block_missing_00204`
- `backend_cli_trace_mir_focus_block_missing_00204`
- `backend_cli_dump_mir_focus_value_t1_00204`
- `backend_cli_dump_mir_focus_value_missing_00204`
- `backend_cli_trace_mir_focus_value_t1_00204`
- `backend_cli_trace_mir_focus_value_missing_00204`
