# Current Packet

Status: Active
Source Idea Path: ideas/open/752_lir_local_object_pointer_authority_convergence.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish the selected local-object authority contract
你該做code review了

## Just Finished

- Step 1 implementation was committed in `ca26a8242`; its focused coverage
  passed. Supervisor review identified a remaining VLA pointer-slot-store
  authority mismatch, so Step 1 is not accepted as complete.

## Suggested Next

- Repair the VLA pointer-slot-store authority so it describes that store's
  pointer operand, then rerun the bounded Step 1 proof. Do not start Step 2
  malformed-authority checks or Step 3 handoff work.

## Watchouts

- Stack restore is emitted only on a backward goto with a VLA lifetime route;
  ordinary VLA fixtures do not cover it. Keep authority selection keyed by
  LocalId/current-function facts, never local names, `%t`, or formatted
  operands. Keep Raw-BIR/importer, memory/va, aggregate/vector, PHI/CFG, and
  target-lowering outside this packet.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$' > test_after.log` passed (1/1). `test_after.log` contains the result.
