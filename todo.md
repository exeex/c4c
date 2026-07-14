# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.4
Current Step Title: Publish representative scalar abs result/use authority

## Just Finished

- Completed Plan Step 7.3 for the current i32 scalar builtin-ffs `LirSelectOp`
  result/use route.
- Allocated the select result through `fresh_value`, preserved exact i32 type
  authority and native zero-immediate authority, and returned the exact result
  operand into a later ordinary Add use.
- Kept the condition and false arm as honest monostate SSA compatibility
  because their internal producers lack authority; wider ffs narrowing also
  remains compatibility.
- Added focused exact-ID and misleading-display positives plus missing,
  invalid, duplicate, unknown/cross-function, type, and malformed-condition
  rejections. The matrix records this exact current-producer boundary.

## Suggested Next

- Execute Step 7.4: publish representative scalar `LirAbsOp` result/use
  authority.

## Watchouts

- Own only the current integer `LirAbsOp` producer with exact integer type,
  structurally available argument authority, and a `fresh_value` result.
- Preserve that exact result ID into a later ordinary use in the same function;
  do not reconstruct either operand from display spelling.
- Accept misleading display after native authority is proven; reject invalid
  or duplicate results, unknown or cross-function uses, and missing or
  conflicting type or argument alternatives.
- Keep every other Step-7 row and all pointer/object, aggregate/vector,
  CFG/terminator, parameter, call, inline-assembly, ABI, and new-BIR families
  outside Step 7.4.

## Proof

- Fresh `cmake --build --preset default` passed for the Step-7.3 select
  producer, verifier, and focused tests.
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure > test_after.log`
  passed 1/1 as the focused Step-7.3 proof.
- The matching narrow `test_before.log` / `test_after.log` monotonic guard
  passed with non-decreasing passed tests and no new failures.
- The supervisor's clean-stashed full regression passed 3033/3033 both before
  and after this slice, with delta 0 passed / 0 failed and no new failures;
  canonical full-regression proof is in `test_before.log` and `test_after.log`.
- `git diff --check` passed for the complete Step-7.3 slice.
