# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.1
Current Step Title: Publish representative scalar cast result/use authority

## Just Finished

- Completed Plan Step 6 for the focused two-operation ordinary scalar integer
  `LirBinOp` result/use chain.
- Added an operand-returning binary-expression seam: normalized scalar integer
  arithmetic allocates results with `fresh_value`, returns the common
  `LirOperand`, and preserves input authority only across representation-
  preserving coercion.
- Preserved exact native Add/Mul and i32 facts and proved that the later Mul lhs
  carries the exact earlier Add result ID. Other binary branches retain
  monostate compatibility.
- Added focused misleading-display acceptance and rejection for invalid or
  duplicate results, unknown or cross-function uses, invalid opcode, and
  missing type authority. Updated the matrix with exact shared versus distinct
  neighboring row classifications.

## Suggested Next

- Execute Step 7.1: publish representative scalar cast result/use authority.

## Watchouts

- Own only one representation-preserving ordinary scalar integer `LirCastOp`
  chain with a later ordinary use in the same function.
- Allocate the cast result through the common `fresh_value`/`LirOperand`
  ownership path and preserve that exact ID into the later use.
- Preserve native cast-kind authority plus exact from/to type refs; do not
  infer semantics from rendered operands or type spelling.
- Accept misleading display after native authority is proven; reject invalid
  or duplicate results, unknown or cross-function uses, invalid cast kind, and
  from/to type conflicts.
- Keep compare, select, abs, every other Step-7 row, pointer/object,
  aggregate/vector, CFG/terminators, parameters, calls, inline assembly, and
  new-BIR work outside Step 7.1.

## Proof

- Fresh `cmake --build --preset default` passed for the Step-6 producer seam
  and focused tests.
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure > test_after.log`
  passed 1/1; canonical proof is in `test_after.log`.
- The supervisor clean-stashed full regression guard passed:
  `test_before.log` passed 3033/3033, `test_after.log` passed 3033/3033, and
  the monotonic delta was passed=0 and failed=0 with no new failures.
- `git diff --check` passed for the complete Step-6 slice.
