Status: Active
Source Idea Path: ideas/open/109_hir_to_lir_struct_layout_lookup_by_struct_name_id.md
Source Plan Path: plan.md
Current Step ID: none
Current Step Title: none

# Current Packet

## Just Finished

Completed `plan.md` Step 6 aggregate call-handling lookup migration for the
variadic aggregate call and `va_arg` layout paths.
Variadic aggregate payload-size checks now route named struct/union `TypeSpec`s
through `lookup_structured_layout`, while continuing to use the legacy
`HirStructDef` size result for ABI decisions. Byval and amd64 `va_arg`
alignment queries now use the `LirModule`-aware `object_align_bytes` overload,
which attempts structured lookup when the `StructNameId` is known but still
uses the legacy layout for the emitted call text and ABI alignment.

## Suggested Next

Supervisor can review whether Step 6 has any remaining aggregate call-layout
families to migrate, or move the step toward acceptance review.

## Watchouts

The migrated call and `va_arg` paths intentionally consume
`layout.legacy_decl` or existing amd64 ABI classifier results after structured
lookup is attempted. This packet did not change emitted LLVM text or test
expectations.

## Proof

Delegated proof passed and wrote `test_after.log`:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|positive_sema_ok_call_variadic_aggregate_runtime_c|abi_abi_variadic_.*|llvm_gcc_c_torture_src_(strct_stdarg_1_c|strct_varg_1_c|struct_ret_[12]_c|va_arg_.*_c))'; } > test_after.log 2>&1`.

The selected subset passed: 37/37 tests.
