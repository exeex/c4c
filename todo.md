Status: Active
Source Idea Path: ideas/open/109_hir_to_lir_struct_layout_lookup_by_struct_name_id.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Prefer Structured Lookup For Field Chains And GEP Lowering

# Current Packet

## Just Finished

Completed `plan.md` Step 4 indexed-GEP element type migration. Indexed GEP
element selection now returns a `LirTypeRef`, preserves the exact legacy
`llvm_ty`/`llvm_alloca_ty` rendered text, and attaches `StructNameId` identity
for known struct/union element types found through `lookup_structured_layout`;
pointer compound assignment now uses the same helper path.

## Suggested Next

Supervisor can review remaining tag-rendered aggregate callers, if any, or move
Step 4 toward acceptance review.

## Watchouts

Structured identity is attached only when `lookup_structured_layout` finds the
matching `StructNameId -> LirStructDecl`; otherwise indexed GEPs keep the plain
rendered text fallback. This packet intentionally did not change emitted LLVM
text or test expectations.

## Proof

Delegated proof passed and wrote `test_after.log`:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|positive_sema_ok_expr_access_misc_runtime_c|llvm_gcc_c_torture_src_(pta_field_[12]_c|struct_(aliasing|cpy|ini|ret)_[0-9]_c|strct_.*_c))'; } > test_after.log 2>&1`.

The selected subset passed: 21/21 tests.
