# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.6
Current Step Title: Publish ordinary scalar floating comparison result/use authority

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

- Execute Step 7.6: publish ordinary scalar floating `LirCmpOp` result/use
  authority.

## Watchouts

- Own only PB's ordinary nonpointer, nonvector scalar floating comparison
  branch after complex, vector, pointer, and logical-helper routes have exited.
- Allocate the comparison result through `fresh_value`, retain the native
  floating predicate and exact compared `LirTypeRef`, and pass that exact
  result ID to the existing i1-to-i32 normalization cast operand.
- Keep floating literal inputs as honest monostate because there is no native
  floating-immediate carrier; never infer authority from rendered constants.
- Require authoritative floating mode, floating predicate, and floating type
  to agree; reject invalid/duplicate results, unknown or cross-function uses,
  integer predicates/types on the floating claim, and float-mode conflicts.
- Keep the normalization cast result compatibility-only. Exclude pointer,
  vector, complex, logical-helper, builtin, vaarg, statement, and other
  comparison producers, as well as CFG/parameters, calls, inline assembly,
  aggregate/object work, and BIR.

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
