Status: Active
Source Idea Path: ideas/open/104_hir_safe_legacy_lookup_demotion.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Use Owner-Aware Record Lookup Where Owner Keys Exist

# Current Packet

## Just Finished

Completed `plan.md` Step 4 for the owner-aware `Lowerer::lower_struct_def`
record-existence checks in `src/frontend/hir/hir_types.cpp`.

Audit classification quote from
`review/103_hir_post_dual_path_legacy_readiness_audit.md`:
`Module::struct_def_owner_index` and `find_struct_def_by_owner_structured` are
`safe-to-demote` targets; owner-key lookup is structured and records
owner/rendered parity mismatches, suitable as replacement where callers have
`HirRecordOwnerKey`; `Module::struct_defs` rendered tag map remains
`bridge-required` for broad `TypeSpec::tag`/codegen consumers.

Changed files:
`src/frontend/hir/hir_types.cpp`, `todo.md`.

## Suggested Next

Supervisor should review Step 4 completion and choose the next owner-key
demotion packet or plan review.

## Watchouts

- The `lower_struct_def` changes only affect record-existence decisions:
  forward-declaration skip and `struct_def_order` append eligibility.
- Rendered `module_->struct_defs` fallback remains in place when owner metadata
  is incomplete or structured owner lookup misses.
- `TypeSpec::tag`, `HirStructDef::tag`, and codegen bridge behavior were not
  changed.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -L hir > test_after.log`
passed 73/73.

`git diff --check` also passed.

Proof log: `test_after.log`.
