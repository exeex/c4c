Status: Active
Source Idea Path: ideas/open/182_phase_e_route4_publication_view_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select Consumer And Baseline Behavior

# Current Packet

## Just Finished

Step 1 from `plan.md` is complete: selected exactly one AArch64
dispatch-publication reader for the Phase E Route 4 migration.

Selected reader:
`src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
`current_block_entry_publication_register(...)`.

Baseline prepared-helper behavior:
the reader currently calls
`prepare::find_prepared_current_block_entry_publication(...)` with prepared
names, regalloc, value locations, value-home lookups, and the current
control-flow block label as the successor label. It returns no publication
unless the prepared answer is
`PreparedCurrentBlockEntryPublicationStatus::Available`, the embedded
`PreparedBlockEntryPublication` is available, the destination home exists, and
the publication has a GP destination register name. On success it parses the
prepared register, coerces it to the requested `abi::RegisterView`, and returns
a `RegisterOperand` with `RegisterOperandRole::StoragePlan` plus the prepared
destination value id/name.

Route 4 source surface:
the matching semantic source should be the block-entry Route 4 path, not a
target-owned storage or move-plan aggregate:
`mir::find_bir_block_entry_publication_identity(...)` in
`src/backend/mir/query.cpp`, backed by
`bir::route4_build_publication_availability_index(...)`,
`bir::route_index_reference_facade(...)`, and
`bir::route_index_validate_block_entry_publication_reference(...)`. The request
should use the selected reader's current control-flow block as the successor
block and the requested named BIR value as the destination value. Prepared
publication helpers stay as fallback/oracle answers for register/home details
and for absent, unavailable, or mismatched Route 4 validation.

Expected Step 2/3 implementation-owned files:
`src/backend/mir/aarch64/codegen/dispatch_publication.cpp`,
`src/backend/mir/aarch64/codegen/dispatch_publication.hpp` only if a local
typed helper declaration is needed, and focused tests in
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` plus existing
Route 4 oracle coverage in
`tests/backend/bir/backend_prealloc_block_entry_publications_test.cpp` and
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.

## Suggested Next

Execute Step 2 from `plan.md`: add the narrow fail-closed Route 4 block-entry
publication boundary for
`current_block_entry_publication_register(...)`, preserving the prepared helper
as fallback/oracle and avoiding migration of any other publication reader.

## Watchouts

- Keep this Phase E slice to one selected value-publication or
  dispatch-publication reader:
  `current_block_entry_publication_register(...)`.
- Do not delete or narrow prepared publication helpers.
- Do not move target storage, move planning, publication construction, or
  block-order policy into BIR.
- Reject testcase-shaped matching, expectation downgrades, and generic facade
  expansion beyond Route 4 publication identity.
- The Route 4 identity can prove publication availability and destination
  value identity, but the selected reader still needs prepared fallback/oracle
  data for destination home/register spelling until Step 3 proves an equivalent
  route-first path.

## Proof

Analysis-only selection packet; delegated proof required no build/test and no
`test_after.log` was produced.

Local verification:
`rg`/AST-backed symbol inspection confirmed the selected reader, its prepared
helper calls, Route 4 block-entry facade/query surfaces, and focused test
coverage locations.

Proposed narrow implementation proof:

```bash
cmake --build build --target backend_aarch64_instruction_dispatch_test backend_prealloc_block_entry_publications_test backend_prepared_lookup_helper_test &&
ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_prealloc_block_entry_publications|backend_prepared_lookup_helper)$' --output-on-failure
```
