Status: Active
Source Idea Path: ideas/open/170_route4_block_entry_publication_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Or Confirm Route 4 Oracle Coverage

# Current Packet

## Just Finished

Step 2 oracle coverage completed for selected consumer
`mir::find_bir_block_entry_publication_identity(...)`.

Confirmed existing coverage:
- `tests/backend/bir/backend_prealloc_block_entry_publications_test.cpp`
  already covers available BIR block-entry PHI identity, prepared/BIR semantic
  field matching, BIR-only PHI availability, wrong successor, wrong destination
  value, and prepared readiness remaining separate from BIR identity.
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
  already covers `route4_find_block_entry_publication(...)`,
  `route4_validate_block_entry_publication_reference(...)`, and
  `route_index_validate_block_entry_publication_reference(...)` for available
  Route 4 answers, missing publication, wrong key/type mismatch, stale owner,
  duplicate reference, and facade/direct validation agreement.
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
  already exercises the selected MIR helper in a call-contract scenario while
  keeping prepared ABI/publication mechanics separate.

Extended coverage:
- Added selected-consumer MIR assertions in
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` for missing
  destination keys and destination type mismatch. These pin fail-closed
  `MissingDestinationValue` behavior before Step 3 migrates the implementation
  to Route 4 availability records.

## Suggested Next

Execute Step 3: migrate
`mir::find_bir_block_entry_publication_identity(...)` in
`src/backend/mir/query.cpp` to use `bir::Route4PublicationAvailabilityIndex`
or the existing typed Route 4 block-entry publication query as the primary
availability and destination-value identity source. Keep `src/backend/mir/query.hpp`
unchanged unless the existing status/request/result shape cannot preserve the
pinned Step 2 behavior.

## Watchouts

- Preserve the Step 2 fail-closed categories: missing successor remains
  `MissingSuccessorLabel`; missing destination key or type mismatch remains
  `MissingDestinationValue`; absent PHI/publication remains
  `MissingPublication`.
- Do not move publication hook kind, destination home, storage encoding,
  stack-source extension, register-view conversion, immediate spelling,
  emitted storage, scalar publication emission policy, or publication ordering
  into BIR.
- Do not add a broad BIR scan or route-index facade unless the direct typed
  Route 4 query cannot preserve the selected consumer contract.
- Do not hide prepared block-entry helpers in Step 3; they remain oracle and
  fallback surface until a separate contraction step re-scans consumers.

## Proof

Ran exact delegated proof; passed. Proof log: `test_after.log`.

```bash
(cmake --build build --target backend_prealloc_block_entry_publications_test backend_prepared_lookup_helper_test backend_prepare_frame_stack_call_contract_test && ctest --test-dir build -R '^(backend_prealloc_block_entry_publications|backend_prepared_lookup_helper|backend_prepare_frame_stack_call_contract)$' --output-on-failure) > test_after.log 2>&1
```
