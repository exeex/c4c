Status: Active
Source Idea Path: ideas/open/241_prepared_callee_save_slot_placement.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Placement Authority

# Current Packet

## Just Finished

Activated `ideas/open/241_prepared_callee_save_slot_placement.md` as the
active lifecycle runbook.

## Suggested Next

Execute Step 1 from `plan.md`: inspect prepared callee-save register and frame
slot publication, then record the first implementation packet target and
focused proof subset before code changes.

## Watchouts

- Do not emit AArch64 callee-save stores or loads in this prerequisite.
- Do not infer slots in AArch64 target lowering from `save_index`,
  `frame_slot_order`, register names, or sorted offsets.
- Do not broaden into variadic save areas, outgoing call stack areas, general
  frame allocation redesign, or preserved-value extent work.
- Treat expectation weakening, named-case shortcuts, and target-local layout
  reconstruction as route drift.

## Proof

Lifecycle activation only. No build or test proof was required.
