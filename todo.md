Status: Active
Source Idea Path: ideas/open/109_hir_to_lir_struct_layout_lookup_by_struct_name_id.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Prefer Structured Lookup For Field Chains And GEP Lowering

# Current Packet

## Just Finished

Completed `plan.md` Step 4 field-chain/member-GEP caller migration. Field-chain
resolution now calls the shared structured layout helper for each legacy
struct/union tag, records a `StructNameId` on `FieldStep` when the helper finds
the corresponding `LirStructDecl`, and member-GEP emission prefers that
structured identity while preserving the legacy tag-string fallback and
legacy-authoritative field lookup.

## Suggested Next

Supervisor can review whether the next Step 4 packet should migrate indexed
GEP element type selection or another remaining tag-rendered aggregate caller
to prefer structured lookup.

## Watchouts

`LirStructDecl` still does not carry field names or offsets, so field-chain
member selection and LLVM slot computation remain legacy-authoritative through
`TypeSpec::tag` and `mod.struct_defs`. Structured identity is used only when
`lookup_structured_layout` finds the matching `StructNameId -> LirStructDecl`;
otherwise member-GEP emission still interns or renders the legacy tag string.

## Proof

Delegated proof passed and wrote `test_after.log`:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|positive_sema_ok_expr_access_misc_runtime_c|llvm_gcc_c_torture_src_(pta_field_[12]_c|struct_(aliasing|cpy|ini|ret)_[0-9]_c|strct_.*_c))'; } > test_after.log 2>&1`.

The selected subset passed: 21/21 tests.
