# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.3
Current Step Title: Publish representative scalar select result/use authority (complete)

## Just Finished

- Completed Plan Step 7.3 for the current i32 scalar builtin-ffs `LirSelectOp`
  result/use route.
- Allocated the select result through `fresh_value`, preserved exact i32 type
  authority and native zero-immediate authority, and returned the exact result
  operand into a later ordinary Add use.
- Kept the condition and false arm as honest monostate SSA compatibility
  because their internal producers lack authority; wider ffs narrowing also
  remains compatibility.
- Added focused exact-ID and misleading-display positives plus missing,
  invalid, duplicate, unknown/cross-function, type, and malformed-condition
  rejections. The matrix records this exact current-producer boundary.

## Suggested Next

- Select the next bounded Plan Step 7 ordinary producer row from the updated
  matrix; abs remains the immediate unclaimed scalar candidate.

## Watchouts

- Do not infer identity for the select's internal cttz call, plus-one binary, or
  zero comparison; their producers remain compatibility.
- Keep wider ffs narrowing and unavailable condition/false-arm authority
  unclaimed; do not reconstruct IDs from their displays.
- Abs may reuse the common allocator/operand mechanism but still requires its
  own producer/type contract and focused proof.
- Preserve closed Step-3 through Step-7.3 rows and idea-741 regression
  neighbors; keep pointer/object, aggregate/vector, CFG/parameters, calls,
  inline assembly, ABI, and BIR outside the next packet.

## Proof

- Fresh `cmake --build --preset default` passed for the Step-7.3 select
  producer, verifier, and focused tests.
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure > test_after.log`
  passed 1/1 as the focused Step-7.3 proof.
- The matching narrow `test_before.log` / `test_after.log` monotonic guard
  passed with non-decreasing passed tests and no new failures.
- The supervisor's clean-stashed full regression passed 3033/3033 both before
  and after this slice, with delta 0 passed / 0 failed and no new failures;
  canonical full-regression proof is in `test_before.log` and `test_after.log`.
- `git diff --check` passed for the complete Step-7.3 slice.
