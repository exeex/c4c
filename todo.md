Status: Active
Source Idea Path: ideas/open/104_hir_safe_legacy_lookup_demotion.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Preserve Or Retire Parity Proof Deliberately

# Current Packet

## Just Finished

Completed and exhausted `plan.md` Step 4 after the owner-aware
`Lowerer::lower_struct_def` record-existence checks in
`src/frontend/hir/hir_types.cpp`.

Audit classification quote from
`review/103_hir_post_dual_path_legacy_readiness_audit.md`:
`Module::struct_def_owner_index` and `find_struct_def_by_owner_structured` are
`safe-to-demote` targets; owner-key lookup is structured and records
owner/rendered parity mismatches, suitable as replacement where callers have
`HirRecordOwnerKey`; `Module::struct_defs` rendered tag map remains
`bridge-required` for broad `TypeSpec::tag`/codegen consumers.

Changed files:
`src/frontend/hir/hir_types.cpp`, `todo.md`.

Lifecycle review:
No further safe HIR-only `Module::find_struct_def_by_owner_structured` consumer
is named for Step 4. Remaining owner-key hits are registration, parity/key
construction, or broad rendered `TypeSpec::tag` / `Module::struct_defs`
consumers that the review artifact classifies as bridge-required until a later
tag-to-owner or layout bridge exists.

## Suggested Next

Proceed to `plan.md` Step 5. The next packet should classify retained fallback
and parity instrumentation from the completed demotion work, separating removed
authority, retained proof-only observation, and bridge-required rendered lookup.

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

Full baseline candidate accepted by supervisor: 2976/2976.

`git diff --check` also passed.

Proof log: `test_after.log`.
