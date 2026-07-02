Status: Active
Source Idea Path: ideas/open/514_rv64_register_source_stack_destination_move_bundles.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Focused Backend Coverage

# Current Packet

## Just Finished

Step 4 - Add Focused Backend Coverage completed for idea 514.

Focused RV64 backend coverage now proves the same-width
`consumer_register_to_stack` stack-destination path is accepted by both
prepared object-module emission and the prepared ELF writer.

The existing classifier-owned conversion reject coverage is tightened to prove
the prepared ELF writer preserves the same precise
`unsupported_prepared_move_bundle_classification` diagnostic as object-module
emission for the single `consumer_stack_to_stack` register-source
stack-destination width mismatch.

The idea 516 multi-source closed boundary remains covered by the duplicate
register-source stack-destination move case, which still rejects through the
shared
`AmbiguousNonParallelMultiSourceStackDestination` prepared-consumer category
instead of being weakened into RV64 materialization.

## Suggested Next

Execute Step 5 validation and handoff for idea 514, using the supervisor's
chosen acceptance proof.

## Watchouts

Remaining intentionally fail-closed shapes include same-width
reason/source-home mismatches, missing source-size authority, malformed
destination offset facts, and conversion moves that do not have the precise
single register-source stack-destination classifier-owned mismatch shape.

No implementation was touched in Step 4; the slice is focused backend coverage
only.

## Proof

Proof is recorded in `test_after.log`.

Commands run:

- `cmake --build --preset default`
- `ctest --test-dir build -j --output-on-failure -R '^backend_'`
- `git diff --check -- tests/backend/mir/backend_riscv_object_emission_test.cpp todo.md`

The delegated build and `^backend_` subset passed.
