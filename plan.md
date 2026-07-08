# Pointer Rhs Branch Stack-Source Policy Publication Runbook

Status: Active
Source Idea: ideas/open/596_pointer_rhs_branch_stack_source_policy_publication.md

## Purpose

Repair the shared prepared/prealloc producer policy so pointer `Rhs` branch
stack-load sources can publish selected freshness authority for exact branch
uses.

## Goal

Make valid pointer `Rhs` branch stack-load uses produce selected
`PreparedValueFreshnessUseKind::BranchStackLoadSource` /
`PreparedValueFreshnessSourceKind::BranchStackSlot` authority without adding
any target-local fallback.

## Core Rule

Fix producer authority only. Do not make RV64, AArch64, x86, string assembly,
or any other target consumer infer freshness from stack homes, frame slots,
aggregate lanes, clobber facts, register facts, operand shape, or testcase
shape.

## Read First

- `ideas/open/596_pointer_rhs_branch_stack_source_policy_publication.md`
- `ideas/closed/592_typed_aggregate_branch_stack_source_publication.md`
- `ideas/closed/593_rv64_branch_stack_source_freshness_consumption.md`
- `ideas/open/594_rv64_branch_stack_source_consumption_followup_from_593.md`
- Current prepared/prealloc branch stack-load producer and collector code
- Existing focused freshness or branch stack-load proof tests and dumps

## Current Targets

- Shared prepared/prealloc branch stack-load source collection and policy
  selection.
- The policy gate that currently leaves `PreparedBranchStackLoadRole::Rhs`
  with `policy=none`, `pointer_status=unknown`, and
  `status=missing_policy`.
- Focused producer-side proof for accepted `Rhs` publication and invalid-route
  rejection.

## Non-Goals

- Do not change RV64, AArch64, x86, string assembly, or other target-local
  consumers.
- Do not redesign branch lowering, ABI classification, BIR, MIR view, or the
  freshness model.
- Do not change runtime behavior through expectation downgrades, unsupported
  markers, allowlists, or weaker tests.
- Do not close or reactivate idea 594 inside this plan; only leave a clear
  handoff note when producer authority is ready.

## Working Model

- Pointer `Lhs` already has a selected branch stack-load producer path from the
  592/593 chain.
- Pointer `Rhs` is collected as producer inventory but lacks selection policy.
- The correct repair is a semantic producer policy rule that accepts only the
  same prepared source value, same branch stack-load use, exact branch block,
  and terminator instruction position.
- Diagnostics must keep missing policy distinct from missing stack home,
  ambiguous authority, stale authority, wrong value, wrong use, future point,
  and stack-home-only evidence.

## Execution Rules

- Prefer shared producer-policy changes over target-specific repairs.
- Keep the `Rhs` route symmetric with existing semantic branch stack-load
  authority where the architecture allows it, but do not clone code blindly.
- Preserve fail-closed behavior for invalid authority.
- Add focused proof before treating the producer repair as complete.
- Escalate to supervisor/reviewer if the only passing route depends on a named
  testcase shortcut or weaker expectation contract.

## Steps

### Step 1: Locate The `Rhs` Producer Policy Gap

Goal: identify the exact shared producer/collector path that records pointer
`Rhs` inventory but refuses selected branch stack-load authority.

Primary Target: prepared/prealloc branch stack-load source collection,
diagnostic status, and policy selection code.

Actions:

- Trace how `PreparedBranchStackLoadRole::Rhs` reaches the producer inventory.
- Compare the accepted pointer `Lhs` path with the current `Rhs` path.
- Locate the policy gate that emits `policy=none`, `pointer_status=unknown`,
  or `status=missing_policy` for valid `Rhs` branch stack-load uses.
- Identify the focused existing test or dump command that exposes the missing
  policy.

Completion Check:

- The executor can name the owning function or helper that must change.
- The proof surface for the missing `Rhs` producer authority is identified.
- No target-local consumer change is needed for the first repair.

### Step 2: Add Semantic `Rhs` Policy Selection

Goal: extend the shared producer policy so valid pointer `Rhs` branch
stack-load uses publish selected freshness authority.

Primary Target: the policy helper or decision table found in Step 1.

Actions:

- Add `Rhs` policy selection only when same-value, same-use, exact-branch, and
  terminator-position conditions are satisfied.
- Publish selected authority with
  `PreparedValueFreshnessUseKind::BranchStackLoadSource` and
  `PreparedValueFreshnessSourceKind::BranchStackSlot`.
- Keep invalid routes fail-closed for missing, ambiguous, stale, wrong-value,
  wrong-use, future-point, and stack-home-only cases.
- Preserve diagnostic status names that distinguish missing policy from other
  rejection reasons.

Completion Check:

- Valid pointer `Rhs` no longer reports `policy=none` /
  `status=missing_policy`.
- Invalid `Rhs` candidates remain rejected for the right reason.
- No target consumer, expectation downgrade, unsupported marker, or allowlist
  change is part of the slice.

### Step 3: Prove Accepted And Rejected Producer Routes

Goal: add or update focused prepared-side proof for the repaired producer
policy.

Primary Target: existing freshness, branch stack-load, prepared dump, or
focused backend tests that observe producer authority.

Actions:

- Add proof that a valid pointer `Rhs` branch stack-load use publishes selected
  `BranchStackLoadSource` / `BranchStackSlot` authority.
- Add or preserve proof that missing, ambiguous, stale, wrong-value,
  wrong-use, future-point, and stack-home-only cases fail closed.
- Run the supervisor-delegated build and proof command exactly as assigned.
- Record proof results in `todo.md` and leave canonical logs in the expected
  filenames only when delegated.

Completion Check:

- Focused proof demonstrates accepted `Rhs` producer publication.
- Focused proof demonstrates invalid-route rejection remains intact.
- Existing focused coverage from the 592/593 producer chain still passes under
  the delegated command.

### Step 4: Record The 594 Handoff

Goal: make the lifecycle handoff clear for the blocked RV64 consumer follow-up.

Primary Target: `todo.md` packet notes during execution, and the eventual
closure note when the supervisor closes the idea.

Actions:

- State whether idea 594 can be reactivated for RV64 consumer migration.
- If producer authority still has a blocker, name the remaining missing
  producer fact and the proof that shows it.
- Do not edit idea 594 or reactivate it from this plan.

Completion Check:

- The handoff says either that selected `Rhs` producer authority exists for
  idea 594, or exactly why it is still blocked.
- The source idea completion questions can be answered without reconstructing
  the route from chat history.
