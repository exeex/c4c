# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.7
Current Step Title: Publish explicit scalar floating cast result/use authority

## Just Finished

- Completed Plan Step 7.6 for PB's ordinary nonpointer, nonvector scalar
  floating `LirCmpOp` result/use route.
- Allocated the comparison through `fresh_value`, retained native floating
  mode, OLt predicate, and exact double type, and passed the exact result ID to
  the existing i1-to-i32 normalization cast operand.
- Kept both floating literals as honest monostate compatibility operands and
  left the normalization cast result unclaimed.
- Added exact-ID and misleading-display positives plus invalid/duplicate
  result, unknown/cross-function use, invalid/wrong-family predicate, and
  missing/conflicting type/mode rejections. The matrix records the boundary.

## Suggested Next

- Execute Step 7.7: publish one explicit representation-changing scalar
  floating `LirCastOp` result/use chain.

## Watchouts

- Own only PX's explicit nonpointer, nonvector scalar floating-to-floating cast
  whose source and destination representations differ; use one FPTrunc chain
  from an authoritative Step-7.5 floating result into a later ordinary
  floating operation.
- Extend the operand-returning `coerce_operand` seam so the cast allocates
  through `fresh_value`, consumes the exact source result ID, retains native
  `FPTrunc` plus exact floating from/to type refs, and returns the same cast
  result ID to the later use.
- Require authoritative floating cast kind, type family, and width direction
  to agree; reject invalid/duplicate results, unknown or cross-function uses,
  missing/conflicting types, nonfloating endpoints, and nonnarrowing FPTrunc.
- Accept misleading display only after native authority is proven. Keep
  same-representation no-op casts and structurally unavailable floating
  literals as honest compatibility; never reconstruct IDs from spelling.
- Exclude FPExt and integer/floating conversion expansion beyond the focused
  FPTrunc row, plus pointer, bitcast, vector, complex, aggregate, implicit
  coercion, other cast producers, CFG/parameters, calls, inline assembly, and
  BIR.

## Proof

- Fresh `cmake --build --preset default` passed for the Step-7.6 scalar
  floating comparison producer, verifier, and focused tests.
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure > test_after.log`
  passed 1/1 as the focused Step-7.6 proof.
- The matching narrow `test_before.log` / `test_after.log` monotonic guard
  passed with non-decreasing passed tests and no new failures.
- The supervisor's clean-stashed full regression passed 3033/3033 both before
  and after this slice, with delta 0 passed / 0 failed and no new failures;
  canonical full-regression proof is in `test_before.log` and `test_after.log`.
- `git diff --check` passed for the complete Step-7.6 slice.
