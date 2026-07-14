# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 6.4
Current Step Title: Receive the checked explicit i32-to-i64 SExt result

## Just Finished

- Step 6.3 complete in `97efe9c38`: imported only direct unconditional
  `LirBr.successor` as the typed Raw-BIR `JumpTerm` target, preserving native
  `LirBlockId` through current-function mapping and rejecting missing, invalid,
  duplicate, cross-owner, and incoherent display-shadow authority
  transactionally.

## Suggested Next

- Execute Step 6.4 only: receive the producer-verified explicit scalar
  `LirCastOp{result: current-function LirValueId, operand: current-function
  LirValueId, kind: SExt, from_type: i32, to_type: i64}` and its exact
  downstream i64 Add use. Do not broaden Cast receipt.

## Watchouts

- The direct-branch contract remains closed: `target_label` is display shadow
  only. For Step 6.4, do not infer cast kind, endpoints, result/use identity,
  or the downstream Add from rendering. Other integer widths/kinds, no-op,
  pointer, bitcast, floating, vector, aggregate, implicit, and text-only cast
  forms remain fail-closed; CFG receipt does not widen.

## Proof

- Step 6.3 passed: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$' >
  test_after.log` (2/2); proof log: `test_after.log`.
- Step 6.4 proof: fresh build plus the supervisor-selected focused receiver
  proof, retaining `frontend_lir_call_type_ref` as the producer neighbor.
