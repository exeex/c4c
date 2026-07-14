# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.5
Current Step Title: Publish representative scalar floating binary result/use authority (complete)

## Just Finished

- Completed Plan Step 7.5 for one ordinary nonpointer, nonvector scalar
  floating `LirBinOp` two-operation result/use chain.
- Reused the common `fresh_value`/`LirOperand` path for double FAdd and FMul,
  retained exact floating opcode/type authority, and preserved the exact FAdd
  result ID into the later FMul lhs.
- Kept initial floating literals as honest monostate compatibility operands;
  no floating-immediate carrier or display-derived identity was introduced.
- Added exact-ID and misleading-display positives plus invalid/duplicate
  result, unknown/cross-function use, invalid/conflicting opcode, and
  missing/conflicting type rejections. The matrix records the exact boundary.

## Suggested Next

- Select the next bounded Step-7 scalar row from the updated matrix; the
  ordinary scalar floating comparison route is the nearest analogous candidate.

## Watchouts

- The Step-7.5 producer test must stay after complex/vector/pointer/logical
  route exits; do not let the shared `LirBinOp` carrier imply those producers.
- Floating literal inputs remain monostate because no native floating-immediate
  carrier exists; do not infer authority from their rendered constants.
- The authority-scoped verifier permits compatibility operations without a
  native result but requires floating opcode/type agreement once one is present.
- Preserve Steps 3 through 7.5 and the four idea-741 neighbors. Keep other
  comparison/binary producers, CFG/parameters, calls, inline assembly, and BIR
  outside the next packet unless separately delegated.

## Proof

- Fresh `cmake --build --preset default` passed for the Step-7.5 scalar
  floating binary producer, verifier, and focused tests.
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure > test_after.log`
  passed 1/1 as the focused Step-7.5 proof.
- The matching narrow `test_before.log` / `test_after.log` monotonic guard
  passed with non-decreasing passed tests and no new failures.
- The supervisor's clean-stashed full regression passed 3033/3033 both before
  and after this slice, with delta 0 passed / 0 failed and no new failures;
  canonical full-regression proof is in `test_before.log` and `test_after.log`.
- `git diff --check` passed for the complete Step-7.5 slice.
