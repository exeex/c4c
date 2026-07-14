# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.6
Current Step Title: Publish ordinary scalar floating comparison result/use authority (complete)

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

- Select the next bounded Step-7 scalar row from the updated matrix; a
  representation-changing scalar floating cast is the nearest candidate.

## Watchouts

- The Step-7.6 claim is only PB's ordinary scalar floating comparison after all
  excluded route exits; do not infer coverage for other `LirCmpOp` producers.
- Floating literals and the normalization cast result remain compatibility;
  do not reconstruct authority from their rendered spellings.
- Authority-scoped comparison verification now accepts coherent integer and
  floating claims but rejects mode/predicate/type family disagreement.
- Preserve Steps 3 through 7.6 and idea-741 neighbors. Keep pointer/vector/
  complex/logical/builtin/vaarg/statement comparisons, aggregate/object,
  CFG/parameters, calls, inline assembly, and BIR outside the next packet.

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
