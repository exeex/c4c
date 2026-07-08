# RV64 Branch Stack-Source Consumption Followup From 593 Runbook

Status: Active
Source Idea: ideas/open/594_rv64_branch_stack_source_consumption_followup_from_593.md
Activated from: ideas/closed/593_rv64_branch_stack_source_freshness_consumption.md closure handoff

## Purpose

Continue the RV64 branch stack-source consume-side migration from idea 593 by
handling exactly the pointer `Rhs` handoff named in the 593 closure note.

## Goal

Migrate the pointer `Rhs` RV64 branch stack-source consume-side gap to selected
shared freshness authority only if the shared producer/collector policy has
been supplied; otherwise block on producer repair instead of adding an RV64
fallback.

## Core Rule

RV64 must not infer branch stack-source freshness from stack homes, frame
slots, aggregate lanes, clobber facts, register facts, operand shape, or
target-local heuristics. Accepted emission must consume selected shared
`PreparedValueFreshnessUseKind::BranchStackLoadSource` /
`PreparedValueFreshnessSourceKind::BranchStackSlot` authority for the exact
prepared value and branch use.

## Read First

- `ideas/open/594_rv64_branch_stack_source_consumption_followup_from_593.md`
- `ideas/closed/593_rv64_branch_stack_source_freshness_consumption.md`
- `ideas/closed/592_typed_aggregate_branch_stack_source_publication.md`
- Existing prepared freshness lookup and branch stack-load publication code
- Existing RV64 object-emission fused pointer branch path
- Focused prepared and RV64 tests covering pointer `Lhs` / `Rhs` branch
  stack-load freshness

## Current Targets

- Pointer `Rhs` branch stack-source consume-side gap named by the 593 closure
  note.
- Shared producer/collector records for the `Rhs` branch stack-load role.
- RV64 MIR object-emission consumer path for fused pointer conditional branch
  stack-load sources.
- Focused tests or dumps proving accepted selected authority and fail-closed
  rejected authority.

## Non-Goals

- Do not choose a consumer family not named or clearly implied by 593.
- Do not repair producer publication inside RV64.
- Do not add target-local freshness inference.
- Do not broaden into aggregate-adjacent branch sources, scalar condition
  register branches, AArch64, x86, Prepared MIR view design, old/new BIR
  equivalence, branch lowering redesign, ABI classification, or freshness model
  redesign.
- Do not edit unsupported markers, allowlists, expectation contracts, or runtime
  behavior as proof of progress.

## Working Model

Idea 593 migrated the RV64 fused pointer conditional branch `Lhs` stack-load
consumer. Its closure note left pointer `Rhs` unwired because the
producer/collector recorded `Rhs` as inventory-only with `policy=none` /
`status=missing_policy`. This runbook therefore starts by proving whether that
producer-side blocker still exists. If it does, the correct result for this
activation is a producer-repair blocker report, not an RV64 workaround.

## Execution Rules

- Cite the exact 593 closure-note handoff in executor progress.
- Keep `todo.md` as the packet-state scratchpad; do not edit the source idea
  unless lifecycle closure/deactivation needs durable notes.
- Preserve diagnostics that distinguish missing source freshness from missing
  stack home, missing layout, clobber failure, unsupported operand shape, and
  missing producer publication.
- Any accepted `Rhs` route must match the same prepared source value,
  `BranchStackLoadSource` use kind, `BranchStackSlot` producer proof, exact
  branch block, and terminator instruction index.
- Treat expectation rewrites, unsupported-marker edits, allowlist edits,
  helper renames, or classification-only changes as non-progress for this
  idea.

## Step 1: Confirm `Rhs` Producer Readiness

Goal: Determine whether the pointer `Rhs` producer/collector policy named by
593 is still missing or is ready for RV64 consumption.

Primary target:
- Shared prepared/prealloc branch stack-load producer and collector surfaces.

Actions:
- Read the 593 closure note and record the exact pointer `Rhs` handoff.
- Inspect the current producer/collector behavior for
  `PreparedBranchStackLoadRole::Rhs`.
- Run or update only focused prepared-side proof needed to distinguish
  `policy=none` / `status=missing_policy` from selected `BranchStackSlot`
  freshness authority.
- If `Rhs` remains producer-side inventory-only, stop implementation and report
  the blocker to the supervisor as a producer-repair prerequisite for the 592
  family.

Completion check:
- The executor can state whether `Rhs` has selected shared
  `BranchStackLoadSource` / `BranchStackSlot` authority available to consumers.
- If not available, no RV64 fallback or consume-side workaround was added.

## Step 2: Audit The `Rhs` RV64 Consumer Path

Goal: Locate the exact RV64 fused pointer conditional branch `Rhs`
stack-source consumer and its adjacent fail-closed paths.

Primary target:
- RV64 MIR object-emission branch stack-source consumer logic.

Actions:
- Trace the existing `Lhs` migrated path and the corresponding `Rhs` branch
  stack-source path.
- Identify where diagnostics currently distinguish missing source freshness
  from layout, clobber, unsupported-shape, or missing producer failures.
- Confirm the selected route can query shared freshness without changing
  branch lowering or ABI classification.

Completion check:
- The executor can name the narrow `Rhs` consume-side code path to migrate and
  list adjacent fail-closed cases that must remain rejected.

## Step 3: Migrate `Rhs` To Selected Shared Freshness

Goal: Require selected shared authority before RV64 emits the pointer `Rhs`
stack-load source for the branch.

Primary target:
- The narrow RV64 object-emission consumer path identified in Step 2.

Actions:
- Reuse or generalize the existing selected `Lhs` freshness query only as a
  semantic branch-stack-load consumer rule, not as named-case matching.
- Require same-value, same-use, same-source-kind, exact branch block, and exact
  terminator position.
- Preserve or add fail-closed status for missing, ambiguous, stale,
  wrong-value, wrong-use, future-point, stack-home-only, and missing-producer
  authority.
- Keep support facts such as layout and clobber separate from freshness
  authority.

Completion check:
- The `Rhs` branch stack-source route emits only when selected shared
  `BranchStackLoadSource` / `BranchStackSlot` authority matches the exact
  branch use.

## Step 4: Prove Focused Behavior

Goal: Prove the migrated `Rhs` route consumes shared authority and rejects
nearby invalid authority.

Primary target:
- Focused prepared/RV64 tests or dumps covering pointer `Rhs` branch
  stack-source freshness.

Actions:
- Add or update focused proof for accepted `Rhs` selected authority.
- Prove stack-home-only, missing, ambiguous, stale, wrong-value, wrong-use, and
  future-point authority fail closed.
- Re-run the supervisor-delegated focused backend subset exactly.
- Keep existing focused freshness coverage from ideas 587 through 593 green.

Completion check:
- Fresh proof demonstrates the selected `Rhs` consumer path uses shared
  authority and does not regress the existing freshness chain.

## Step 5: Prepare Closure Inventory

Goal: Leave a durable completion inventory for lifecycle closure or follow-up
selection.

Primary target:
- `todo.md` progress notes first; source-idea closure note only during a
  lifecycle close operation.

Actions:
- Record which 593 closure-note gap was implemented or why it blocked.
- Record the migrated RV64 consumer path, if migration proceeded.
- Record remaining unwired RV64 branch stack-source shapes and whether another
  numbered follow-up is required.
- State whether any selected RV64 path still relies on stack-home-only,
  frame-slot-only, aggregate-lane-only, clobber-only, register-only, or
  operand-shape-only evidence as freshness.
- State whether the RV64 branch stack-source consume side is now stable enough
  for the 591 Prepared MIR view line.

Completion check:
- The supervisor has enough packet state to either close 594, block it on
  producer repair, or open the next numbered follow-up without rediscovering
  the handoff.
