Status: Active
Source Idea Path: ideas/open/202_route3_memory_source_identity_adapter.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Success, Rejection, And Fallback

# Current Packet

## Just Finished

Step 4 repaired the Route 3 load-local adapter regression exposed by
`c_testsuite_aarch64_backend_src_00164_c`. The selected reader now
distinguishes absent Route 3 identity from a contradictory Route 3/prepared
mismatch: absent Route 3 evidence can use the already validated prepared
load-local source fallback, while actual Route 3/prepared mismatches still
reject the source-home operand.

The focused AArch64 scalar-consumer fixture now proves that missing Route 3
identity preserves the prepared source fallback instead of falling through to an
uninitialized loaded-value home. The existing prepared instruction-index
mismatch case still rejects the source-home operand.

Files changed:

- `src/backend/mir/aarch64/codegen/alu.cpp`
- `tests/backend/mir/backend_aarch64_prepared_scalar_alu_records_test.cpp`
- `todo.md`

## Suggested Next

Select the next Step 5 packet from `plan.md`: run the broader validation and
handoff route if the supervisor accepts this regression repair and the current
selected-reader evidence, or request review if another fail-closed case is
needed before broad proof.

## Watchouts

Do not move address formation, frame/global/TLS relocation, stack/frame
offsets, concrete layout, addressing-mode legality, materialization, final
operands, or target-addressing fallback into BIR schema.

Do not replace all `memory_accesses`, `PreparedMemoryAccessLookups`, or
memory/frame/stack helper groups.

The missing-Route-3-identity coverage mutates only the BIR address base kind
away from `LocalSlot`; it does not add testcase-shaped production matching.
The production repair is semantic fallback-boundary handling, not a special case
for `00164.c`.

## Proof

Supervisor-selected proof command run exactly:

`cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_aarch64_prepared_scalar_alu_records|backend_store_source_publication_plan|backend_prepared_lookup_helper|c_testsuite_aarch64_backend_src_00164_c)$' > test_after.log`

Result: passed after the final edit. `test_after.log` contains 4/4 passing tests:
`backend_store_source_publication_plan`,
`backend_aarch64_prepared_scalar_alu_records`,
`backend_prepared_lookup_helper`, and
`c_testsuite_aarch64_backend_src_00164_c`.

Additional validation: `git diff --check` passed.
