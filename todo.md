Status: Active
Source Idea Path: ideas/open/200_hir_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Fence Metadata-Rich Module And Owner Lookups

# Current Packet

## Just Finished

Completed `plan.md` Step 2 owner-fallback slice for HIR record/aggregate
lookup. `resolve_struct_method_lookup_owner_tag` now resolves complete
record/tag owner keys before delegating to rendered compatibility helpers, so
complete owner-key misses fail closed before `tag_text_id`, legacy tag, or
rendered `struct_defs` recovery.

Retained direct-constructor `callee_name` and aggregate `typespec_legacy_tag`
bridges are documented as no-owner compatibility only. Focused lookup coverage
now proves a complete method owner-key miss rejects rendered/tag compatibility
even when a stale rendered `struct_defs` entry exists.

## Suggested Next

Continue `plan.md` Step 2 by reviewing the remaining non-owned
`resolve_member_lookup_owner_tag` rendered recovery in
`src/frontend/hir/impl/templates/value_args.cpp`; this packet fenced the owned
method wrapper without editing that broader helper.

## Watchouts

- The method-wrapper fence intentionally resolves or rejects complete owner
  keys before calling `resolve_member_lookup_owner_tag`, because that helper is
  outside this packet's owned files and still has a rendered tag recovery path.
- No-owner direct constructor and aggregate compatibility remain intact; this
  slice only rejects complete owner-key misses.

## Proof

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_lookup_tests|frontend_hir_tests|cpp_hir_expr_object_materialization_helper)$"' > test_after.log 2>&1`

Result: passed. `test_after.log` is the proof log.
