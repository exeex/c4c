# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Delete TypeSpec Tag And Validate

## Just Finished

Step 6's C variadic/ABI fallout after the LIR aggregate renderer changes is
repaired without reintroducing `TypeSpec::tag`.

The first bad boundary was the module-aware LIR alloca type renderer:
`llvm_alloca_ty(mod, ts)` preserved structured aggregate rendering but fell back
through `llvm_value_ty(mod, ts)` for `TB_VA_LIST`, which renders va_list values
as `ptr`. On non-Apple aarch64, local `va_list ap` therefore became `alloca ptr`
even though `llvm.va_start` writes the target va_list storage object
`%struct.__va_list_tag_`. That corrupted the va_list storage before
`va_arg`/variadic aggregate lowering ran, causing segfaults and the inline
diagnostics timeout.

The fix restores the existing target-aware va_list storage rule in the
module-aware alloca path: `TB_VA_LIST` storage now uses
`llvm_va_list_storage_ty(mod.target_profile)`. Aggregate value rendering remains
structured/module-aware, while C ABI storage for va_list locals and slots stays
target-correct.

## Suggested Next

Next packet should return to the remaining Step 6 positive/Sema failures now
that the delegated ABI subset and broader va_arg torture cluster are green.

## Watchouts

- Do not reintroduce `TypeSpec::tag` or rendered-string semantic lookup.
- Keep module-aware aggregate rendering and target ABI storage as separate
  concerns: aggregate values may need structured HIR owner/layout metadata, but
  `TB_VA_LIST` alloca storage must be target-profile driven.
- The delegated proof covers the dominant ABI/va_list family, but the broader
  full-suite va_arg cluster was also rerun before acceptance.

## Proof

Delegated proof command:
`cmake --build build && ctest --test-dir build -j --output-on-failure -R '^(positive_sema_inline_diagnostics_runtime_c|positive_sema_ok_call_variadic_aggregate_runtime_c|abi_)' > test_after.log 2>&1`

Result: monotonic improvement versus the delegated `test_before.log` baseline.

The proof improved the delegated subset from 0/5 passing to 5/5 passing. There
are 0 new failures relative to `test_before.log`.

Fixed baseline failures:
- `positive_sema_inline_diagnostics_runtime_c`
- `positive_sema_ok_call_variadic_aggregate_runtime_c`
- `abi_abi_variadic_forward_wrapper_c`
- `abi_abi_variadic_struct_result_c`
- `abi_abi_variadic_va_copy_accumulate_c`

Additional proof:
`ctest --test-dir build -j --output-on-failure -R '^llvm_gcc_c_torture_src_va_arg_' > /tmp/c4c_vaarg_after.log 2>&1`

Result: passed 25/25.

Canonical proof log: `test_after.log`.
