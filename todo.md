# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.10
Current Step Title: Publish explicit unsigned-integer-to-floating cast authority

## Just Finished

- Completed Plan Step 7.9 for one explicit nonpointer, nonvector scalar SIToFP
  result/use chain.
- Required an authoritative Step-6 signed i32 Add source, preserved that exact
  ID into an exact i32-to-double SIToFP allocated through `fresh_value`, and
  preserved the cast result ID into a later double FMul.
- Extended only the authoritative-source signed-integer-to-floating branch of
  the common operand-returning coercion seam; UIToFP, reverse conversions, and
  monostate-source casts remain compatibility.
- Added exact-ID and misleading-display positives plus invalid/duplicate,
  unknown/cross-function, wrong-kind, and missing/conflicting endpoint
  rejections. The matrix records the exact boundary.

## Suggested Next

- Execute Step 7.10: publish one explicit authoritative-source scalar `UIToFP`
  result/use chain.

## Watchouts

- Own only PX's explicit nonpointer, nonvector conversion from an unsigned
  scalar integer to a scalar floating type. Use an authoritative Step-6
  unsigned integer result as the exact `UIToFP` operand and preserve the cast
  result into a later ordinary floating operation.
- Reuse the authoritative-source scalar conversion seam: allocate through
  `fresh_value`, retain native `UIToFP` plus exact integer source and floating
  destination type refs, and return the identical result operand to the later
  use.
- Require authoritative kind and endpoint families to agree; reject invalid or
  duplicate results, unknown or cross-function uses, missing/conflicting
  endpoints, floating sources, integer destinations, and non-UIToFP kinds.
- Accept misleading displays only after native authority is proven. Keep
  monostate-source conversion and text-only neighbors honest compatibility;
  never infer identity or unsignedness from rendered spelling.
- Preserve Step-7.9 SIToFP and the earlier scalar cast rules. Exclude `FPToSI`,
  `FPToUI`, pointer, bitcast, vector, complex, aggregate, implicit coercion,
  other cast producers, CFG/parameters, calls, inline assembly, and BIR.

## Proof

- Fresh `cmake --build --preset default` passed for the Step-7.9 explicit
  SIToFP producer, verifier, and focused tests.
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure > test_after.log`
  passed 1/1 as the focused Step-7.9 proof.
- The matching narrow `test_before.log` / `test_after.log` monotonic guard
  passed with non-decreasing passed tests and no new failures.
- The supervisor's full regression guard passed 3033/3033 before and after,
  with delta 0 passed / 0 failed and no new failures; canonical proof is in
  `test_before.log` and `test_after.log`.
- `git diff --check` passed for the complete Step-7.9 slice.
