Status: Active
Source Idea Path: ideas/open/289_structured_opaque_pointer_byte_range_provenance.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Publish Provenance Verdicts to Prepared Consumers

# Current Packet

## Just Finished

Step 5: Publish Provenance Verdicts to Prepared Consumers published passive
range and dynamic-array verdict metadata through prepared reporting/copy
surfaces without changing prepared/prealloc acceptance behavior.

- Added prepared naming helpers for `MemoryRangeVerdict` and
  `MemoryDynamicArrayRangeVerdict`, and printed both verdicts on prepared
  `access` rows next to, but separate from, `base_plus_offset`.
- Copied source memory `range_verdict` and `dynamic_array.verdict` from
  `PreparedMemoryAccess::address.provenance` into
  `PreparedEdgePublication` and `PreparedEdgeCopySourceFacts`.
- Extended the prepared source-memory match helper to compare copied verdict
  metadata as part of copy-surface agreement, while leaving
  `source_memory_access_status` and existing completeness checks unchanged.
- Focused lookup/printer coverage asserts verdict copy and report surfaces and
  keeps existing stale-row, duplicate-row, and fail-closed behavior unchanged.

## Suggested Next

Continue Step 5 or move to Step 6 only under supervisor direction: decide which
consumer, if any, should start gating on explicit provenance verdicts, or begin
quarantining the opaque pointer compatibility bridge once route coverage is
complete.

## Watchouts

- `range_verdict` and `dynamic_array.verdict` are still passive metadata; no
  current consumer gates route admission, target lowering, or prepared/prealloc
  acceptance on them.
- `can_use_base_plus_offset` remains the syntactic address-usability field and
  was not reinterpreted as object-range proof.
- Source-memory availability still depends on existing prepared row identity,
  complete-base, size/alignment, stale-row, duplicate-row, and producer checks.
- Target-specific acceptance policy, layout-authority-specific gating, and
  compatibility bridge retirement remain deferred.

## Proof

Supervisor-selected proof passed and wrote `test_after.log`:

`cmake --build --preset default --target backend_lir_to_bir_notes_test backend_prepared_lookup_helper_test backend_riscv_prepared_edge_publication_test backend_prepared_printer_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_prepared_lookup_helper|backend_riscv_prepared_edge_publication|backend_prepared_printer|backend_aarch64_instruction_dispatch)$' > test_after.log`

CTest result: 5/5 passed.
