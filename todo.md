# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.9
Current Step Title: Publish explicit signed-integer-to-floating cast authority

## Just Finished

- Completed Plan Step 7.8 for one explicit nonpointer, nonvector scalar
  floating FPExt result/use chain.
- Required an authoritative Step-7.5 float FAdd source, preserved that exact ID
  into an exact float-to-double FPExt allocated through `fresh_value`, and
  preserved the cast result ID into a later double FMul.
- Extended only the authoritative-source representation-changing floating
  branch of the common operand-returning coercion seam; same-representation
  and monostate-source casts remain compatibility.
- Added exact-ID and misleading-display positives plus invalid/duplicate,
  unknown/cross-function, wrong-kind, missing/conflicting endpoint, and
  direction rejections. The matrix records the exact boundary.

## Suggested Next

- Execute Step 7.9: publish one explicit authoritative-source scalar `SIToFP`
  result/use chain.

## Watchouts

- Own only PX's explicit nonpointer, nonvector conversion from a signed scalar
  integer to a scalar floating type. Use an authoritative Step-6 signed integer
  result as the exact `SIToFP` operand and preserve the cast result into a later
  ordinary floating operation.
- Extend `coerce_operand` only for an authoritative source and exact scalar
  endpoint families: allocate through `fresh_value`, retain native `SIToFP`
  plus exact integer source and floating destination type refs, and return the
  identical result operand to the later use.
- Require the authoritative cast kind and endpoint families to agree; reject
  invalid/duplicate results, unknown or cross-function uses, missing or
  conflicting endpoints, floating sources, integer destinations, and other
  cast kinds on the SIToFP claim.
- Accept misleading displays only after native authority is proven. Keep
  monostate-source conversion and all text-only neighbors honest compatibility;
  never infer identity or signedness from rendered spelling.
- Preserve Steps 7.1, 7.7, and 7.8. Exclude `UIToFP`, `FPToSI`, `FPToUI`,
  pointer, bitcast, vector, complex, aggregate, implicit coercion, other cast
  producers, CFG/parameters, calls, inline assembly, and BIR.

## Proof

- Fresh `cmake --build --preset default` passed for the Step-7.8 explicit
  FPExt producer, verifier, and focused tests.
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure > test_after.log`
  passed 1/1 as the focused Step-7.8 proof.
- The matching narrow `test_before.log` / `test_after.log` monotonic guard
  passed with non-decreasing passed tests and no new failures.
- The supervisor's full regression guard passed 3033/3033 before and after,
  with delta 0 passed / 0 failed and no new failures; canonical proof is in
  `test_before.log` and `test_after.log`.
- `git diff --check` passed for the complete Step-7.8 slice.
