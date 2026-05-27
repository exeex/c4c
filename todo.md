# Current Packet

Status: Active
Source Idea Path: ideas/open/57_aarch64_variadic_va_arg_register_save_progression_repair.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Confirm aggregate register-save progression ordering

## Just Finished

Completed Step 1 classification for aggregate `va_arg` register-save
progression ordering in `src/backend/mir/aarch64/codegen/variadic.cpp`.

The mismatched owner is `print_aggregate_va_arg_lowering_lines()`, specifically
its `RegisterSaveArea` branch at the call into
`append_aggregate_copy_from_va_list_field()`. That helper handles both
`PreparedVariadicVaListFieldKind::GpRegisterSaveArea` and
`PreparedVariadicVaListFieldKind::FpRegisterSaveArea` through the shared
`register_save_area` branch, and it forms register-save source addresses by
reloading `va_arg.progression_field_offset_bytes` from the live va_list field.

The ordering problem is that `print_aggregate_va_arg_lowering_lines()` computes
the advanced progression value before the register-save copy, then
`append_aggregate_copy_from_va_list_field()` reloads the live progression field
when forming the current aggregate source address. The GPR and FPR aggregate
register-save paths share this helper, so Step 2 should repair the shared
register-save addressing contract rather than adding a GPR-only or testcase
shortcut.

## Suggested Next

Delegate Step 2 implementation in
`src/backend/mir/aarch64/codegen/variadic.cpp`: change the aggregate
register-save branch of `print_aggregate_va_arg_lowering_lines()` and/or
`append_aggregate_copy_from_va_list_field()` so the current register-save source
address uses the pre-increment `gp_offset`/`fp_offset` value already loaded for
the overflow decision, and only stores the advanced progression field after the
current aggregate copy has selected its source. Keep the change semantic across
both `GpRegisterSaveArea` and `FpRegisterSaveArea`; do not match testcase names,
temporary spelling, stack offsets, string literals, or `%` format contents.

## Watchouts

The semantic contract is the AAPCS64 va_list register-save rule: the current
aggregate source address uses the old negative offset, while the va_list
progression field receives old offset plus the prepared stride.

`append_aggregate_copy_from_va_list_field()` is also used for the overflow-area
fallback, where `register_save_area` is false and the source field is a pointer;
Step 2 should preserve that behavior.

Treat existing dirty `memory.cpp`, `dispatch_edge_copies.cpp`, and transient
`review/*` files as external context.

## Proof

Ran the delegated proof:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'
```

Result: build passed, focused subset `7/8` passed, and only
`c_testsuite_aarch64_backend_src_00204_c` failed with `[RUNTIME_MISMATCH]`.
Proof log path: `test_after.log`.
