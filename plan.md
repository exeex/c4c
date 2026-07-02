# BIR Call Metadata Semantic Producer Admission Runbook

Status: Active
Source Idea: ideas/open/558_bir_call_metadata_semantic_producer_admission.md

## Purpose

Repair BIR call metadata semantic producer admission for the current exact
`semantic lir_to_bir` call lane without letting prepared or RV64 consumers
infer missing call facts.

## Goal

Publish the direct-call callee, argument, and call-return result metadata
needed by semantic admission, then prove representative RV64 backend-object
rows advance for producer-owned reasons.

## Core Rule

Do not claim progress from downstream call lowering assumptions, generic
scalar/local-memory routing, diagnostic rewrites, expectation changes,
unsupported downgrades, allowlist changes, or weakened semantic admission.
Progress must come from BIR call producer facts that generalize across the
current direct-call and call-return rows.

## Read First

- `ideas/open/558_bir_call_metadata_semantic_producer_admission.md`
- `todo.md`
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_followups.md`
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_classification.md`
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_rows.md`
- `src/backend/bir/lir_to_bir/calling.cpp`
- adjacent BIR call semantic emission code
- `tests/backend/bir/`

## Current Targets

- Exact call metadata semantic rows: `55`.
- Topic counts: `52` direct-call rows and `3` call-return rows.
- Representative RV64 proof seeds:
  - `src/20000412-2.c` for direct-call metadata.
  - `src/20050121-1.c` for call-return metadata.
- Primary implementation surface:
  `src/backend/bir/lir_to_bir/calling.cpp` and directly adjacent call
  semantic producer code.

## Non-Goals

- Do not implement RV64/MIR call lowering that guesses missing BIR call facts.
- Do not treat call metadata as generic local-memory, runtime/intrinsic, or
  scalar/signature/control support.
- Do not repair runtime/intrinsic memory effects, local-memory facts, or
  bootstrap/global data-shape handoff in this runbook.
- Do not weaken semantic admission, expectations, unsupported markers,
  allowlists, or runtime comparison behavior.
- Do not prove only direct-call while leaving call-return unexamined.

## Working Model

- Representative RV64 rows are proof seeds, not testcase-shaped contracts.
- The direct-call family should publish enough callee and argument metadata
  for semantic admission to decide the call without prepared or RV64 inference.
- The call-return family should publish or admit result metadata for returned
  call values, or be split only if inspection proves a distinct producer
  boundary.
- If inspection proves a row belongs to an unrelated first owner, record that
  in `todo.md` and request lifecycle review instead of absorbing the work.

## Execution Rules

- Keep routine packet progress, proof commands, row seeds, and blockers in
  `todo.md`.
- Add focused BIR tests before or with each producer repair.
- Each code-changing packet needs fresh build proof plus the supervisor's
  delegated RV64 representative command.
- Use RV64 backend-object representative rows only to confirm that the old
  semantic call family moved for producer-owned reasons.
- Escalate validation if shared call helpers affect prepared/RV64 call
  behavior outside BIR semantic producer publication.

## Steps

### Step 1: Inspect Direct-Call Producer Boundary

Goal: identify the missing or malformed BIR metadata behind the
`src/20000412-2.c` direct-call representative.

Actions:

- Inspect the current `src/20000412-2.c` per-case log and any available BIR
  dumps for the failing `main` direct call.
- Trace direct-call emission through `src/backend/bir/lir_to_bir/calling.cpp`
  and adjacent semantic call admission code.
- Identify the exact callee, argument, type, or result metadata that semantic
  admission lacks.
- Select the smallest focused BIR test that pins the direct-call producer
  contract before implementation.

Completion check:

- `todo.md` names the direct-call boundary, the focused BIR test to add or
  extend, and the RV64 representative command the supervisor should use after
  repair.

### Step 2: Repair Direct-Call Metadata Publication

Goal: publish or admit direct-call callee and argument metadata in BIR without
moving the repair downstream.

Actions:

- Add or extend focused BIR tests for the direct-call contract selected in
  Step 1.
- Implement the minimal producer-side repair in the BIR call lowering surface.
- Preserve semantic admission checks, diagnostics, and existing local-memory
  and runtime/intrinsic behavior.
- Run the delegated backend build and focused BIR/backend test command.

Completion check:

- Focused BIR tests pass, direct-call metadata is present or semantically
  rejected for a documented reason, and `todo.md` records touched code
  surfaces plus neighboring call topics intentionally left for later.

### Step 3: Prove Direct-Call Representative And Classify Residual

Goal: prove `src/20000412-2.c` moves off the same direct-call semantic
admission failure for producer-owned reasons.

Actions:

- Run the supervisor-delegated RV64 backend-object command for
  `src/20000412-2.c`.
- Inspect the row `case.log` to confirm whether the old direct-call semantic
  admission failure moved.
- If it moves to call-return or another in-scope call metadata family, record
  that boundary in `todo.md`.
- If it moves downstream, keep that downstream issue out of this source idea
  unless the supervisor explicitly opens a separate route.

Completion check:

- `todo.md` records the exact command, result, current `case.log` outcome, and
  whether the next packet should target call-return or another call metadata
  producer boundary.

### Step 4: Inspect And Repair Call-Return Metadata

Goal: cover the smaller call-return family so the runbook does not leave the
`3` call-return rows unexamined.

Actions:

- Inspect `src/20050121-1.c` and at least one current call-return row log if
  the representative has moved.
- Trace returned call-value metadata through BIR call emission and semantic
  admission.
- Add focused BIR coverage for call-return result metadata.
- Repair the producer boundary or stop for lifecycle split if inspection
  proves call-return is a distinct source idea.
- Run the delegated backend proof command.

Completion check:

- `todo.md` records the call-return boundary, focused proof, representative
  outcome, and whether direct-call and call-return now share a completed call
  metadata route.

### Step 5: Reconcile Call Metadata Representatives

Goal: decide whether the source idea is complete, needs another runbook
checkpoint, or should split a distinct call-adjacent initiative.

Actions:

- Rerun the supervisor-delegated representative RV64 backend-object scan for
  direct-call and call-return rows.
- Compare current failures against the original direct-call and call-return
  semantic families.
- Confirm no expectation, unsupported-marker, allowlist, runtime comparison,
  or semantic admission weakening occurred.
- Request supervisor escalation if multiple call packets changed shared BIR
  call helpers broadly.

Completion check:

- `todo.md` records which representative rows moved, which still fail in
  semantic call metadata admission, and whether this source idea should
  continue with another checkpoint, split a distinct initiative, deactivate,
  or close.
