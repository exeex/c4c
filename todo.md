# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.8
Current Step Title: Publish explicit scalar floating extension result/use authority

## Just Finished

- Completed Plan Step 7.7 for one explicit nonpointer, nonvector scalar
  floating FPTrunc result/use chain.
- Required an authoritative Step-7.5 double FAdd source, preserved that exact
  ID into an exact double-to-float FPTrunc allocated through `fresh_value`, and
  preserved the cast result ID into a later float FMul.
- Extended only the authoritative-source narrowing branch of the common
  operand-returning coercion seam; FPExt and monostate-source casts remain
  compatibility.
- Added exact-ID and misleading-display positives plus invalid/duplicate,
  unknown/cross-function, wrong-kind, endpoint, and direction rejections. The
  matrix records the exact boundary.

## Suggested Next

- Execute Step 7.8: publish one explicit authoritative-source scalar `FPExt`
  result/use chain.

## Watchouts

- Own only PX's explicit nonpointer, nonvector scalar floating-to-floating cast
  with a wider destination; use a float FAdd source and preserve its exact ID
  through FPExt into a later double floating operation.
- Reuse the authoritative-source floating branch of `coerce_operand`: allocate
  the cast through `fresh_value`, retain native `FPExt` with exact float source
  and double destination type refs, and return the identical result operand to
  the later use.
- Require authoritative floating kind, endpoint family, and widening direction
  to agree; reject invalid/duplicate results, unknown or cross-function uses,
  missing/conflicting types, nonfloating endpoints, and nonwidening FPExt.
- Accept misleading displays only after native authority is proven. Keep
  same-representation no-op and monostate-source casts honest compatibility;
  never reconstruct identity from rendered spelling.
- Preserve Step-7.7 FPTrunc and Step-7.1 integer cast rules. Exclude integer/
  floating conversions, pointer, bitcast, vector, complex, aggregate, implicit
  coercion, other cast producers, CFG/parameters, calls, inline assembly, and
  BIR.

## Proof

- Fresh `cmake --build --preset default` passed for the Step-7.7 explicit
  FPTrunc producer, verifier, and focused tests.
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure > test_after.log`
  passed 1/1 as the focused Step-7.7 proof.
- The matching narrow `test_before.log` / `test_after.log` monotonic guard
  passed with non-decreasing passed tests and no new failures.
- The supervisor's full clean-stashed regression passed 3033/3033 both before
  and after this slice, with delta 0 passed / 0 failed and no new failures;
  canonical full-regression proof is in `test_before.log` and `test_after.log`.
- `git diff --check` passed for the complete Step-7.7 slice.
