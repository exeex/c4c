Status: Active
Source Idea Path: ideas/open/182_phase_e_route4_publication_view_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate The Selected Consumer

# Current Packet

## Just Finished

Step 3 from `plan.md` is complete: migrated only
`current_block_entry_publication_register(...)` to prefer the validated Route 4
block-entry publication identity when it is present.

Implemented migration:
`src/backend/mir/aarch64/codegen/dispatch_publication.cpp` now returns the
selected operand identity from `route4_current_block_entry_publication_identity(...)`
when the Route 4 identity is available, while still using prepared publication
data as the register spelling/home oracle. When Route 4 identity is unavailable
or invalid for this consumer, prepared publication data remains the fallback;
no prepared publication helper was deleted or narrowed, and no other publication
reader was migrated.

Coverage:
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` now verifies
the selected reader's Route 4-present path, Route 4-unavailable prepared
fallback, and invalid Route 4 data prepared fallback. Existing
`tests/backend/bir/backend_prealloc_block_entry_publications_test.cpp` and
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp` coverage continues
to prove the BIR/prepared facade behavior below this consumer.

## Suggested Next

Execute Step 4 from `plan.md`: expand the selected reader's validation surface
as directed by the runbook, without migrating another publication reader unless
the supervisor explicitly delegates that next packet.

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
- Existing branch-fusion paths still require prepared fallback when their BIR
  block does not have a valid Route 4 block-entry PHI publication.

## Proof

Delegated proof passed and is preserved in `test_after.log`:

```bash
cmake --build build --target backend_aarch64_instruction_dispatch_test backend_prealloc_block_entry_publications_test backend_prepared_lookup_helper_test &&
ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_prealloc_block_entry_publications|backend_prepared_lookup_helper)$' --output-on-failure
```

Result: 3/3 tests passed.
