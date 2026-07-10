# Stack-Carried Pointer Source Publication Materialization Runbook

Status: Active
Source Idea: ideas/open/653_stack_carried_pointer_source_publication_materialization.md

## Purpose

Repair the remaining stack-carried pointer source publication or
materialization boundary exposed after RV64 fused pointer branch terminator
lowering advanced.

## Goal

Make the `%t6` path in `src/20140828-1.c` and the `%t23` path in
`src/loop-2e.c` either consume explicitly fresh, materialized stack-carried
pointer sources or fail closed with a precise producer-authority diagnostic.

## Core Rule

Pointer source freshness and materialization must come from explicit prepared
facts at the consumer point. Do not infer them from stack offsets, final
assembly shape, source spelling, local names, diagnostics, testcase identity,
runtime outcomes, expectation changes, unsupported markers, or pass/fail
accounting.

## Read First

- `ideas/open/653_stack_carried_pointer_source_publication_materialization.md`
- `ideas/closed/645_rv64_branch_residual_terminator_fragment_lowering.md`
- `ideas/closed/636_prepared_branch_stack_source_freshness_publication.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`

## Current Scope

- Representative rows: `tests/c/external/gcc_torture/src/20140828-1.c` and
  `tests/c/external/gcc_torture/src/loop-2e.c`.
- Target values: `%t6` in `src/20140828-1.c` and `%t23` in `src/loop-2e.c`.
- Owned boundary: prepared/RV64 stack-carried pointer source publication and
  materialization after terminator admission is no longer the first owner.

## Non-Goals

- Do not reopen RV64 terminator-fragment admission closed by idea 645.
- Do not republish generic branch stack-source freshness already owned by prior
  branch freshness ideas unless refreshed evidence proves a distinct producer
  gap.
- Do not take direct-global stack-backed pointer branch operands from idea 654.
- Do not change ABI policy, runtime policy, expectations, unsupported markers,
  allowlists, timeouts, or pass/fail accounting.
- Do not special-case `src/20140828-1.c`, `src/loop-2e.c`, `%t6`, or `%t23`.

## Working Model

Idea 645 proved the fused pointer branch terminator shape can lower when the
condition and exactly one compared pointer operand have explicit branch
stack-load authority. The remaining representative rows now reach object
emission but abort at runtime, which points to stale or unmaterialized
stack-carried pointer source publication rather than terminator admission.

## Execution Rules

- Keep routine packet progress and proof paths in `todo.md`.
- Start by refreshing object, prepared-BIR, disassembly, and runtime evidence
  for both representative values before implementation.
- Identify the producer, selected stack slot, source value, consumer branch,
  and publication or materialization point before changing code.
- Add support only when explicit prepared facts prove the pointer source is
  fresh and materialized at the selected consumer point.
- Preserve fail-closed diagnostics for missing source publication, stale stack
  slots, ambiguous pointer sources, mismatched homes, and unrelated branch
  shapes.
- If refreshed evidence belongs to the direct-global branch boundary, request
  lifecycle switch to idea 654 instead of broadening this plan.

## Steps

### Step 1: Refresh Stack-Carried Pointer Evidence

Goal: Confirm the current first owner for `%t6` in `src/20140828-1.c` and
`%t23` in `src/loop-2e.c`.

Actions:

- Capture prepared-BIR, object, disassembly, and runtime evidence for both
  representative rows.
- Record the selected stack slot, source value, consumer branch, and current
  pointer materialization or publication facts for each target value.
- Distinguish stale source, unmaterialized source, missing publication,
  mismatched home, and unrelated downstream runtime failures.

Completion check:

- `todo.md` records the refreshed evidence paths, the shared or separate first
  owners for `%t6` and `%t23`, and the next implementation or split boundary.

### Step 2: Locate The Publication And Materialization Boundary

Goal: Find the narrow producer or RV64 consumer surface that should own one
stack-carried pointer source rule.

Actions:

- Trace where the selected pointer source is written, carried through stack
  storage, reloaded, and consumed by the branch path.
- Identify which prepared fact should publish freshness, selected source
  identity, materialized value identity, and selected stack home at the
  consumer point.
- Choose one first implementation family only if the evidence proves a common
  semantic rule for at least one representative row.
- Define focused positive and fail-closed test shapes for complete, missing,
  stale, ambiguous, and mismatched pointer source publications.

Completion check:

- `todo.md` names the owned implementation surface, selected first family,
  focused proof target, and rejection behavior to preserve.

### Step 3: Implement The Narrow Stack-Carried Pointer Rule

Goal: Repair one proven stack-carried pointer source publication or
materialization owner without broad branch or stack rewrites.

Actions:

- Publish or consume explicit prepared facts for the selected stack-carried
  pointer source only when the producer can prove freshness and materialization
  at the consumer branch point.
- Keep stale, missing, ambiguous, and mismatched pointer source states rejected
  with precise diagnostics.
- Add focused positive coverage for the selected semantic shape.
- Add or preserve focused negative coverage for incomplete pointer source
  publication and unrelated branch shapes.

Completion check:

- Fresh build plus focused proof passes, or lifecycle state records the exact
  missing producer authority and parks or splits this route.

### Step 4: Prove Representative Integration

Goal: Show the representative stack-carried pointer rows advance past the
stale or unmaterialized pointer source failure mode.

Actions:

- Rerun focused coverage plus the RV64 GCC torture backend route for
  `src/20140828-1.c` and `src/loop-2e.c`.
- Capture prepared-BIR, object, disassembly, and runtime evidence showing the
  consumed pointer source is fresh and materialized at the selected branch.
- Record any distinct downstream owner if either representative advances but
  does not fully satisfy the source idea.

Completion check:

- `todo.md` records representative proof for both target values and any
  remaining downstream owner.

### Step 5: Run Broader Validation And Close Or Park

Goal: Decide whether the source idea is complete after focused and
representative proof.

Actions:

- Run the supervisor-selected broader validation for the affected prepared,
  RV64 backend, and representative GCC torture scope after focused proof is
  green.
- If acceptance criteria are satisfied, request plan-owner close with
  regression-guard proof.
- If a distinct producer or downstream owner remains, record it in `todo.md`
  and request lifecycle split or park instead of broadening this idea.

Completion check:

- Lifecycle state either closes the source idea with passing guard proof or
  records a precise blocked or follow-up owner without expanding this plan.
