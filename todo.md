# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.12
Current Step Title: Receive the checked explicit scalar FPToUI result

## Just Finished

- Step 7.12 complete: received only the checked explicit scalar
  double-to-unsigned-i32 `LirCastOp::FPToUI` from the admitted double `FAdd`
  result through its one exact later i32 `Add` use. The Raw-BIR cast kind,
  builder, verifier, source-ID registry, importer, and focused receiver test
  preserve native `FPToUI` authority and reject missing, invalid, duplicate,
  unresolved/cross-owner, wrong-endpoint/direction, and malformed-use linkage
  transactionally. The fresh focused backend/producer proof passed 2/2 in
  `test_after.log`.

## Suggested Next

- Runbook exhausted. Send the active plan to plan-owner for the explicit source
  completion, repair, replacement, or conclusion decision; do not infer closure.

## Watchouts

- FPToSI and FPToUI are both admitted only as distinct native kinds; signless
  i32 type refs do not independently encode signedness. No further executor
  packet is selected while the runbook is exhausted.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'` (2/2).
  Proof output: `test_after.log`.
