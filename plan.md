# AArch64 Hanoi Starting-State Refresh Runbook

Status: Active
Source Idea: ideas/open/360_aarch64_hanoi_starting_state_output_mismatch.md
Activated from: parked source idea with no active plan present

## Purpose

Refresh and, only if still current, resolve the AArch64 Hanoi starting-state
output mismatch owner recorded in the source idea.

## Goal

Determine whether the current tree still loses one initialized Hanoi source
tower element before recursive post-call pointer-formal behavior is relevant,
then either close the idea or hand off a freshly localized residual.

## Core Rule

Do not implement under this idea unless fresh generated-code evidence shows a
general starting-state value-flow failure before the recursive `%p.spare`
reuse point.

## Read First

- `ideas/open/360_aarch64_hanoi_starting_state_output_mismatch.md`
- The parked outcome noting commit `87e79a50a` repaired the direct
  `LoadGlobal` current-memory select-store owner.
- Adjacent split and guardrail ideas named by the source idea: materialized
  pointer `StoreLocal` writeback, stack-preserved pointer formal post-call
  handling, scalar formal post-call reloads, pointer-formal callee-saved home
  publication, and address-valued memory/call-argument publication.
- Current semantic BIR, prepared BIR, and generated AArch64 artifacts for
  `c_testsuite_aarch64_backend_src_00181_c` only as representative evidence,
  not as a testcase-specific repair target.

## Current Scope

- AArch64 value flow for the initial Tower of Hanoi source-tower state before
  the first recursive post-call `%p.spare` reuse point.
- Source tower initialization, stores, reloads, and first printed state for
  `00181` as external evidence.
- Focused backend coverage for the localized value-flow shape if the owner is
  live again.

## Non-Goals

- Do not reopen materialized pointer-addressed `StoreLocal` writeback from
  idea 361 unless fresh first-bad-fact evidence proves the starting-state
  owner moved there and lifecycle routing is updated.
- Do not reopen stack-preserved pointer formal post-call handling, scalar
  formal post-call reloads, pointer-formal callee-saved home publication,
  address-valued memory publication, semantic string loads, frontend
  admission, ABI composite work, variadic/floating work, or dynamic stack work
  without a separate lifecycle split.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, CTest registration, or proof-log policy.
- Do not special-case `00181`, `Hanoi`, the literal value `3`, one tower name,
  one source line, one stack offset, one ABI register, or one emitted
  instruction neighborhood.

## Working Model

The historical failure was an incorrect initial printed Hanoi state where
`A: 1 2 0 4` appeared instead of `A: 1 2 3 4`. The source idea says that
owner was repaired by reloading select-store globals from current memory, and
the later unchanged-moves residual was split to materialized pointer store
writeback. This runbook therefore starts with a refresh and only proceeds to
implementation if the starting-state value-flow owner is live again.

## Execution Rules

- Treat `ideas/open/360_aarch64_hanoi_starting_state_output_mismatch.md` as
  the durable scope contract.
- Prefer read-only generated artifact inspection before code edits.
- If the starting state is correct and focused guardrails remain green,
  request lifecycle closure instead of implementation work.
- If the representative fails for an out-of-scope owner, record the
  classification in `todo.md` and return for lifecycle routing rather than
  widening this plan.
- Any repair must add or preserve focused coverage for the general value-flow
  shape and keep ideas 357, 358, 359, and adjacent memory guardrails stable.
- For any code-changing step, require build proof plus the
  supervisor-selected focused CTest subset before acceptance.

## Step 1: Refresh Current First Bad Fact

Goal: determine whether the Hanoi starting-state value-flow owner is still a
live first bad fact.

Primary target: the supervisor-selected focused representative for this idea,
normally `c_testsuite_aarch64_backend_src_00181_c` plus existing backend
contracts for the select-store/current-memory and adjacent pointer-formal
guardrails.

Actions:

- Rebuild the current tree before classification.
- Run the focused representative and nearby backend coverage selected by the
  supervisor.
- If anything fails, inspect current semantic BIR, prepared BIR, and generated
  AArch64 artifacts around the initial tower initialization, store, reload,
  and first printed state.
- Decide whether any observed failure still shows an in-scope starting-state
  value-flow loss before recursive post-call pointer-formal behavior.

Completion check:

- `todo.md` records either a current starting-state bad fact with concrete
  generated-code evidence, or a current out-of-scope / already-green
  classification for supervisor lifecycle routing.

## Step 2: Localize The Value-Flow Boundary

Goal: if Step 1 finds an in-scope failure, identify exactly where the initial
source-tower value stops reaching the printed starting state.

Primary target: source tower initialization, selected store publication,
current-memory reload, and first print consumer.

Actions:

- Compare semantic BIR, prepared BIR, selected records, and emitted assembly
  for the first mismatched tower element.
- Identify whether the gap is in source value materialization, selected-store
  current-memory selection, store emission, reload addressing, or print
  argument publication.
- Record the smallest focused backend shape that should fail without the
  repair.

Completion check:

- The failing boundary is named in terms of source value, expected memory
  update, observed stale or missing carrier, emitted instruction behavior, and
  first consumer.

## Step 3: Repair General Starting-State Publication

Goal: repair the localized value-flow rule without testcase-shaped matching.

Actions:

- Implement the smallest general lowering or publication change needed for
  the localized owner.
- Add or update focused backend coverage for the starting-state value-flow
  shape independent of `00181`.
- Keep pointer-formal, scalar formal reload, address-valued memory, and
  materialized pointer store behavior stable.

Completion check:

- Focused backend coverage proves the initialized tower element reaches the
  first printed state through the repaired general rule.
- The implementation does not mention `00181`, `Hanoi`, one literal, one tower,
  one register, or one emitted instruction sequence as a control path.

## Step 4: Prove And Reclassify

Goal: prove the repaired or refreshed owner and hand off any remaining first
bad fact.

Actions:

- Run the exact focused proof command delegated by the supervisor.
- Include `00181` representative proof if it remains the external witness.
- If the representative still fails, classify the new first bad fact and keep
  it separate from this starting-state source scope.

Completion check:

- `todo.md` records the proof command and result.
- The source idea is either ready for close consideration, or the next first
  bad fact is clearly classified for lifecycle handoff.
