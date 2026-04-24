Status: Active
Source Idea Path: ideas/open/91_advanced_prepared_call_authority_and_grouped_width_allocation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish Advanced Prepared Call Authority
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Step 2 now proves two richer advanced-call families through the same shared
consumer surface. Added a built indirect-call publication proof in
`tests/backend/backend_prepare_liveness_test.cpp` and verified through the
shared prepared call-plan plus prepared-printer boundary that an indirect
callee value publishes register-backed indirect authority while the cross-call
`carry` value publishes callee-saved preservation authority, alongside the
earlier memory-return focus proof.

## Suggested Next

Decide whether Step 2 needs one more publication packet for a stack-backed
before/after-call preservation edge or whether the route is now broad enough to
start the grouped width `> 1` allocator work in Step 3.

## Watchouts

- Keep this route target-independent; do not hide missing authority behind
  x86-local recovery.
- Do not reopen out-of-SSA follow-on work from idea 90 inside this runbook.
- Reject testcase-shaped shortcuts; the next packet must generalize across the
  owned advanced call or grouped-width failure family.
- Focused prepared dumps still leave unrelated non-value-owned sections such as
  broader frame-plan material in place; treat that as adjacent shared-dump
  cleanup, not as a reason to weaken the new advanced-call filtering proof.
- The new indirect-call proof stays on the built prepared-printer/call-plan
  surface and intentionally checks generic authority fields without pinning a
  target-local register name.

## Proof

`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_'`
Log: `test_after.log`
