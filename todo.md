# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.9
Current Step Title: Receive the checked explicit scalar SIToFP result

## Just Finished

- Step 7.9 complete: received only the checked explicit scalar signed
  i32-to-double `LirCastOp::SIToFP` from the admitted i32 Add result and its
  one exact later double `FMul` use, with typed Raw-BIR payload/builder/view,
  reachable verification, source-ID registry receipt, and transactional
  rejection coverage.

## Suggested Next

- Return the exhausted Step-7 runbook to the plan owner for the explicit
  source-completion-gate decision; do not infer source-idea completion.

## Watchouts

- SIToFP remains a native-kind-authorized signed i32-to-double boundary only:
  UIToFP and all other cast families, non-scalar forms, and presentation-derived
  authority remain fail-closed. Native LIR signless i32 type refs cannot
  distinguish signedness independently; the exact SIToFP kind does so.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'` (2/2).
