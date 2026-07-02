Status: Active
Source Idea Path: ideas/open/514_rv64_register_source_stack_destination_move_bundles.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Focused Backend Coverage

# Current Packet

## Just Finished

Step 4 added focused RV64 backend coverage around the prepared
register-source to stack-destination move-bundle boundary.

Coverage completed:

- The existing accepted single `consumer_register_to_stack` fixture still
  proves a GPR source is stored into the explicit stack-slot destination.
- The register-to-stack fail-closed coverage now rejects multiple
  `consumer_register_to_stack` moves in one bundle and asserts the semantic
  diagnostic reports `move_count=2` plus both register-to-stack reasons.
- The same fail-closed coverage now rejects a register-source move whose source
  value has no BIR type and no prepared `size_bytes`, while confirming the
  diagnostic still identifies a register source, stack-slot destination,
  destination type, and generic move-bundle materialization failure.

## Suggested Next

Proceed to Step 5 by reassessing the representative multi-move bundle owner and
recording whether it remains outside this runbook or needs a separate follow-up
contract.

## Watchouts

The added tests intentionally preserve the Step 3 boundary: exactly one
coherent `consumer_register_to_stack` move is accepted, while multiple such
moves and missing source-size authority fail closed. Do not widen Step 5 by
dropping moves, choosing a source by order, or inferring size/location from row
names.

## Proof

Ran `cmake --build --preset default` and
`ctest --test-dir build -j --output-on-failure -R '^backend_'`; both completed
successfully with final output captured in `test_after.log`.

Also ran `git diff --check --
tests/backend/mir/backend_riscv_object_emission_test.cpp todo.md`, which
passed.
