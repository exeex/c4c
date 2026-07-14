# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.12
Current Step Title: Receive the checked explicit scalar FPToUI result

## Just Finished

- Step 7.11 complete (`3e45cef35`): received only the checked explicit scalar
  double-to-signed-i32 `LirCastOp::FPToSI` from the admitted double `FAdd`
  result through its one exact later i32 `Add` use. The Raw-BIR cast kind,
  builder, verifier, source-ID registry, importer, and focused receiver test
  preserve native `FPToSI` authority and reject invalid, duplicate,
  unresolved/cross-owner, wrong-kind/endpoint, and malformed-use linkage
  transactionally. The focused backend/producer proof passed 2/2 in
  `test_after.log`; its matching regression guard was non-decreasing 2/2;
  fresh broader `^backend_` proof passed 4/4.

## Suggested Next

- Execute Step 7.12 only: receive the authority-matrix explicit scalar
  double-to-unsigned-i32 `LirCastOp::FPToUI` from the admitted double `FAdd`
  result through its exact later unsigned i32 `Add` use.

## Watchouts

- FPToUI requires its native cast kind because signless i32 type refs cannot
  distinguish it from FPToSI. Every other cast family remains fail-closed.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'` (2/2).
  Proof output: `test_after.log`.
