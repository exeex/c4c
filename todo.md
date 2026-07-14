# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.11
Current Step Title: Publish explicit floating-to-signed-integer cast authority (complete)

## Just Finished

- Completed Plan Step 7.11 for one explicit nonpointer, nonvector scalar FPToSI
  result/use chain.
- Required an authoritative Step-7.5 double FAdd source, preserved that exact
  ID into an exact double-to-i32 FPToSI allocated through `fresh_value`, and
  preserved the cast result ID into a later signed i32 Add.
- Extended only the authoritative-source floating-to-signed-integer branch of
  the common operand-returning coercion seam; FPToUI and monostate-source casts
  remain compatibility.
- Added exact-ID and misleading-display positives plus invalid/duplicate,
  unknown/cross-function, reverse-kind, and missing/conflicting endpoint
  rejections. The matrix records the exact boundary.

## Suggested Next

- Select the next bounded Step-7 matrix row whose result allocation, operand
  propagation, type authority, and verifier rules form one coherent packet.

## Watchouts

- Preserve Step-7.11's authoritative floating source, production-selected
  FPToSI kind, exact floating/integer endpoints, and result/use identity.
- Native LIR integer refs are signless: production selects FPToSI from signed
  destination `TypeSpec`; verification must not infer signedness or identity
  from rendered spelling.
- Exclude `FPToUI`, pointer, bitcast, vector, complex, aggregate, implicit
  coercion, other cast producers, CFG/parameters, calls, inline assembly, and
  BIR from this completed packet.

## Proof

- Fresh `cmake --build --preset default` passed for the Step-7.11 explicit
  FPToSI producer, verifier, and focused tests.
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure > test_after.log`
  passed 1/1 as the focused Step-7.11 proof.
- The matching narrow `test_before.log` / `test_after.log` monotonic guard
  passed with non-decreasing passed tests and no new failures.
- The supervisor's full regression guard passed 3033/3033 before and after,
  with delta 0 passed / 0 failed and no new failures; canonical proof is in
  `test_before.log` and `test_after.log`.
- `git diff --check` passed for the complete Step-7.11 slice.
