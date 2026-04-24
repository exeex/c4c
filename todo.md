Status: Active
Source Idea Path: ideas/open/91_advanced_prepared_call_authority_and_grouped_width_allocation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish Advanced Prepared Call Authority
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Step 2 now proves a richer advanced-call family through the same shared
consumer surface. Added a focused prepared-dump route-debug fixture in
`tests/backend/backend_x86_route_debug_test.cpp` for a memory-return call and
verified that focusing on `lv.call.sret.storage` keeps the published
memory-return callsite and storage authority while dropping the unrelated
immediate argument detail from the filtered prepared dump.

## Suggested Next

Continue Step 2 with another advanced prepared-call family that shares this
consumer boundary, preferably indirect-call focus proof or before/after-call
move publication for a harder boundary case, so the route proves more than the
memory-return seam before moving on to grouped width `> 1` allocator work.

## Watchouts

- Keep this route target-independent; do not hide missing authority behind
  x86-local recovery.
- Do not reopen out-of-SSA follow-on work from idea 90 inside this runbook.
- Reject testcase-shaped shortcuts; the next packet must generalize across the
  owned advanced call or grouped-width failure family.
- Focused prepared dumps still leave unrelated non-value-owned sections such as
  broader frame-plan material in place; treat that as adjacent shared-dump
  cleanup, not as a reason to weaken the new advanced-call filtering proof.

## Proof

`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_'`
Log: `test_after.log`
