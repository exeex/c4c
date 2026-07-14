# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.16
Current Step Title: Publish builtin-ffs cttz call-result/add-use authority

## Just Finished

- Completed Plan Step 7.15 for only the shared i32/i64 builtin-ffs
  equality-to-zero producer and its existing select condition use.
- Allocated the comparison result through `fresh_value`, retained integer mode,
  native Eq and exact i32/i64 type authority, published exact
  `LirIntegerImmediate{0}`, and preserved the result ID as the condition.
- Kept the prepared argument honest monostate SSA when unavailable and left the
  cttz result compatibility; no other builtin comparison/call/select family was
  claimed.
- Added exact-ID and misleading-display positives plus invalid/duplicate,
  unknown/cross-function, invalid/conflicting predicate/mode/type, wrong operand
  authority, and unrepresentable-immediate rejections. The matrix records the
  exact boundary.

## Suggested Next

- Execute Step 7.16: publish the builtin-ffs i32/i64 cttz call result as the
  exact lhs use of the accepted add-one `LirBinOp`.

## Watchouts

- Own only PI's i32/i64 `llvm.cttz` call inside `emit_builtin_ffs_call` and the
  exact use of its result as the accepted Step-7.14 add-one lhs.
- Allocate the call result through `fresh_value`; publish the intrinsic callee
  through module-owned `LinkNameId` authority and an exact nonvariadic
  structured signature with matching integer return and parameter type refs.
- Preserve the prepared value argument as honest monostate when unavailable
  and publish the structural `false` flag as an exact i1 integer immediate;
  never reconstruct either argument, callee, or result from call text.
- Require valid/unique current-function result ownership, resolvable native
  callee identity, exact return/signature/argument agreement, and the exact
  result-to-add lhs edge. Reject invalid/duplicate, unknown/cross-function,
  wrong-alternative, count, type, signature, or callee-authority conflicts.
- Accept misleading call/result/add displays only after native authority is
  proven. Preserve Steps 3, 7.14, and 7.15; exclude other intrinsic/builtin/
  direct/indirect calls, ABI or variadic work, pointer/vector/aggregate/object
  families, CFG/parameters, inline assembly, and BIR.

## Proof

- Fresh `cmake --build --preset default` passed for the Step-7.15 shared i32/i64
  ffs zero-comparison/select-condition path and focused tests.
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure > test_after.log`
  passed 1/1 as the focused Step-7.15 proof.
- The matching narrow `test_before.log` / `test_after.log` monotonic guard
  passed with non-decreasing passed tests and no new failures.
- The supervisor's matched full regression guard passed 3033/3033 before and
  after Step 7.15, with delta 0 passed / 0 failed and no new failures.
- `git diff --check` passed for the complete Step-7.15 slice.
