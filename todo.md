Status: Active
Source Idea Path: ideas/open/91_advanced_prepared_call_authority_and_grouped_width_allocation.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Consumer Use And Decide Closure
Plan Review Counter: 2 / 6
# Current Packet

## Just Finished

Step 4 added direct backend-consumer proof for the remaining advanced
prepared-call seams: same-module variadic-byval now proves x86 reads the
published same-module call and byval argument authority directly from consumed
plans, indirect-callee publication now proves x86 reads the indirect callee,
preserved carry, and call result directly from consumed plans, and
memory-return publication now proves x86 reads the explicit sret call-plan
authority and matching storage-plan visibility through the consumer surface.
Lifecycle review then judged idea 91 complete on source scope, but close was
rejected for now because the required monotonic regression guard did not pass.

## Suggested Next

Keep Step 4 active until the supervisor resolves the close gate honestly:
either produce a close-scope regression comparison that passes
`c4c-regression-guard`, or leave idea 91 open while classifying that guard
failure as the only remaining blocker rather than inventing new technical
scope.

## Watchouts

- This packet stayed test-only; no implementation source changes were needed
  to make the remaining advanced prepared-call seams visible through direct
  backend consumers.
- `tests/backend/backend_x86_route_debug_test.cpp` remains publication/debug
  coverage rather than the primary Step 4 consumer-use proof surface; the new
  direct-consumer assertions live in the call-contract and liveness tests.
- Keep idea 90 out-of-SSA follow-on work and any wider pool-policy redesign
  outside this closure decision unless the supervisor decides a separate idea
  is still required.
- The close blocker is workflow proof, not leftover idea-91 technical scope:
  `check_monotonic_regression.py` rejected the existing backend before/after
  pair because pass count stayed flat at 97 to 97.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
Result: passed after adding direct x86 consumer-surface assertions for the
remaining advanced prepared-call seams.
Log: `test_after.log`

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`
Result: failed because the backend CTest pass count did not strictly increase
(`97 -> 97`), so close could not be accepted.
