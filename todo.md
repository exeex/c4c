Status: Active
Source Idea Path: ideas/open/42_riscv_prepared_edge_publication_aggregate_stack_source_policy.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement Or Preserve Fail-Closed Aggregate Form

# Current Packet

## Just Finished

Step 2 preserved and made explicit the fail-closed policy for RISC-V
aggregate-width `StackSlot -> Register` prepared edge-publication sources.

Changes made:

- Tightened `backend_riscv_prepared_edge_publication` negative coverage for
  aggregate-width stack sources at a signed-12 stack offset and at a large
  offset.
- The tests now prove aggregate-width forms remain `UnsupportedSourceHome`,
  produce no scalar `lw`/`ld` stack load, produce no scalar large-offset `t6`
  load sequence, and record no scalar stack-load provenance.
- The existing scalar 4-byte, 8-byte, and large-offset paths remain covered by
  the same focused test binary.
- `src/backend/mir/riscv/codegen/emit.cpp` was not changed; current lowering
  already rejects aggregate-width stack sources before scalar load opcode
  selection.

## Suggested Next

Run Step 3 only if the supervisor wants another narrow hardening packet before
validation: add adjacent negative coverage for aggregate destination/lane,
alignment, partial-copy, or scratch-policy expectations that can be represented
without inventing new prepared authority. Otherwise Step 4 validation can prove
the documented fail-closed aggregate policy route.

## Watchouts

The remaining blocker is still prepared-authority shape, not an emitter
mechanical gap: shared edge-publication data does not yet describe aggregate
copy width, lanes, partial-copy/alignment/ABI layout, destination lane mapping,
or scratch ownership. Do not treat aggregate stack sources as scalar loads from
size, fixture names, value ids, stack slot ids, offsets, or register spelling.
Do not broaden into typed, dynamic, pointer, or source-to-stack
edge-publication routes.

## Proof

Passed:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_prepared_edge_publication|backend_prepared_lookup_helper)$' | tee test_after.log`

Result: 2/2 tests passed. Proof log: `test_after.log`.
