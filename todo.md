# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.10
Current Step Title: Receive the checked explicit scalar UIToFP result

## Just Finished

- Step 7.9 complete: received only the checked explicit scalar signed
  i32-to-double `LirCastOp::SIToFP` from the admitted i32 Add result and its
  one exact later double `FMul` use, with typed Raw-BIR payload/builder/view,
  reachable verification, source-ID registry receipt, and transactional
  rejection coverage.

## Suggested Next

- Execute Step 7.10 only: receive the producer-authorized explicit scalar
  unsigned i32-to-double `LirCastOp::UIToFP` from the admitted unsigned i32
  Add result through its one exact later double `FMul` use. Require native
  `UIToFP` kind authority because signless i32 type refs do not independently
  distinguish a UIToFP/SIToFP kind swap; retain all other conversion families
  and presentation-derived authority fail-closed.

## Watchouts

- UIToFP is a native-kind-authorized unsigned i32-to-double boundary only.
  SIToFP is accepted historical work, while FPToSI, FPToUI, all other cast
  families, non-scalar forms, and presentation-derived authority remain
  fail-closed.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'` (2/2).
