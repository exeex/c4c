Status: Active
Source Idea Path: ideas/open/182_phase_e_route4_publication_view_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Route/Prepared Equivalence

# Current Packet

## Just Finished

Step 4 from `plan.md` is complete: proved Route/prepared equivalence and
fail-closed coverage for the selected
`current_block_entry_publication_register(...)` boundary.

Coverage added:
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` now asserts
that the Route 4 block-entry identity matches the prepared destination identity
on the route-present path, and that the selected consumer preserves prepared
fallback for both invalid and mismatched Route 4 data. The existing unavailable
Route 4 fallback and missing prepared-index fail-closed cases remain in place.

Facade proof strengthened:
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp` now checks that the
Route 4 facade preserves both validation status and route status against the
direct Route 4 validator for missing, type mismatch, missing value, stale owner,
wrong relationship, duplicate reference, and diverged-reference cases.
`tests/backend/bir/backend_prealloc_block_entry_publications_test.cpp` was left
unchanged; its existing prepared oracle/publication coverage was not weakened.

## Suggested Next

Execute Step 5 from `plan.md`: document the final selected-reader result and
prepare the supervisor/plan-owner handoff for lifecycle closure or the next
Phase E follow-up idea.

## Watchouts

- Keep this Phase E slice to one selected value-publication or
  dispatch-publication reader:
  `current_block_entry_publication_register(...)`.
- Do not delete or narrow prepared publication helpers.
- Do not move target storage, move planning, publication construction, or
  block-order policy into BIR.
- Reject testcase-shaped matching, expectation downgrades, and generic facade
  expansion beyond Route 4 publication identity.
- The selected reader still depends on prepared data for register spelling and
  destination home; Route 4 supplies preferred semantic identity only when
  available.
- Step 4 did not require production-code changes.
- Existing branch-fusion paths still require prepared fallback when their BIR
  block does not have a valid Route 4 block-entry PHI publication.

## Proof

Delegated proof passed and is preserved in `test_after.log`:

```bash
cmake --build build --target backend_aarch64_instruction_dispatch_test backend_prealloc_block_entry_publications_test backend_prepared_lookup_helper_test &&
ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_prealloc_block_entry_publications|backend_prepared_lookup_helper)$' --output-on-failure
```

Result: 3/3 tests passed.
