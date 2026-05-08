Status: Active
Source Idea Path: ideas/open/152_hir_lir_rendered_owner_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Retire Member Typedef And Signature Recovery Fallbacks regression repair

# Current Packet

## Just Finished

Completed Plan Step 5 regression repair by updating the member typedef readiness metadata fixture to seed the structured `struct_def_nodes_by_owner_` authority for the valid `tag_text_id` owner instead of relying on rendered/tag-map owner recovery.

The readiness regression now passes without restoring fallback recovery after complete owner-key misses. Existing focused `frontend_hir_lookup_tests` coverage still rejects stale `struct_def_nodes_` and module `struct_defs` recovery after complete owner-key misses.

## Suggested Next

Next coherent packet: resume Step 6 aggregate-helper compatibility retirement, or have the supervisor route any remaining Step 5 review follow-up if the repaired fixture exposes additional owner-readiness drift.

## Watchouts

- The code path in `resolve_struct_member_typedef_if_ready` still treats complete structured owner-key misses as authoritative misses; do not reintroduce rendered tag or module tag scans for that case.
- The passing readiness fixture now models the hit case through `struct_def_nodes_by_owner_`; stale rendered `struct_def_nodes_` entries remain present only as collision pressure.

## Proof

Supervisor-selected proof passed and wrote `test_after.log`:

`cmake --build --preset default --target cpp_hir_member_typedef_readiness_metadata_test frontend_hir_lookup_tests && ctest --test-dir build -R '^(cpp_hir_member_typedef_readiness_structured_metadata|frontend_hir_lookup_tests)$' --output-on-failure > test_after.log`
