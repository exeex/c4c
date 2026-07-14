# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.11
Current Step Title: Receive the checked explicit scalar FPToSI result

## Just Finished

- Step 7.11 complete: received only the checked explicit scalar
  double-to-signed-i32 `LirCastOp::FPToSI` from the admitted double `FAdd`
  result through its one exact later i32 `Add` use. The Raw-BIR cast kind,
  builder, verifier, source-ID registry, importer, and focused receiver test
  preserve native `FPToSI` authority and reject invalid, duplicate,
  unresolved/cross-owner, wrong-kind/endpoint, and malformed-use linkage
  transactionally.

## Suggested Next

- Runbook is exhausted after Step 7.11; ask plan-owner for the required
  explicit close, repair, replace, or conclude decision before selecting more
  work.

## Watchouts

- FPToSI remains a native-kind-authorized double-to-signed-i32 boundary only;
  FPToUI and every other cast family remain fail-closed.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'` (2/2).
  Proof output: `test_after.log`.
