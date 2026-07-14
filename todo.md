# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.7
Current Step Title: Publish explicit scalar floating cast result/use authority (complete)

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

- Select the next bounded Step-7 row from the updated matrix without widening
  the FPTrunc claim into its explicitly excluded neighboring cast families.

## Watchouts

- Native FPTrunc requires both an authoritative source ID and strict scalar
  floating narrowing; do not infer a claim from kind/type spelling alone.
- FPExt, integer/floating conversions, pointer/bitcast/vector/complex/
  aggregate casts, implicit coercions, and monostate-source casts remain
  compatibility.
- The verifier's other authoritative cast path remains the Step-7.1 integer
  Trunc/ZExt/SExt contract; do not weaken either endpoint/direction check.
- Preserve Steps 3 through 7.7 and idea-741 neighbors. Keep other producers,
  CFG/parameters, calls, inline assembly, and BIR outside the next packet.

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
