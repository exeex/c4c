Status: Active
Source Idea Path: ideas/open/209_route4_publication_source_semantic_reader.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prove Negative and Fallback Cases

# Current Packet

## Just Finished

Step 3 completed focused negative/fallback proof for only
`find_prepared_indirect_callee_source_producer(...)`.

Extended
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` so
`block_dispatch_indirect_callee_route4_agreement_preserves_prepared_policy()`
now rebuilds the fixture per case, keeps the stale prepared select payload
decoy as the positive discriminator, and proves selected-callee output is still
preserved for missing Route 4 relationship data, route/prepared value-type
mismatch, and stale/wrong-reference prepared source-producer indices.

No implementation files changed. The selected reader's duplicate/conflict Route
4 validation is not directly injectable through this dispatch fixture without
creating a broader malformed retained-BIR block that fails outside the selected
reader; the paired `backend_prepared_lookup_helper` proof still covers
current-block Route 4 duplicate-reference validation in the delegated subset.

## Suggested Next

Step 4 should prove output and wrapper stability for the selected-reader slice
without changing expected strings or broadening into adjacent call-boundary
readers, wrappers, printer/debug output, public fallback removal, or prepared
API migration.

## Watchouts

Do not migrate or rename `prepared_call_boundary_source_value(...)`; it remains
an adjacent Route 4 reader, not part of this selected-reader slice. Duplicate
current-block Route 4 rows are validated by the Route 4 index/reference helper;
forcing a duplicate into this full dispatch fixture changes ordinary retained
BIR lowering before the indirect-callee reader can be isolated. Do not move
indirect-callee materialization, scratch selection, ABI register policy, call
records, wrappers, printer/debug rows, expected strings, or prepared API
surfaces into Route 4.

## Proof

Ran the delegated proof exactly:
`cmake --build --preset default --target backend_aarch64_instruction_dispatch_test backend_prepared_lookup_helper_test && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_prepared_lookup_helper)$' > test_after.log`

Result: passed, 2/2 focused tests green. Canonical proof log:
`test_after.log`.
