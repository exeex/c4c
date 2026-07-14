# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.4
Current Step Title: Publish representative scalar abs result/use authority (complete)

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

- Execute the Step-8 producer-boundary re-enumeration and verifier audit against
  the completed scalar producer packets and exact remaining-row blockers.

## Watchouts

- The Step-7.4 claim covers only the existing integer abs special branch; no
  other builtin, direct, indirect, variadic, or ABI call route was broadened.
- Abs arguments without structurally published identity remain monostate SSA;
  do not reconstruct authority from their displays during the Step-8 audit.
- Aggregate/vector rows still need their own opcode/type/index/mask contracts;
  pointer/object, CFG/parameters, inline assembly, and BIR remain separate.
- Preserve the four idea-741 neighbors and completed Steps 3 through 7.4 while
  auditing exact blockers; do not turn an ownership-ready carrier into a false
  producer guarantee.

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
