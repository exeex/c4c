# RV64 Out-Of-SSA Phi Join Immediate Materialization Runbook

Status: Active
Source Idea: ideas/open/506_rv64_out_of_ssa_phi_join_immediate_materialization.md

## Purpose

Activate the focused RV64 route for coherent generic out-of-SSA
`phi_join_immediate_materialization` moves.

## Goal

Classify and, only if facts prove coherent, materialize generic out-of-SSA
immediate phi-join moves using explicit prepared parallel-copy authority and
explicit immediate value facts.

## Core Rule

Do not infer immediate values, destination identity, or edge coordinates from
testcase shape, source text, raw BIR order, object output, or target register
behavior. Missing facts must remain fail-closed.

## Read First

- ideas/open/506_rv64_out_of_ssa_phi_join_immediate_materialization.md
- build/agent_state/504_step3_select_publication_consumer_classification/summary.md
- build/agent_state/504_step3_select_publication_consumer_classification/rows.tsv
- build/agent_state/504_step3_select_publication_consumer_classification/classification_counts.tsv

## Current Scope

- Generic prepared move-bundle rows with:
  - `event_kind=pre_terminator_copies`
  - `phase=block_entry`
  - `authority=out_of_ssa_parallel_copy`
  - `move_reason=phi_join_immediate_materialization`
  - `fragment_status=generic_move_bundle_materialization_failed`
- Representative evidence includes `src/pr37924.c`, but the implementation
  route must prove whether it belongs to a broader coherent family before
  materializing anything.

## Non-Goals

- Select-publication evidence, intent support, or materialization.
- Before-instruction move handling.
- Out-of-SSA register/stack preservation.
- Before-return materialization.
- Broad prepared move-bundle rewrites unrelated to immediate phi joins.
- Expectation rewrites, unsupported-marker downgrades, allowlist changes,
  pass/fail accounting changes, runtime-comparison changes, or baseline churn.

## Working Model

- Prepared parallel-copy authority owns whether a generic out-of-SSA move may
  be consumed by RV64.
- Prepared immediate value facts own the source value for immediate
  materialization.
- RV64 lowering may materialize only rows whose authority, edge/block identity,
  destination storage, and immediate facts are all explicit and internally
  coherent.
- Ambiguous edge identity, unsupported destination storage, cycles, coordinate
  confusion, or absent immediate facts must keep the row unsupported.

## Execution Rules

- Keep classification before implementation.
- Use focused evidence files under `build/agent_state/` for row selection and
  family counts; create new focused evidence only if the existing 504 handoff
  is insufficient.
- Keep producer gaps out of RV64 lowering. If prepared facts are missing or
  incoherent, stop and route that as producer work instead of reconstructing
  facts in target code.
- Each code-changing step needs fresh build proof plus the narrow backend
  command chosen by the supervisor.
- Escalate to broader validation before closure if the materialization path
  touches shared move-bundle lowering.

## Ordered Steps

### Step 1: Classify Generic Immediate Phi-Join Rows

Goal: Establish the exact row family and prove whether implementation is
justified.

Primary target: Existing 504 classification evidence plus any focused
inspection needed for `generic_move_bundle_materialization_failed` rows.

Actions:

- Inspect the 504 handoff files and identify all rows matching the current
  scope.
- Confirm which rows have explicit out-of-SSA parallel-copy authority.
- Confirm which rows expose explicit immediate facts suitable for target
  materialization.
- Separate coherent immediate rows from rows blocked by missing edge identity,
  missing immediate facts, unsupported destination storage, cycles, or
  coordinate ambiguity.
- Record the classification and suggested proof command in `todo.md`.

Completion check:

- `todo.md` names the coherent family, representative rows, blockers, and the
  exact next implementation target, or records that no target-side
  materialization is justified.

### Step 2: Implement The Narrow Materialization Path

Goal: Materialize only the coherent immediate phi-join family proven in Step 1.

Primary target: RV64 object lowering for prepared generic move-bundle
materialization.

Actions:

- Locate the fail-closed `generic_move_bundle_materialization_failed` path.
- Add the smallest semantic rule that consumes prepared parallel-copy
  authority and explicit immediate facts for supported destination storage.
- Preserve existing fail-closed diagnostics for all incoherent or unsupported
  cases found in Step 1.
- Avoid special-casing `src/pr37924.c` or any source/test/function/block name.

Completion check:

- The focused representative row lowers through the new semantic path, and
  nearby incoherent rows still fail closed with stable diagnostics.

### Step 3: Add Focused Backend Coverage

Goal: Cover the implemented family without weakening contracts.

Primary target: Focused backend tests or existing backend scan harness entries
that exercise generic out-of-SSA immediate materialization.

Actions:

- Add or update tests only for the coherent materialization family.
- Assert the semantic capability, not a brittle source-name or row-name shape.
- Keep unsupported expectations for blocked rows unless implementation
  legitimately handles their facts too.

Completion check:

- Focused tests fail before the implementation or cover the old unsupported
  path, then pass after the implementation without expectation downgrades.

### Step 4: Validate And Prepare Closure Evidence

Goal: Prove the route is narrow, non-overfit, and stable enough for supervisor
  acceptance.

Primary target: Build plus supervisor-selected backend proof, with broader
validation if shared move-bundle lowering changed.

Actions:

- Run the delegated build and narrow backend proof exactly as assigned by the
  supervisor.
- If shared move-bundle lowering changed, request or run the broader validation
  checkpoint the supervisor selects.
- Summarize proof results, remaining unsupported families, and any producer
  follow-up needed in `todo.md`.

Completion check:

- Fresh proof is recorded in `todo.md`, no reject signal from the source idea
  applies, and remaining gaps are either outside this idea or recorded as
  separate follow-up work.
