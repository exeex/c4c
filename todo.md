Status: Active
Source Idea Path: ideas/open/91_advanced_prepared_call_authority_and_grouped_width_allocation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Remaining Advanced Prepared-Authority Gaps
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Step 1 audit identified the first shared consumer gap inside idea 91: focused
prepared-value dumps were keeping unrelated call-plan and storage-plan
authority live even when the selected value only owned one call-result seam.
Repaired `src/backend/backend.cpp` so focused prepared dumps retain only the
matching call/result-preservation and storage publication, and tightened
`backend_cli_dump_prepared_bir_focus_value_t1_00204` to prove the filtered
surface no longer leaks unrelated callsites or storage values.

## Suggested Next

Advance to a Step 2 packet that exercises a richer advanced-call publication
family through the same shared consumer surface, preferably memory-return or
indirect-call prepared dump proof, so the next slice proves more than the
baseline variadic call-result path.

## Watchouts

- Keep this route target-independent; do not hide missing authority behind
  x86-local recovery.
- Do not reopen out-of-SSA follow-on work from idea 90 inside this runbook.
- Reject testcase-shaped shortcuts; the next packet must generalize across the
  owned advanced call or grouped-width failure family.
- Focused prepared dumps still print unrelated frame-plan sections for other
  functions after function/value filtering; treat that as adjacent shared-dump
  cleanup, not as a reason to weaken the new call/storage filtering proof.

## Proof

`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_'`
Log: `test_after.log`
