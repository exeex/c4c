# RV64 Branch Stack-Source Freshness Consumption

Status: Closed
Type: Target-consumer migration after shared producer authority
Parent: `ideas/closed/592_typed_aggregate_branch_stack_source_publication.md`
Related:
- `ideas/closed/587_prepared_value_freshness_authority_mvp.md`
- `ideas/closed/588_shared_prealloc_move_operand_source_freshness_inventory.md`
- `ideas/closed/589_direct_edge_publication_move_freshness_ownership.md`
- `ideas/closed/590_branch_stack_load_freshness_contract.md`
- `ideas/closed/592_typed_aggregate_branch_stack_source_publication.md`
Owning Layer: RV64 MIR branch/stack-source consumers using shared prepared
freshness authority

## Goal

After idea 592 defines typed and aggregate branch stack-source producer
publication, migrate the RV64 branch stack-source consume path to use selected
shared freshness authority instead of target-local structural inference.

This idea should make RV64 consume branch stack-source freshness published by
shared prealloc, especially for pointer `Lhs` / `Rhs` and typed or
aggregate-adjacent branch stack-load operands that were intentionally left
blocked by idea 590.

## Why This Exists

Ideas 587 through 590 established the shared freshness vocabulary and migrated
representative shared-prealloc consumers. Idea 590 specifically defined the
branch stack-load rule: a stack home, frame slot, aggregate lane, or clobber
safety fact is not freshness by itself. A branch stack-loaded source is valid
only when shared authority proves the same value and use are fresh at the
branch terminator point.

Idea 592 is expected to add the missing producer side for typed and aggregate
branch stack-source publication. Once that producer contract exists, RV64
should stop relying on any local branch/stack-source freshness guess and should
consume the shared selected authority.

This must be completed before the separate Prepared MIR view line can safely
depend on RV64 branch stack-source behavior as a stable backend contract.

## Prerequisites

- Idea 592 is closed or otherwise has a completed implementation that publishes
  typed and aggregate `BranchStackSlot` freshness for
  `PreparedValueFreshnessUseKind::BranchStackLoadSource`.
- The selected freshness authority is visible to RV64 consumers through shared
  prepared/prealloc lookup APIs.
- Missing, ambiguous, stale, wrong-value, wrong-use, future-point, and
  stack-home-only producer cases from idea 592 fail closed before RV64
  migration begins.

## In Scope

- Audit RV64 branch emission and stack-source consume paths that currently
  depend on structural stack homes, frame slots, typed stack-source facts,
  aggregate stack-source facts, or branch stack-load rows.
- Identify the RV64 consumer families blocked by idea 590, including pointer
  `Lhs` / `Rhs` branch stack-load operands and any typed or aggregate-adjacent
  branch stack-source operands that 592 makes publishable.
- Replace the selected narrow RV64 consume path with a requirement for selected
  shared freshness authority matching:
  - the same prepared source value;
  - `PreparedValueFreshnessUseKind::BranchStackLoadSource`;
  - a branch-terminator-valid proof from the shared producer contract;
  - the exact branch block and terminator position being emitted.
- Preserve or add RV64 diagnostics that make missing source freshness distinct
  from missing stack home, missing layout, clobber failure, or unsupported
  operand shape.
- Add focused RV64 proof showing:
  - an accepted pointer or typed/aggregate branch stack-source route consumes
    selected shared freshness;
  - stack-home-only authority remains rejected;
  - stale, wrong-value, wrong-use, ambiguous, and future-point authority fail
    closed.
- Keep the 587, 588, 589, 590, and 592 freshness tests green.

## Out Of Scope

- Defining typed or aggregate branch stack-source producer publication; that is
  owned by idea 592.
- Any target-local RV64 rule that infers freshness from a stack home, frame
  slot, aggregate lane, clobber-safety fact, register allocation fact, or local
  branch operand shape.
- Broad AArch64 or x86 migration.
- Prepared MIR view design, old/new BIR equivalence, or MIR interface
  slimming; that is the separate 591 line.
- Redesigning control-flow lowering, branch instruction selection, ABI
  classification, register identity policy, or the freshness data model.
- Expectation downgrades, unsupported-marker edits, allowlist edits, or runtime
  output changes as proof of capability progress.

## Acceptance Criteria

- The RV64 branch stack-source consume path selected for this idea obtains
  freshness through shared prepared/prealloc authority, not target-local
  structural inference.
- At least one previously blocked RV64 branch stack-source case made possible
  by idea 592 becomes accepted only when selected `BranchStackSlot` freshness
  exists for the exact branch load use.
- Missing, ambiguous, stale, wrong-value, wrong-use, future-point, and
  stack-home-only authority remain rejected with visible diagnostics or dump
  status.
- The implementation distinguishes missing source freshness from ordinary RV64
  layout, clobber, or unsupported-shape failures.
- Focused tests or prepared/RV64 dumps prove the accepted route consumes the
  shared authority published by 592.
- Existing focused freshness coverage from ideas 587 through 592 continues to
  pass.
- The closure note inventories every remaining RV64 branch stack-source
  consumer family that is still unwired or intentionally deferred.
- A concrete follow-up idea is opened as `ideas/open/594_*.md` to take over the
  next remaining RV64 consume-side gap, unless the closure proves there is no
  remaining RV64 branch stack-source consume work.

## Completion Questions

The closure note must answer:

1. Which RV64 consumer path was migrated?
2. Which branch stack-source shapes remain unwired after this slice, and why?
3. Did any RV64 path still rely on stack-home-only, frame-slot-only,
   aggregate-lane-only, or clobber-only evidence as freshness?
4. Did the work expose a missing shared producer fact that should be sent back
   to the 592 family instead of patched in RV64?
5. Did the work expose any AArch64 or x86 follow-up, or are those still
   intentionally deferred?
6. Is the RV64 branch stack-source behavior stable enough for the later
   Prepared MIR view contract line to treat it as a required backend input?
7. What exact RV64 consume-side gap should 594 take over next?
8. Which `ideas/open/594_*.md` file was created, or why is no 594 needed?

## Closure Note

Closed after the active runbook migrated the prepared RV64 object-emission
fused pointer conditional branch path for a stack-slot `Lhs` operand. The
migrated route requires selected shared
`PreparedValueFreshnessUseKind::BranchStackLoadSource` / `BranchStackSlot`
authority for the same prepared source value, exact branch block, and
terminator instruction index before emitting the stack load into the RV64
`Lhs` scratch register and branching.

Remaining unwired branch stack-source shapes are deferred rather than hidden:
pointer `Rhs` remains unwired because the producer/collector records it as
inventory-only with `policy=none` / `status=missing_policy`;
aggregate-adjacent branch stack-source consumers and scalar-condition-register
branch shapes were not selected for the narrow 593 migration; string assembly
emission and other target emission paths remain outside this slice. The `Rhs`
gap is a producer/consumer handoff, not permission for RV64 to manufacture a
target-local fallback.

No selected RV64 path in this 593 slice may rely on stack-home-only,
frame-slot-only, aggregate-lane-only, clobber-only, register-only, or
operand-shape-only evidence as freshness. The migrated `Lhs` route fails
closed unless selected shared authority matches the exact branch use; layout
and clobber facts remain support facts, not freshness.

The work exposed a missing shared producer fact for pointer `Rhs`. That gap
must return to the 592 family before RV64 consumes it if the producer policy
is still missing. AArch64 and x86 remain intentionally deferred under this
RV64-only idea.

RV64 behavior is stable enough for the later Prepared MIR view line to require
selected shared branch stack-source authority for the migrated `Lhs` backend
input, but not for treating every RV64 branch stack-source shape as fully
wired until the `Rhs` producer/consumer handoff is resolved.

The exact follow-up is
`ideas/open/594_rv64_branch_stack_source_consumption_followup_from_593.md`.
It should take over the pointer `Rhs` branch stack-source consume-side gap,
starting by checking whether the missing `Rhs` producer/collector policy has
been supplied by the 592 family. If not, 594 should block on producer repair
instead of adding an RV64 fallback.

Validation accepted for closure: Step 4 backend proof passed 346/346 using
the supervisor-selected backend subset; regression guards passed; hook-managed
full-suite baselines after the code commits were accepted at 3375/3375. The
close-time lifecycle guard reused canonical `test_before.log` and
`test_after.log`, both 346/346, and passed the non-decreasing regression check
for this lifecycle-only close.

## Reviewer Reject Signals

- Reject any slice that makes RV64 branch stack-source emission succeed because
  a stack home, frame slot, aggregate lane, or clobber-safety fact exists
  without selected shared freshness authority for the exact branch use.
- Reject target-local freshness inference in RV64 branch emission, even if it
  matches the current testcase shape.
- Reject named-case-only matching for a specific branch operand instead of a
  semantic consumer rule tied to the shared freshness query.
- Reject helper renames, expectation rewrites, unsupported-marker edits,
  allowlist edits, or classification-only changes claimed as RV64 consumer
  migration.
- Reject broad RV64, AArch64, x86, BIR, or MIR rewrites that bypass the narrow
  RV64 consume migration.
- Reject a route that hides missing producer authority by manufacturing a
  fallback freshness fact in RV64.
- Reject retaining the idea-590 blocked pointer `Lhs` / `Rhs` failure mode
  behind a new abstraction name while claiming the RV64 consume side is wired.
- Reject closing this idea without either opening a concrete 594 follow-up for
  the next RV64 consume-side gap or explicitly proving that no such RV64
  consume-side gap remains.
