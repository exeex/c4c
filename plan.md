# RV64 gcc_torture Prepared Module Shape Classification Runbook

Status: Active
Source Idea: ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md

## Purpose

Finish the RV64 gcc_torture prepared-module-shape umbrella by validating that
the failure classification, child idea split, and completed downstream repairs
still satisfy the source idea.

## Goal

Decide whether idea 354 can close, or identify the smallest remaining
classification or follow-up gap that prevents closure.

## Core Rule

Treat this as an analysis and lifecycle umbrella. Do not implement lowering
repairs, weaken gcc_torture expectations, or claim capability progress from
classification-only edits.

## Read First

- `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
- `review/354_prepared_shape_classification.md`, if present
- closed child ideas named by the source idea:
  - `ideas/closed/355_rv64_prepared_object_shape_diagnostics.md`
  - `ideas/closed/356_rv64_object_route_assembler_backed_prepared_text.md`
  - `ideas/closed/357_rv64_object_route_data_sections_globals_strings.md`
  - `ideas/closed/358_rv64_object_route_abi_width_edges.md`
  - `ideas/closed/359_bir_prepared_object_consumer_contract_completion.md`
- current RV64 gcc_torture backend scan artifacts under `build/agent_state/`
  and `build/rv64_gcc_c_torture_backend/`, when available

## Scope

- Reconcile the source idea's acceptance criteria against current artifacts.
- Confirm that generated child ideas were closed or intentionally superseded.
- Refresh representative scan evidence only where needed for closure
  confidence.
- Produce a clear closure decision for the supervisor.

## Non-Goals

- Do not create another broad RV64 object-route implementation plan inside
  this umbrella.
- Do not add testcase-shaped shortcuts or case-specific allowlist changes.
- Do not downgrade gcc_torture runner contracts.
- Do not edit implementation files or backend tests from this plan.
- Do not require a full gcc_torture scan unless existing artifacts and a
  targeted representative rerun are insufficient to decide closure.

## Working Model

Idea 354 began as a classification umbrella for the dominant
`RISC-V backend object route unsupported prepared module shape` failure family.
The source idea already records the classification artifact, bucket counts, and
child ideas. Since all listed child ideas are now closed, the remaining work is
to verify that the classification still covers the dominant failure family and
that no in-scope untriaged prepared-shape bucket remains.

## Execution Rules

- Prefer existing scan and review artifacts before rerunning heavy scans.
- When refreshing evidence, use a temporary copied allowlist or `MAX_CASES`
  only for probes and record that scope in `todo.md`.
- Preserve root-level canonical logs: `test_before.log` and `test_after.log`
  are for regression proof; put umbrella analysis logs under
  `build/agent_state/`.
- If closure is not justified, update `todo.md` with the exact missing bucket
  or child idea needed; do not silently expand this runbook.
- If a new distinct initiative is discovered, write it as a new
  `ideas/open/*.md` only after confirming it is outside the current umbrella's
  already generated children.

## Steps

### Step 1: Rehydrate Classification Evidence

Goal: Confirm the original prepared-shape classification artifact and scan
inputs are available or identify the minimum refresh needed.

Actions:

- Inspect `review/354_prepared_shape_classification.md`, if present.
- Inspect `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv` and
  `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`, if present.
- Check whether the recorded counts and representative buckets in the source
  idea are traceable to actual logs.
- If artifacts are missing or stale enough to block closure, record the exact
  refresh command needed in `todo.md`.

Completion Check:

- `todo.md` states whether the classification evidence is reusable, missing,
  or requires a bounded refresh before closure.

### Step 2: Verify Child Idea Coverage

Goal: Confirm every generated repair or prerequisite idea from the umbrella is
closed or intentionally superseded.

Actions:

- Read the closed child ideas listed in `Read First`.
- Confirm each child closure covers the bucket or layer boundary assigned by
  idea 354.
- Check whether any generated child remains open under `ideas/open/`.
- If a child closure left a deliberate follow-up outside idea 354, record
  whether that follow-up blocks this umbrella.

Completion Check:

- `todo.md` records a child coverage matrix and any blocker for closure.

### Step 3: Refresh Representative Backend Evidence

Goal: Provide current proof that the repaired buckets no longer leave an
obvious prepared-module-shape representative failure in the source idea's
scope.

Actions:

- Build `c4cll` if needed.
- Run a representative RV64 gcc_torture backend allowlist that covers the
  source idea's major buckets and the most recent child closure evidence.
- Store noncanonical analysis logs under `build/agent_state/`.
- Keep `test_after.log` only for the supervisor's chosen regression proof if
  requested.

Completion Check:

- Representative evidence is recorded in `todo.md` with the exact command,
  pass/fail counts, and any remaining prepared-shape failures.

### Step 4: Closure Decision

Goal: Decide whether idea 354 satisfies its acceptance criteria.

Actions:

- Compare Step 1 through Step 3 evidence against the source idea's
  `Acceptance` section.
- If closure is justified, ask the plan owner close flow to archive the idea
  after running the required regression guard.
- If closure is not justified, update `todo.md` with the smallest remaining
  lifecycle action: refresh classification, create one child idea, or mark a
  named child as intentionally superseded.

Completion Check:

- The supervisor has a clear close/defer decision with enough evidence to
  choose the next delegated packet.
