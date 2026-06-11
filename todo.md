Status: Active
Source Idea Path: ideas/open/182_phase_e_route4_publication_view_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define The Narrow Route 4 Boundary

# Current Packet

## Just Finished

Step 2 from `plan.md` is complete: added a local Route 4 block-entry
publication identity boundary for the selected
`current_block_entry_publication_register(...)` consumer.

Implemented boundary:
`src/backend/mir/aarch64/codegen/dispatch_publication.cpp` now wraps
`mir::find_bir_block_entry_publication_identity(...)` in a file-local
`route4_current_block_entry_publication_identity(...)` helper. The helper uses
the current BIR block as the successor block, the requested named BIR value as
the destination value, and the prepared destination home only as the oracle for
value id/name metadata and later register spelling.

Consumer behavior:
the selected reader consults the Route 4 boundary when BIR block context is
available and validates any available Route 4 identity against the requested
destination value name/type. Prepared publication data remains the fallback and
register/home oracle for absent or unavailable Route 4 data in this Step 2
slice; no prepared publication helper was deleted or narrowed, and no other
publication reader was migrated.

Coverage:
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` now gives the
selected reader a matching Route 4 PHI publication record in the indexed
block-entry publication test. Existing
`tests/backend/bir/backend_prealloc_block_entry_publications_test.cpp` and
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp` coverage continues
to prove absent and mismatched Route 4 block-entry publication validation at
the fail-closed facade/query boundary.

## Suggested Next

Execute Step 3 from `plan.md`: migrate
`current_block_entry_publication_register(...)` to prefer the validated Route 4
identity path when present, while preserving prepared fallback/oracle behavior
for invalid or unavailable Route 4 data.

## Watchouts

- Keep this Phase E slice to one selected value-publication or
  dispatch-publication reader:
  `current_block_entry_publication_register(...)`.
- Do not delete or narrow prepared publication helpers.
- Do not move target storage, move planning, publication construction, or
  block-order policy into BIR.
- Reject testcase-shaped matching, expectation downgrades, and generic facade
  expansion beyond Route 4 publication identity.
- The Route 4 boundary currently validates semantic destination identity only;
  prepared data still supplies destination home and register spelling.
- Do not hard-gate every call to `current_block_entry_publication_register(...)`
  on Route 4 presence in Step 3. Existing branch-fusion paths still require
  prepared fallback when their BIR block does not have a Route 4 block-entry
  PHI publication.

## Proof

Delegated proof passed and is preserved in `test_after.log`:

```bash
cmake --build build --target backend_aarch64_instruction_dispatch_test backend_prealloc_block_entry_publications_test backend_prepared_lookup_helper_test &&
ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_prealloc_block_entry_publications|backend_prepared_lookup_helper)$' --output-on-failure
```

Result: 3/3 tests passed.
