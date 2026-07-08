# RV64 Pointer Rhs Branch Stack-Source Consumer Migration

Status: Active
Source Idea: ideas/open/594_rv64_branch_stack_source_consumption_followup_from_593.md
Activated From:
- ideas/closed/593_rv64_branch_stack_source_freshness_consumption.md
- ideas/closed/596_pointer_rhs_branch_stack_source_policy_publication.md

## Purpose

Migrate the RV64 pointer `Rhs` branch stack-source consumer that idea 593 left
unwired, now that idea 596 has supplied the missing shared producer policy.

## Goal

Make the selected RV64 pointer `Rhs` branch stack-source consume path require
selected shared freshness authority for the exact branch use instead of any
target-local stack-home, frame-slot, clobber, register, operand-shape, or
testcase-shaped inference.

## Core Rule

RV64 pointer `Rhs` branch stack-source emission may proceed only when shared
prepared/prealloc authority selects
`PreparedValueFreshnessUseKind::BranchStackLoadSource` with
`PreparedValueFreshnessSourceKind::BranchStackSlot` for the same prepared
source value, exact branch block, and exact terminator instruction index.

## Read First

- `ideas/open/594_rv64_branch_stack_source_consumption_followup_from_593.md`
- `ideas/closed/593_rv64_branch_stack_source_freshness_consumption.md`
- `ideas/closed/596_pointer_rhs_branch_stack_source_policy_publication.md`
- Relevant RV64 branch emission and stack-source consumer code.
- Focused prepared/RV64 freshness tests and dumps touched by ideas 592, 593,
  and 596.

## Handoff Facts

- Idea 593 migrated the prepared RV64 object-emission fused pointer conditional
  branch path for a stack-slot `Lhs` operand.
- Idea 593 left pointer `Rhs` unwired because the producer/collector recorded
  it as inventory-only with `policy=none` / `status=missing_policy`.
- Idea 596 repaired that producer-side blocker. Valid pointer `Rhs` branch
  stack-load uses now publish selected `BranchStackLoadSource` /
  `BranchStackSlot` authority with
  `PreparedValueFreshnessProofKind::BranchTerminatorOrdering` and
  `PreparedValueFreshnessSourceRank::BranchStackSlot`.
- No remaining producer-side blocker is known for this 594 activation. If
  execution finds one, block and route it back to the producer family instead
  of adding an RV64 fallback.

## Current Targets

- The exact RV64 pointer `Rhs` branch stack-source consumer path implied by the
  idea 593 closure note and unblocked by idea 596.
- Adjacent fail-closed behavior for missing, ambiguous, stale, wrong-value,
  wrong-use, future-point, and stack-home-only authority.
- Diagnostics that keep source freshness failures distinct from missing stack
  home, missing layout, clobber failure, unsupported operand shape, and missing
  producer publication.

## Non-Goals

- Do not choose a consumer family that was not named or clearly implied by the
  idea 593 closure note.
- Do not define or repair typed/aggregate producer publication.
- Do not infer freshness in RV64 from stack homes, frame slots, aggregate
  lanes, clobber safety, register facts, local branch operand shape, or named
  testcase shape.
- Do not perform broad RV64, AArch64, x86, BIR, MIR, ABI, branch-lowering, or
  freshness-model rewrites.
- Do not change expectations, unsupported markers, allowlists, or runtime
  output as proof of capability progress.

## Working Model

The shared producer side is responsible for selecting branch stack-load source
freshness. The RV64 consumer should query that shared selected authority for
the exact branch point and prepared source value, then use layout and clobber
facts only as support facts after freshness has been proven.

## Execution Rules

- Keep each implementation packet narrow and semantic; do not add
  named-case-only matching.
- Preserve existing diagnostics where possible and add only focused
  distinctions needed to prove fail-closed behavior.
- Prefer focused prepared/RV64 dumps or tests that show the shared selected
  authority being consumed.
- Run the supervisor-delegated proof command exactly for executor packets.
- Keep routine progress, proof output, and blockers in `todo.md`.

## Step 1: Confirm Handoff And Locate Rhs Consumer

Goal: establish the exact `Rhs` consumer route before editing code.

Primary target: RV64 branch emission and stack-source consumer surfaces.

Actions:

- Re-read the 593 and 596 closure notes and cite the exact handoff in
  `todo.md`.
- Locate the migrated `Lhs` consumer path from idea 593 and the adjacent
  pointer `Rhs` path.
- Confirm, through code inspection or focused dumps, that valid pointer `Rhs`
  producer rows now expose selected shared authority rather than
  `policy=none` / `status=missing_policy`.
- Identify the narrow edit surface for requiring selected shared authority in
  the RV64 `Rhs` consumer.

Completion check:

- `todo.md` names the exact RV64 pointer `Rhs` consumer path, the shared
  authority query it should use, and any blocker. If the producer authority is
  missing, stop and report the producer-side blocker instead of continuing.

## Step 2: Migrate The Rhs Consumer

Goal: require selected shared freshness authority for RV64 pointer `Rhs`
branch stack-source emission.

Primary target: the narrow RV64 consumer route found in Step 1.

Actions:

- Mirror the semantic authority requirement used by the migrated `Lhs` route
  where appropriate, without coupling the implementation to operand names or a
  single testcase shape.
- Require the same prepared source value, exact branch block, exact terminator
  instruction index, `BranchStackLoadSource` use kind, and `BranchStackSlot`
  source kind.
- Keep layout, stack-home, and clobber checks as support checks, not
  freshness authority.
- Preserve separate failure paths for missing shared freshness versus ordinary
  layout, clobber, unsupported-shape, or missing-producer failures.

Completion check:

- The selected RV64 pointer `Rhs` route cannot emit from stack-source evidence
  unless the shared selected freshness authority matches the exact branch use.

## Step 3: Prove Accepted And Rejected Rhs Cases

Goal: add focused proof that the migrated route consumes shared authority and
fails closed for invalid authority.

Primary target: focused prepared/RV64 tests, dumps, or existing freshness test
fixtures.

Actions:

- Add or update proof for a valid pointer `Rhs` branch stack-source route that
  succeeds because shared selected authority is present.
- Prove rejection for missing, ambiguous, stale, wrong-value, wrong-use,
  future-point, and stack-home-only authority.
- Keep diagnostics or dump status clear enough to distinguish source
  freshness failure from layout, clobber, unsupported-shape, or producer
  publication failures.
- Avoid expectation downgrades, unsupported-marker edits, allowlist edits, or
  weaker contracts.

Completion check:

- Focused proof shows both the accepted `Rhs` consumer route and the required
  fail-closed invalid-authority cases.

## Step 4: Validate Freshness Queue Coverage

Goal: ensure the 587 through 596 freshness chain still holds after the `Rhs`
consumer migration.

Primary target: supervisor-selected focused backend/freshness subset, then any
broader checkpoint the supervisor requires.

Actions:

- Run the delegated build or backend proof command.
- Include existing focused freshness coverage from ideas 587 through 593 and
  the producer proof from idea 596 when routed by the supervisor.
- Escalate if the change touches shared behavior beyond the narrow RV64
  consumer.

Completion check:

- The selected proof is green, or `todo.md` records the exact failing command
  and blocker.

## Step 5: Closure Inventory And Follow-Up Decision

Goal: prepare the completion evidence needed before the source idea can close.

Primary target: `todo.md` closure notes for supervisor and later plan-owner
close evaluation.

Actions:

- Record which 593 closure-note gap was implemented.
- Record which RV64 consumer path was migrated.
- Inventory any remaining RV64 branch stack-source shapes and why they remain
  unwired.
- State whether any selected RV64 path still depends on stack-home-only,
  frame-slot-only, aggregate-lane-only, clobber-only, register-only, or
  operand-shape-only evidence as freshness.
- State whether another numbered RV64 consume-side follow-up is required.
- State whether RV64 branch stack-source consume behavior is stable enough for
  the 591 Prepared MIR view contract line to continue.

Completion check:

- `todo.md` contains enough closure inventory for a plan-owner close decision
  without rediscovering the 593 or 596 handoff.
