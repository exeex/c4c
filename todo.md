Status: Active
Source Idea Path: ideas/open/111_lir_struct_decl_printer_authority_switch.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Run Focused LLVM Parity Coverage

# Current Packet

## Just Finished

Step 4 - Run Focused LLVM Parity Coverage is complete.

The supervisor-selected focused LLVM parity coverage after the struct
declaration printer authority switch passed. The command rebuilt `c4cll` and
the four frontend LIR type-reference targets, then ran the delegated LLVM,
variadic ABI, split-LLVM, template, record-layout, and builtin-layout CTest
subset.

## Suggested Next

Delegate Step 5 as a broader validation checkpoint. Suggested packet: run the
supervisor-selected broader repo-native validation, preserve the canonical log
state requested by the supervisor, and record whether the authority switch is
ready for lifecycle review or closure.

## Watchouts

- Keep `type_decls` populated as legacy proof/backcompat data.
- Leave verifier shadow/parity checks active.
- Do not broaden the switch to global, function, extern, call, backend, BIR,
  MIR, or layout lookup authority.
- Do not rewrite expectations or downgrade tests as proof.
- Focused parity coverage is green, but Step 5 should still decide whether a
  broader validation checkpoint is needed before lifecycle review or closure.

## Proof

Supervisor-selected proof passed, and the full output is preserved in
`test_after.log`.

Command:
`{ cmake --build build --target c4cll frontend_lir_global_type_ref_test frontend_lir_function_signature_type_ref_test frontend_lir_extern_decl_type_ref_test frontend_lir_call_type_ref_test && ctest --test-dir build -R '^(frontend_lir_(global_type_ref|function_signature_type_ref|extern_decl_type_ref|call_type_ref)|positive_sema_ok_call_variadic_aggregate_runtime_c|abi_abi_variadic_(forward_wrapper_c|struct_result_c|va_copy_accumulate_c)|positive_split_llvm_pragma_exec|cpp_hir_template_(struct_body_instantiation|struct_arg_materialization|function_pack_signature_binding)|cpp_hir_record_(packed_aligned_layout|field_array_layout)|cpp_hir_builtin_layout_query_(sizeof_type|alignof_type|alignof_expr))$' --output-on-failure; } > test_after.log 2>&1`

Result:
- Build completed for `c4cll` and all four delegated frontend LIR test
  targets; Ninja reported no work to do.
- CTest passed 17/17:
  `frontend_lir_global_type_ref`,
  `frontend_lir_function_signature_type_ref`,
  `frontend_lir_extern_decl_type_ref`,
  `frontend_lir_call_type_ref`,
  `positive_sema_ok_call_variadic_aggregate_runtime_c`,
  `abi_abi_variadic_forward_wrapper_c`,
  `abi_abi_variadic_struct_result_c`,
  `abi_abi_variadic_va_copy_accumulate_c`,
  `positive_split_llvm_pragma_exec`,
  `cpp_hir_template_struct_body_instantiation`,
  `cpp_hir_template_struct_arg_materialization`,
  `cpp_hir_template_function_pack_signature_binding`,
  `cpp_hir_record_packed_aligned_layout`,
  `cpp_hir_record_field_array_layout`,
  `cpp_hir_builtin_layout_query_sizeof_type`,
  `cpp_hir_builtin_layout_query_alignof_type`, and
  `cpp_hir_builtin_layout_query_alignof_expr`.
