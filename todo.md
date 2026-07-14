# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.15
Current Step Title: Publish builtin-ffs zero-comparison/select-condition authority (complete)

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

- Select the next bounded Step-7 ordinary-value identity row from the source
  idea and establish its exact producer/use boundary before implementation.

## Watchouts

- Preserve the Step-7.15 contract: only PI's shared i32/i64 equality-to-zero
  comparison is newly authoritative, with exact fresh result, integer Eq/type,
  immediate zero, and exact select-condition identity.
- Keep the prepared argument honest monostate when unavailable and never infer
  the argument result or select edge from rendered text.
- Preserve Steps 7.2, 7.3, 7.13, and 7.14. Continue excluding the cttz result
  and other builtin comparisons/calls/selects, pointer/vector/aggregate/object
  work, CFG/parameters, inline assembly, and BIR.

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
