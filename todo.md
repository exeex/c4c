Status: Active
Source Idea Path: ideas/open/202_route3_memory_source_identity_adapter.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Success, Rejection, And Fallback

# Current Packet

## Just Finished

Step 3/4 strengthened selected-reader proof for
`make_unpublished_load_local_source_operand(...)` without touching production
code. The focused AArch64 scalar-consumer fixture now reuses one local lowering
helper, preserves the success case where prepared addressing supplies the
source-home frame-slot operands, keeps the prepared-instruction-index mismatch
fallback, and adds a missing Route 3 BIR local-slot identity case that rejects
the source-home operand and falls back instead.

This proves an additional fail-closed/fallback case beyond the prior prepared
instruction-index mismatch while preserving target-addressing authority in
prepared lookup and `make_prepared_scalar_load_source(...)`.

Files changed:

- `tests/backend/mir/backend_aarch64_prepared_scalar_alu_records_test.cpp`
- `todo.md`

## Suggested Next

Select the next Step 5 packet from `plan.md`: run the broader validation and
handoff route if the supervisor accepts the current selected-reader evidence,
or request review if another fail-closed case is needed before broad proof.

## Watchouts

Do not move address formation, frame/global/TLS relocation, stack/frame
offsets, concrete layout, addressing-mode legality, materialization, final
operands, or target-addressing fallback into BIR schema.

Do not replace all `memory_accesses`, `PreparedMemoryAccessLookups`, or
memory/frame/stack helper groups.

The missing-Route-3-identity coverage mutates only the BIR address base kind
away from `LocalSlot`; it does not add testcase-shaped production matching.
`src/backend/mir/aarch64/codegen/alu.cpp` was not changed in this packet.

## Proof

Supervisor-selected proof command run exactly:

`cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_aarch64_prepared_scalar_alu_records|backend_store_source_publication_plan|backend_prepared_lookup_helper)$' > test_after.log`

Result: passed after the final edit. `test_after.log` contains 3/3 passing tests:
`backend_store_source_publication_plan`,
`backend_aarch64_prepared_scalar_alu_records`, and
`backend_prepared_lookup_helper`.

Additional validation: `git diff --check` passed.
