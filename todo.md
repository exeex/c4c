# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.13
Current Step Title: Publish wider scalar select narrowing result/use authority (complete)

## Just Finished

- Completed Plan Step 7.13 for only the i64 builtin-ffs select narrowing chain.
- Preserved the exact authoritative i64 select ID into an i64-to-i32 Trunc
  allocated by the existing integer `coerce_operand` seam through
  `fresh_value`, then preserved the cast result ID into a later i32 Add.
- Kept the internal cttz call, plus-one binary, zero comparison, select
  condition, and false arm honest compatibility.
- Added exact-ID and misleading-display positives plus invalid/duplicate,
  unknown/cross-function, wrong-kind, missing/conflicting endpoint, and
  nonnarrowing rejections. The matrix records the exact boundary.

## Suggested Next

- Select the next bounded Step-7 matrix row whose result allocation, operand
  propagation, type authority, and verifier rules form one coherent packet.

## Watchouts

- Preserve Step-7.13's exact i64 select → i32 Trunc → later i32 use chain and
  strict integer narrowing contract.
- Keep cttz, plus-one, zero-comparison, condition, and false-arm producers
  honest compatibility; never derive their IDs from rendered spelling.
- Exclude other builtins/calls, other select or cast producers, pointer/vector/
  complex/aggregate/object work, implicit coercions, CFG/parameters, inline
  assembly, and BIR from this completed packet.

## Proof

- Fresh `cmake --build --preset default` passed for the Step-7.13 wide ffs
  select-narrowing producer path and focused tests.
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure > test_after.log`
  passed 1/1 as the focused Step-7.13 proof.
- The matching narrow `test_before.log` / `test_after.log` monotonic guard
  passed with non-decreasing passed tests and no new failures.
- The supervisor's full regression guard passed 3033/3033 before and after,
  with delta 0 passed / 0 failed and no new failures; canonical proof is in
  `test_before.log` and `test_after.log`.
- `git diff --check` passed for the complete Step-7.13 slice.
