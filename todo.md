Status: Active
Source Idea Path: ideas/open/508_rv64_prepared_callee_saved_gpr_save_slot_emission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Rebuild Save-Slot Evidence

# Current Packet

## Just Finished

Lifecycle activation created the active runbook and execution-state skeleton
for Step 1 of `plan.md`.

## Suggested Next

Execute Step 1: rebuild the save-slot evidence for `src/20000603-1.c`, confirm
the prepared saved-register facts, and identify the current RV64 object-route
rejection point.

## Watchouts

- Keep the route limited to RV64 object emission consuming explicit prepared
  callee-saved GPR save-slot facts.
- Do not infer save slots from testcase names, raw slot numbers, source shape,
  or register spellings.
- Do not include FPR, F128, vector, dynamic-stack, or broad fixed-frame support
  in this plan.

## Proof

- Pending executor proof for Step 1.
