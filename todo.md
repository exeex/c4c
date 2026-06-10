Status: Active
Source Idea Path: ideas/open/170_route4_block_entry_publication_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate The Selected Consumer

# Current Packet

## Just Finished

Step 3 selected-consumer migration completed for
`mir::find_bir_block_entry_publication_identity(...)`.

Changed `src/backend/mir/query.cpp` so block-entry publication identity now
builds a typed Route 4 publication availability index for the successor block
and uses `bir::route4_find_block_entry_publication(...)` as the primary lookup
when the destination type is known. For destination-name requests with no
explicit type, it uses the Route 4 block-entry availability records directly as
the primary source instead of the older semantic PHI scan.

Preserved the Step 2 fail-closed contract:
- missing or mismatched successor label returns `MissingSuccessorLabel`
- missing destination key returns `MissingDestinationValue`
- Route 4 destination type mismatch (`NoMatch`) returns `MissingDestinationValue`
- absent Route 4 block-entry publication returns `MissingPublication`
- available records are mapped back onto the original successor block for MIR
  instruction/PHI/value pointers

## Suggested Next

Supervisor should review and commit the Step 3 code-plus-`todo.md` slice, then
delegate the next plan step or plan-owner lifecycle decision according to the
active runbook.

## Watchouts

- `src/backend/mir/query.hpp` did not need changes; the existing request/result
  shape preserved behavior.
- Publication emission/storage mechanics, prepared helper public surface,
  AArch64 codegen, tests, `plan.md`, and source ideas were left untouched.
- The no-explicit-type path intentionally reads Route 4 availability records
  directly because `route4_find_block_entry_publication(...)` requires a typed
  destination `Value`.

## Proof

Ran exact delegated proof; passed 3/3 tests. Proof log: `test_after.log`.

```bash
(cmake --build build --target backend_prealloc_block_entry_publications_test backend_prepared_lookup_helper_test backend_prepare_frame_stack_call_contract_test && ctest --test-dir build -R '^(backend_prealloc_block_entry_publications|backend_prepared_lookup_helper|backend_prepare_frame_stack_call_contract)$' --output-on-failure) > test_after.log 2>&1
```
