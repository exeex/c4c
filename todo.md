# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.9
Current Step Title: Receive the checked explicit scalar SIToFP result

## Just Finished

- Step 7.8 complete in `dff35977c`: received only the producer-verified
  explicit scalar `float`-to-`double` `LirCastOp::FPExt` from the admitted
  scalar FPTrunc result and its one exact later double `FMul` use, preserving
  native cast, endpoint types, source IDs, and current-function SSA edges.

## Suggested Next

- Execute Step 7.9 only: receive the authority-matrix explicit signed
  i32-to-double `LirCastOp::SIToFP` from the admitted signed i32 Add result
  and its exact later double `FMul` use, with typed Raw-BIR receipt,
  verification, rollback coverage, and focused backend/producer proof.

## Watchouts

- Step 7.9 admits only the checked signed scalar i32-to-double SIToFP
  boundary. UIToFP, all reverse and other cast families, non-scalar forms,
  and presentation-derived authority remain fail-closed. The source remains
  incomplete until every valid current-LIR row has an evidenced typed receiver
  disposition.

## Proof

- Required after Step 7.9: `cmake --build --preset default && ctest --test-dir
  build -j --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'` (2/2), plus
  the supervisor-selected regression and broader checkpoints when requested.
