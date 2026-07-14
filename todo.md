# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.14
Current Step Title: Publish builtin-ffs plus-one result/select-use authority

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

- Execute Step 7.14: publish the builtin-ffs scalar plus-one `LirBinOp` result
  as the exact false-arm use of its existing `LirSelectOp`.

## Watchouts

- Own only PI's scalar integer add-one operation inside `emit_builtin_ffs_call`
  for the existing i32 and i64 routes. Allocate its `LirBinOp.result` through
  `fresh_value` and preserve that exact result ID as the existing select's
  false-arm operand.
- Retain native integer Add opcode/type authority and publish the structural
  constant one as `LirIntegerImmediate`; keep the cttz-produced lhs honest
  monostate until its call-result row is separately owned.
- Require unique current-function result ownership and exact select-arm use;
  reject invalid/duplicate results, unknown or cross-function uses, invalid or
  conflicting opcode/type authority, and malformed immediate alternatives.
- Accept misleading result/false-arm displays only after native authority is
  proven. Do not infer the cttz result or any select edge from rendered text.
- Preserve Steps 7.3 and 7.13. Exclude the cttz call result, zero comparison,
  select condition, other builtins/calls/binaries/selects, pointer/vector/
  aggregate/object work, CFG/parameters, inline assembly, and BIR.

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
