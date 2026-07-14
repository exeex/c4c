# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.13
Current Step Title: Publish wider scalar select narrowing result/use authority

## Just Finished

- Completed Plan Step 7.12 for one explicit nonpointer, nonvector scalar FPToUI
  result/use chain.
- Required an authoritative Step-7.5 double FAdd source, preserved that exact
  ID into an exact double-to-i32 FPToUI allocated through `fresh_value`, and
  preserved the cast result ID into a later unsigned i32 Add.
- Extended only the authoritative-source floating-to-integer branch of the
  common operand-returning coercion seam to select FPToUI from unsigned
  destination semantics; monostate-source casts remain compatibility.
- Added exact-ID and misleading-display positives plus invalid/duplicate,
  unknown/cross-function, reverse-kind, and missing/conflicting endpoint
  rejections. The matrix records the exact boundary.

## Suggested Next

- Execute Step 7.13: publish the existing i64 builtin-ffs select through its
  required i64-to-i32 narrowing result/use chain.

## Watchouts

- Own only PI's wider `ffs` family route where the existing authoritative i64
  `LirSelectOp` result must narrow to the builtin's i32 result. Preserve the
  exact select ID as the narrowing cast operand and the exact cast result ID
  into one later ordinary i32 use.
- Route the wider result through an operand-returning narrowing seam: allocate
  the i64-to-i32 `Trunc` with `fresh_value`, retain exact native
  endpoint refs, and return the same operand through builtin CallExpr lowering.
- Require current-function ownership plus coherent integer Trunc direction;
  reject invalid/duplicate results, unknown or cross-function uses,
  missing/conflicting endpoints, and nonnarrowing or wrong-kind casts.
- Keep the select's internal cttz call, plus-one binary, zero comparison,
  condition, and false arm as honest compatibility. Never derive their IDs or
  the select/cast edge from rendered spelling.
- Preserve the Step-7.3 i32 select and all completed scalar cast contracts.
  Exclude other builtins/calls, other select or cast producers, pointer/vector/
  aggregate/object work, CFG/parameters, inline assembly, and BIR.

## Proof

- Fresh `cmake --build --preset default` passed for the Step-7.12 explicit
  FPToUI producer, verifier, and focused tests.
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure > test_after.log`
  passed 1/1 as the focused Step-7.12 proof.
- The matching narrow `test_before.log` / `test_after.log` monotonic guard
  passed with non-decreasing passed tests and no new failures.
- The supervisor's full regression guard passed 3033/3033 before and after,
  with delta 0 passed / 0 failed and no new failures; canonical proof is in
  `test_before.log` and `test_after.log`.
- `git diff --check` passed for the complete Step-7.12 slice.
