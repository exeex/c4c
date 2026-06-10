Status: Active
Source Idea Path: ideas/open/170_route4_block_entry_publication_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Residual Route 4 Consumers

# Current Packet

## Just Finished

Step 1 inventory completed for Route 4 block-entry publication migration.

Selected residual consumer:
- `mir::find_bir_block_entry_publication_identity(...)` in
  `src/backend/mir/query.cpp`, declared in `src/backend/mir/query.hpp`.
- Current behavior still performs an older semantic PHI scan over
  `request.successor_block->insts` and fills `BirBlockEntryPublicationIdentity`
  directly.
- Target migration should build/read `bir::Route4PublicationAvailabilityIndex`
  and use `bir::route4_find_block_entry_publication(...)` as the primary
  availability and destination-value identity source.
- A broad route-index facade is not needed for this consumer; keep
  `bir::route_index_validate_block_entry_publication_reference(...)` as
  validation/oracle coverage unless implementation discovers a need for a
  narrow helper.

Target files for the future implementation packet:
- `src/backend/mir/query.cpp`
- `src/backend/mir/query.hpp` only if status/request/result mapping needs a
  signature or enum adjustment.

Rejected/out-of-scope consumers:
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp` helpers
  `current_block_entry_publication_register(...)`,
  `value_has_current_block_entry_publication(...)`, and
  `record_current_block_entry_publication_registers(...)`: target register,
  home, and emitted-storage decisions; keep on prepared publication data.
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
  `block_entry_move_clobbers_current_join_publication(...)`: register alias and
  clobber policy; out of Route 4 semantic availability scope.
- `src/backend/prealloc/publication_plans.cpp`
  `plan_prepared_scalar_publication(...)` and related
  `current_block_entry_publication_available` plumbing: publication emission
  policy and destination home/storage encoding; out of scope.
- `src/backend/mir/x86/prepared/prepared.hpp`
  `collect_block_entry_publications(...)`: prepared facade/oracle exposure,
  not the MIR semantic residual.
- `src/backend/prealloc/prepared_printer/value_locations.cpp`: diagnostic
  printing of prepared publications, not a migration consumer.

Coverage candidates:
- Positive/oracle coverage already nearby:
  `tests/backend/bir/backend_prealloc_block_entry_publications_test.cpp`
  checks available block-entry PHI identity, prepared/BIR semantic matching,
  BIR-only PHI availability, and prepared readiness remaining separate from
  BIR identity.
- Route 4 index/reference coverage already nearby:
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
  `verify_bir_block_entry_publication_identity_lookup()` checks
  `route4_find_block_entry_publication(...)`,
  `route4_validate_block_entry_publication_reference(...)`, and
  `route_index_validate_block_entry_publication_reference(...)`.
- Additional consumer-contract coverage nearby:
  `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
  exercises `mir::find_bir_block_entry_publication_identity(...)` in a
  call-contract scenario while keeping prepared ABI/publication mechanics
  separate.

Missing/negative coverage candidates to confirm before or during migration:
- The selected MIR helper should have behavior pinned for missing successor,
  missing destination name/value, missing publication, and destination type
  mismatch after switching to Route 4 status mapping.
- Keep existing route-index negatives for wrong key/type mismatch, stale owner,
  duplicate reference, and missing publication in the proof subset.
- Do not add expectations that require prepared register/home availability for
  BIR block-entry PHI identity.

## Suggested Next

Execute Step 2: confirm or extend oracle coverage for the selected
`mir::find_bir_block_entry_publication_identity(...)` consumer, especially
block-entry missing-publication and type/wrong-key negative behavior, before
switching the implementation to Route 4.

## Watchouts

- Keep the first packet analysis-only unless the supervisor delegates
  implementation.
- Do not move publication hook kind, destination home, storage encoding,
  stack-source extension, register-view conversion, immediate spelling,
  emitted storage, scalar publication emission policy, or publication ordering
  into BIR.
- Do not hide prepared block-entry helpers while the residual consumer still
  needs them as oracle or fallback.
- Reject broad BIR scans, publication emission rewrites, or named-case-only
  shortcuts.

## Proof

No build or tests run; this packet was analysis-only and did not touch
`test_after.log`.

Read-only inventory used `rg`, `ctest --test-dir build -N`, and
`c4c-clang-tool-ccdb` direct callee/caller queries for
`find_bir_block_entry_publication_identity(...)` and
`find_prepared_current_block_entry_publication(...)`.

Proposed future proof command:

```bash
cmake --build build --target backend_prealloc_block_entry_publications_test backend_prepared_lookup_helper_test backend_prepare_frame_stack_call_contract_test && ctest --test-dir build -R '^(backend_prealloc_block_entry_publications|backend_prepared_lookup_helper|backend_prepare_frame_stack_call_contract)$' --output-on-failure
```
