# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.5
Current Step Title: Publish representative scalar floating binary result/use authority

## Just Finished

- Completed Plan Step 7.4 for the existing integer `abs`/`labs`/`llabs`
  `LirAbsOp` result/use route.
- Routed the argument through the common operand/coercion seam, allocated the
  result through `fresh_value`, stored exact i32/i64 type authority, and
  preserved the exact result ID into a later ordinary Add use.
- Preserved selected-global SSA and immediate argument authority where
  structurally available while retaining honest monostate SSA compatibility
  for sources without native identity.
- Added focused exact-ID, immediate-neighbor, and misleading-display positives
  plus missing/invalid/duplicate result, unknown/cross-function use, type, and
  missing/wrong argument rejections. The matrix records the exact boundary.

## Suggested Next

- Execute Step 7.5: publish representative scalar floating `LirBinOp`
  result/use authority.

## Watchouts

- Own only one ordinary nonpointer, nonvector scalar floating `LirBinOp`
  two-operation result/use chain with exact native floating opcode and type
  authority.
- Allocate both results through the common `fresh_value`/`LirOperand` path and
  preserve the exact first result ID into the later operation in the same
  function.
- Keep structurally unavailable initial floating operands as honest monostate
  compatibility; do not add a floating-immediate carrier or infer IDs from
  display spelling.
- Accept misleading display after native authority is proven; reject invalid
  or duplicate results, unknown or cross-function uses, invalid opcode, and
  missing or conflicting type authority.
- Keep complex, vector, pointer, logical-helper, cast, compare, select, abs,
  other Step-7 rows, CFG/terminators, parameters, calls, inline assembly, and
  new-BIR work outside Step 7.5.

## Proof

- Fresh `cmake --build --preset default` passed for the Step-7.4 abs
  producer, verifier, and focused tests.
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure > test_after.log`
  passed 1/1 as the focused Step-7.4 proof.
- The matching narrow `test_before.log` / `test_after.log` monotonic guard
  passed with non-decreasing passed tests and no new failures.
- The supervisor's clean-stashed full regression passed 3033/3033 both before
  and after this slice, with delta 0 passed / 0 failed and no new failures;
  canonical full-regression proof is in `test_before.log` and `test_after.log`.
- `git diff --check` passed for the complete Step-7.4 slice.
