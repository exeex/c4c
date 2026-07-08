# RV64 Branch Stack-Source Consumption Followup From 593

Status: Open
Type: Conditional follow-up implementation idea
Parent: `ideas/closed/593_rv64_branch_stack_source_freshness_consumption.md`
Related:
- `ideas/closed/592_typed_aggregate_branch_stack_source_publication.md`
- `ideas/closed/593_rv64_branch_stack_source_freshness_consumption.md`
- `ideas/open/596_pointer_rhs_branch_stack_source_policy_publication.md`
- `ideas/closed/587_prepared_value_freshness_authority_mvp.md`
- `ideas/closed/588_shared_prealloc_move_operand_source_freshness_inventory.md`
- `ideas/closed/589_direct_edge_publication_move_freshness_ownership.md`
- `ideas/closed/590_branch_stack_load_freshness_contract.md`
Owning Layer: RV64 MIR branch/stack-source consumers remaining after 593

## Goal

Read the closure note from idea 593, identify the next remaining RV64
branch stack-source consume-side gap, and migrate that exact gap to selected
shared freshness authority.

This idea is intentionally a handoff slot. It should not guess the remaining
consumer family before 593 closes. Its first execution step is to use the 593
closure inventory as the source of truth.

## Lifecycle Blocker

The active 594 runbook was deactivated after Step 1 confirmed that the 593
closure-note blocker still exists: pointer `Rhs` is still collected as
producer-side inventory only, with `policy=none`,
`pointer_status=unknown`, and `status=missing_policy`. No selected
`PreparedValueFreshnessUseKind::BranchStackLoadSource` /
`PreparedValueFreshnessSourceKind::BranchStackSlot` authority is available for
RV64 to consume.

Do not reactivate 594 or proceed to RV64 consumer migration until the producer
side supplies selected `Rhs` branch stack-load authority for the same prepared
source value, exact branch block, and terminator instruction index. The
producer-side repair is tracked separately in
`ideas/open/596_pointer_rhs_branch_stack_source_policy_publication.md`.

## Why This Exists

Idea 593 is expected to migrate one narrow RV64 branch stack-source consumer
after idea 592 provides typed and aggregate branch stack-source publication.
That first RV64 consume-side migration may still leave nearby branch
stack-source shapes unwired, such as the opposite pointer side, an
aggregate-adjacent source, or a target emission path that was too broad for
593.

The remaining gap should not be reconstructed from memory or rediscovered from
scratch. 593 must close with an explicit inventory and name the next RV64
consume-side gap. This idea exists to consume that handoff and continue the
same architecture rule without widening scope accidentally.

## Required Input From 593

Before implementation starts, read the closed 593 source idea and its closure
note. The runbook derived from this idea must record:

- which RV64 consumer path 593 migrated;
- which RV64 branch stack-source consumer families 593 left unwired;
- which exact gap 593 recommended for 594;
- whether any missing producer fact must be sent back to the 592 family before
  this idea can proceed;
- whether 593 judged the remaining gap ready for implementation or still
  blocked on contract design.

If 593 did not provide this information, this idea should block and request a
593 closure repair instead of guessing a route.

## In Scope

- Select exactly one RV64 branch stack-source consume-side gap named by the
  593 closure note.
- Audit the selected RV64 consumer path and its adjacent fail-closed cases.
- Require selected shared freshness authority matching:
  - the same prepared source value;
  - `PreparedValueFreshnessUseKind::BranchStackLoadSource`;
  - the producer proof kind accepted by ideas 592 and 593;
  - the exact branch block and terminator position being emitted.
- Preserve diagnostics that distinguish missing source freshness from missing
  stack home, missing layout, clobber failure, unsupported operand shape, or
  missing producer publication.
- Add focused proof for the selected remaining RV64 consume-side gap.
- Update the closure note with any further remaining RV64 branch stack-source
  consume families and whether another numbered follow-up is needed.

## Out Of Scope

- Choosing a gap not named or implied by the 593 closure inventory.
- Defining or repairing typed/aggregate producer publication; send that back
  to the 592 family if needed.
- Target-local RV64 freshness inference from stack homes, frame slots,
  aggregate lanes, clobber safety, register allocation facts, or local branch
  operand shape.
- Broad AArch64 or x86 migration.
- Prepared MIR view design or old/new BIR view equivalence; that remains the
  separate 591 line.
- Redesigning branch lowering, ABI classification, register identity policy, or
  the freshness data model.
- Expectation downgrades, unsupported-marker edits, allowlist edits, or runtime
  output changes as proof of capability progress.

## Acceptance Criteria

- The runbook identifies the exact 593 closure note finding that this idea is
  implementing.
- One remaining RV64 branch stack-source consume-side gap named by 593 is
  migrated to selected shared freshness authority.
- The migrated route fails closed for missing, ambiguous, stale, wrong-value,
  wrong-use, future-point, and stack-home-only authority.
- The implementation keeps source freshness failures distinct from ordinary
  RV64 layout, clobber, unsupported-shape, or missing-producer failures.
- Focused tests or prepared/RV64 dumps prove the accepted route consumes the
  shared authority published by the 592/593 chain.
- Existing focused freshness coverage from ideas 587 through 593 continues to
  pass.
- The closure note states whether RV64 branch stack-source consume work is now
  complete enough for the 591 Prepared MIR view line, or opens the next
  numbered follow-up for the remaining RV64 consume-side gap.

## Completion Questions

The closure note must answer:

1. Which specific 593 closure-note gap did this idea implement?
2. Which RV64 consumer path was migrated?
3. Which RV64 branch stack-source shapes remain unwired after this slice, and
   why?
4. Did any selected RV64 path still rely on stack-home-only, frame-slot-only,
   aggregate-lane-only, clobber-only, or operand-shape-only evidence as
   freshness?
5. Did the work expose a missing shared producer fact that must return to the
   592 family?
6. Is another numbered RV64 consume-side follow-up required? If yes, which
   `ideas/open/595_*.md` file was created?
7. Is the RV64 branch stack-source consume side now stable enough for the 591
   Prepared MIR view contract line to continue?

## Reviewer Reject Signals

- Reject starting implementation without reading and citing the 593 closure
  note.
- Reject choosing a consumer family that was not named or clearly implied by
  the 593 closure inventory.
- Reject any slice that makes RV64 branch stack-source emission succeed because
  a stack home, frame slot, aggregate lane, clobber-safety fact, register fact,
  or operand shape exists without selected shared freshness authority for the
  exact branch use.
- Reject target-local freshness inference in RV64 branch emission.
- Reject named-case-only matching for a specific branch operand instead of a
  semantic consumer rule tied to the shared freshness query.
- Reject helper renames, expectation rewrites, unsupported-marker edits,
  allowlist edits, or classification-only changes claimed as RV64 consumer
  migration.
- Reject broad RV64, AArch64, x86, BIR, or MIR rewrites that bypass this narrow
  593 handoff.
- Reject closing this idea without either proving the RV64 branch stack-source
  consume side is complete enough for 591 or opening the next numbered
  follow-up for the remaining RV64 consume-side gap.
