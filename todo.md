# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.10
Current Step Title: Receive the checked explicit scalar UIToFP result

## Just Finished

- Step 7.10 complete: received only the checked explicit scalar unsigned
  i32-to-double `LirCastOp::UIToFP` from the admitted i32 Add result and its
  one exact later double `FMul` use, with a distinct typed Raw-BIR cast kind,
  builder/view/verifier receipt, source-ID registry path, and transactional
  rejection coverage. The positive receiver assertion proves that native
  `UIToFP` maps to `CastKind::UIToFP`, preserving authority beyond signless
  i32 endpoint types.

## Suggested Next

- Request the supervisor's next bounded packet; Step 7.10 is complete and no
  further cast family is authorized by this packet.

## Watchouts

- UIToFP is a native-kind-authorized unsigned i32-to-double boundary only.
  SIToFP remains independently accepted historical work; FPToSI, FPToUI, all
  other cast families, non-scalar forms, and presentation-derived authority
  remain fail-closed.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'` (2/2).
  Proof output: `test_after.log`.
