Status: Active
Source Idea Path: ideas/open/594_rv64_branch_stack_source_consumption_followup_from_593.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm `Rhs` Producer Readiness

# Current Packet

## Just Finished

Lifecycle activation created the runbook for Step 1 from idea 594 and the
closed 593 pointer `Rhs` handoff.

## Suggested Next

Delegate Step 1 to an executor: confirm whether pointer `Rhs` now has selected
shared `BranchStackLoadSource` / `BranchStackSlot` producer authority, and stop
with a producer-repair blocker if it remains `policy=none` /
`status=missing_policy`.

## Watchouts

- Do not add an RV64 fallback for missing pointer `Rhs` producer authority.
- Do not infer freshness from stack homes, frame slots, aggregate lanes,
  clobber facts, register facts, or operand shape.
- Keep `Rhs` work narrow to the 593 closure-note handoff.

## Proof

Lifecycle-only activation. No build or test proof required by the plan owner.
