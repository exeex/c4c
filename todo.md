Status: Active
Source Idea Path: ideas/open/395_rv64_object_route_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Lower The First Reusable Fragment Family

# Current Packet

## Just Finished

Step 2 - Lower The First Reusable Fragment Family repaired the RV64 object
route for scalar GPR call arguments sourced from stack-slot
`PriorPreservation`. `fragment_for_prepared_call()` now preserves the existing
callee-saved-register move path and, when the prior preservation route is a
prepared stack slot, validates the source/preserved slot id, stack offset,
size, alignment, scalar argument size, frame slot, and signed-12-bit stack
offset before loading the preserved scalar into the ABI destination register.

This completes the first reusable family represented by `src/20000223-1.c`:
same-module/direct fixed-arity prepared calls whose GPR argument value was
preserved in a prepared stack slot.

## Suggested Next

Delegate the next Step 2 object-route packet against one of the still-red
representatives in `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`,
starting by classifying the first unsupported instruction fragment in
`src/20000722-1.c`.

## Watchouts

- Unsupported banks, aggregate transports, missing size/offset facts, and
  non-scalar stack-slot prior-preservation shapes still fail closed.
- The delegated proof now has `src/20000223-1.c` passing, but the other four
  representatives remain adjacent buckets and were not repaired by this slice.
- Keep `prepared_call_emit.*` out of this route unless a future proof shows the
  text emitter, not `--codegen obj`, is the active blocker.

## Proof

Ran the delegated proof exactly. `test_after.log` is the proof log:
`[rv64-gcc-torture] total=5 passed=1 failed=4`, with
`pass src/20000223-1.c`. The final guard verified that
`build/rv64_gcc_c_torture_backend/src_20000223-1.c/case.log` no longer
contains `unsupported_instruction_fragment` or
`unsupported RV64 object lowering`.
