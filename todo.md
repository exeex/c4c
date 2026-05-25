Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Preservation Reconstruction Authority Leak

# Current Packet

## Just Finished

Step 1 selected one preservation reconstruction authority leak for the next
implementation packet: `find_prior_stack_preserved_value_before_instruction`
in `calls_preservation.cpp`, consumed by before-instruction move lowering in
`calls_moves.cpp`.

Selected local reconstruction/fallback to remove:
`find_prior_stack_preserved_value_before_instruction` locally refinds prepared
call plans, walks `call_plans->calls` in reverse, filters same-block calls
before an arbitrary instruction index, and rechecks stack-preservation shape
near AArch64 emission.

Prepared replacement route:
use the existing `context.function.call_plan_lookups` authority backed by
`PreparedCallPlanLookups::prior_preserved_by_value` and
`prepare::find_latest_indexed_prior_preserved_value`. The Step 2 slice should
query that indexed prior-preserved table at the current block/instruction
position, then keep only emission-side validation that the returned
`PreparedCallPreservedValue` is a complete `StackSlot` preservation record.
If the current helper cannot safely use the existing indexed API without
creating a synthetic current-position key, the precise blocker is a missing
prepared lookup helper shaped as: latest prior stack-preserved value by
`PreparedValueId`, `block_index`, and `instruction_index`.

## Suggested Next

Execute Step 2 by replacing
`find_prior_stack_preserved_value_before_instruction` with prepared lookup
consumption or, if the synthetic-position route is rejected during
implementation, stop with the missing prepared lookup helper blocker recorded
above. Remove obsolete local call-plan fallback parameters only if the selected
helper no longer needs them.

## Watchouts

- Do not work on `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not weaken tests or mark nearby preservation/publication cases
  unsupported to claim progress.
- Keep this slice to the selected before-instruction stack-preserved lookup;
  do not fold in callee-saved republication/population or block-entry
  non-call-use reconstruction.
- The existing lookup returns prior preserved values by value id and position;
  Step 2 must still validate `StackSlot`, slot id, stack offset, nonzero stack
  size, and alignment fallback before building the `MemoryOperand`.

## Proof

No build was required for this selection-only Step 1 packet, and no
`test_after.log` was produced.

Evidence commands used:

- `sed -n '1,220p' todo.md`
- `sed -n '1,240p' plan.md`
- `rg -n "find_prior_preserved_value|preserved_value_has_|lower_before_call_moves|lower_after_call_moves|plan_prepared_call_boundary_effects|Prepared" src/backend/mir/aarch64/codegen src/backend/prealloc tests/backend/mir`
- `sed -n '220,620p' src/backend/mir/aarch64/codegen/calls_preservation.cpp`
- `sed -n '3320,3405p' src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `sed -n '140,220p' src/backend/prealloc/prepared_lookups.cpp`
- `sed -n '1,160p' src/backend/prealloc/prepared_lookups.hpp`

Next Step 2 proof command:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`
