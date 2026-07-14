# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.13
Current Step Title: Receive the checked wide builtin-ffs select narrowing

## Just Finished

- Step 7.12 complete (`9f8194fd0`): received only the checked explicit scalar
  double-to-unsigned-i32 `LirCastOp::FPToUI` from the admitted double `FAdd`
  result through its one exact later i32 `Add` use. The Raw-BIR cast kind,
  builder, verifier, source-ID registry, importer, and focused receiver test
  preserve native `FPToUI` authority and reject missing, invalid, duplicate,
  unresolved/cross-owner, wrong-endpoint/direction, and malformed-use linkage
  transactionally. The fresh focused backend/producer proof passed 2/2 in
  `test_after.log`; its matching regression guard was non-decreasing 2/2;
  fresh broader `^backend_` proof passed 4/4.

## Suggested Next

- Execute Step 7.13 only: receive PI's checked i64 `__builtin_ffsll`
  `LirSelectOp` result and its required typed i64-to-i32 `LirCastOp::Trunc`
  edge to the exact later ordinary i32 `Add` use.

## Watchouts

- Do not receive the shared ffs add-one false arm, zero-comparison condition,
  Cttz call, other select/cast producers, or any rendered-value authority.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'` (2/2).
  Proof output: `test_after.log`.
