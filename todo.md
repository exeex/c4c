Status: Active
Source Idea Path: ideas/open/584_rv64_stack_destination_move_bundle_authority_contract.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Consume Authority In RV64

# Current Packet

## Just Finished

Step 4: Consume Authority In RV64 completed.

Changed files:
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/prealloc/prepared_object_traversal.cpp`
- `src/backend/prealloc/prepared_object_traversal.hpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`
- `test_after.log`

Implemented:
- Taught RV64 prepared move-bundle emission to fail closed for register fan-in
  into one stack destination unless the bundle carries
  `StackDestinationRegisterFanIn` authority.
- Allowed RV64 register-to-stack and stack-to-stack prepared moves to emit under
  `StackDestinationRegisterFanIn` only when the individual move carries the
  same authority as the bundle.
- Added a prepared consumer diagnostic category for contradictory
  `StackDestinationRegisterFanIn` bundle/move facts.
- Added RV64 coverage for missing, unknown, unsupported, contradictory, and
  matching stack-destination fan-in authority, including positive emission after
  producer publication.

## Suggested Next

Delegate the next plan step or plan-owner review if Step 4 completes the current
runbook slice.

## Watchouts

- The shared prepared-object traversal contract still classifies the legacy
  select-materialized stack-destination shape as available; RV64 now performs
  its own fail-closed check before materialization so generic traversal tests do
  not need to change.
- The RV64 positive path does not rediscover producer select legality; it only
  requires matching `StackDestinationRegisterFanIn` facts on the bundle and
  moves, then relies on ordinary RV64 move materialization support.

## Proof

Ran exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains the proof output; CTest reports
`Total Test time (real) =   1.98 sec`.
