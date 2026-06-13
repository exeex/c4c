# Phase E5 PreparedBirModule Demotion Or Retirement Gate Runbook

Status: Active
Source Idea: ideas/open/239_phase_e5_prepared_bir_module_demotion_or_retirement_gate.md

## Purpose

Decide whether E0 through E4 evidence is sufficient to keep, rewrite,
supersede, or eventually open draft 155, and whether any selected
`PreparedBirModule` or `PreparedFunctionLookups` field group is ready for a
later demotion or retirement packet.

## Goal

Produce a conservative E5 gate artifact that names safe field-group outcomes,
remaining blockers, compatibility adapters, proof expectations, and any final
follow-up implementation ideas.

## Core Rule

Treat this as a readiness gate before deletion. Do not delete or broadly
demote `PreparedBirModule` or `PreparedFunctionLookups` during this plan.

## Read First

- `ideas/open/239_phase_e5_prepared_bir_module_demotion_or_retirement_gate.md`
- `docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md`
- `docs/bir_prealloc_fusion/phase_e1_semantic_duplicate_candidate_triage.md`
- `docs/bir_prealloc_fusion/phase_e2_prepared_lookup_api_private_pass_context_readiness.md`
- `docs/bir_prealloc_fusion/phase_e3_prepared_diagnostic_oracle_replacement_readiness.md`
- `docs/bir_prealloc_fusion/phase_e4_cross_target_wrapper_convergence_readiness.md`
- Relevant E1/E2/E3/E4 closed follow-up notes under `ideas/closed/`

## Current Scope

- Verify E0 through E4 closure evidence and accepted follow-up status,
  including idea 238.
- Map `PreparedBirModule` construction, mutation, debug, oracle, target wrapper,
  fallback, and consumer surfaces.
- Map `PreparedFunctionLookups` construction and public lookup delivery
  surfaces.
- Classify each field group as demotion-ready, target/prepared policy,
  private pass context, public fallback/oracle, diagnostic/string authority, or
  cross-target compatibility.
- Decide whether draft 155 should be rewritten, opened later, superseded, or
  kept blocked.
- Produce final proof strategy and follow-up idea files only for safe final
  packets.

## Non-Goals

- Do not directly delete `PreparedBirModule`.
- Do not directly delete `PreparedFunctionLookups`.
- Do not perform rename-only facade work.
- Do not combine this gate with unrelated backend feature work.
- Do not remove target policy, diagnostics, fallback, wrapper behavior, or
  public consumers without proven replacements.
- Do not claim riscv readiness from x86-only evidence.
- Do not weaken baselines, expected strings, helper-oracle names,
  supported-path status, printer/debug output, wrapper output, or fallback
  behavior.

## Working Model

- E5 may produce a readiness document even if blockers remain.
- Open or unresolved prerequisite evidence blocks demotion or retirement
  readiness, but should still be recorded explicitly.
- Selected route-reader, diagnostic-row, private-pass-context, or x86-only
  wrapper evidence is not whole-module retirement evidence.
- Follow-up implementation ideas must be one field group at a time.

## Execution Rules

- Keep routine findings in `todo.md` until they are ready for the E5 gate
  artifact.
- Prefer documentation and lifecycle artifacts for gate conclusions; do not
  change implementation files in this plan unless the supervisor delegates a
  later implementation packet from a follow-up idea.
- If a candidate field group lacks cross-target, diagnostic, fallback,
  baseline, or public-consumer proof, classify it as blocked or retained.
- Treat testcase-overfit, expectation downgrades, helper renames, and
  classification-only changes as non-progress.
- Before closure, validation must include regression-guard evidence chosen by
  the supervisor for this docs/lifecycle gate and must record any full-suite or
  baseline expectations in the closure note.

## Steps

### Step 1: Build the E5 Evidence Inventory

Goal: collect prerequisite and blocker evidence before drafting conclusions.

Primary targets:

- E0 through E4 readiness documents.
- E1/E2/E3/E4 follow-up closure notes under `ideas/closed/`.
- Idea 238 status under `ideas/open/` or `ideas/closed/`.
- Existing draft 155 state, if present.

Actions:

- Verify that E0 has a field-by-field ownership map.
- Verify accepted E1 semantic duplicate migrations have closed.
- Verify accepted E2 public lookup/API contractions have closed.
- Verify required E3 diagnostic/oracle replacements have closed.
- Verify E4 readiness has closed and record whether idea 238 is closed or a
  blocker.
- Record baseline and string-authority evidence that must remain
  non-regressive.
- Identify any missing durable artifacts or unresolved blockers.

Completion check:

- `todo.md` records the prerequisite inventory, idea 238 status, blocker list,
  and the exact artifact set that Step 2 should classify.

### Step 2: Map PreparedBirModule and PreparedFunctionLookups Field Groups

Goal: build a field-group map from prepared aggregate surfaces to current
owners and consumers.

Primary targets:

- `PreparedBirModule` construction and mutation sites.
- `PreparedFunctionLookups` construction and public lookup-group delivery.
- MIR/codegen, printer/debug, target-wrapper, oracle, fallback, and test
  consumers.

Actions:

- Inspect construction, mutation, and read paths for each selected field group.
- Identify which consumers require public prepared artifact visibility.
- Separate semantic route facts from target policy products, diagnostics,
  string authority, fallback behavior, and wrapper compatibility.
- Record compatibility adapters that would be required during a later
  transition.

Completion check:

- `todo.md` contains a field-group matrix with current owner, consumers,
  candidate disposition, required adapters, and missing proof.

### Step 3: Draft the E5 Gate Artifact

Goal: write the durable E5 readiness gate conclusions.

Primary target:

- `docs/bir_prealloc_fusion/phase_e5_prepared_bir_module_demotion_or_retirement_gate.md`

Actions:

- State whether draft 155 should be rewritten, opened, superseded, or kept
  blocked.
- State whether idea 238 was closed before E5 ran or remains a blocker.
- List field groups ready for later demotion or retirement.
- List field groups that must remain target/prepared policy or private pass
  context.
- List field groups that must remain public fallback/oracle,
  diagnostic/string, or cross-target compatibility surfaces.
- Name compatibility adapters required during transition.
- Include a final proof strategy with full-suite and baseline expectations.

Completion check:

- The E5 document exists, matches the source idea's expected output, and does
  not claim readiness beyond the evidence inventory.

### Step 4: Create Safe Follow-Up Implementation Ideas

Goal: create only the final implementation ideas that the E5 gate artifact
proves are safe.

Primary targets:

- `ideas/open/*.md`
- Any existing draft 155 file referenced by the E5 gate conclusion.

Actions:

- If draft 155 should be rewritten or superseded, perform only the delegated
  lifecycle-safe rewrite or successor creation.
- Create follow-up implementation ideas one field group at a time.
- Include concrete reviewer reject signals for named-case-only fixes,
  expectation downgrades, broad rewrites, facade-only changes, and retained old
  failure modes.
- Do not create follow-up ideas for blocked or under-proven field groups.

Completion check:

- Each opened follow-up has a narrow field-group scope, acceptance criteria,
  proof expectations, and reviewer reject signals.

### Step 5: Validate and Prepare Closure Notes

Goal: prove the gate is documentation/lifecycle safe and ready for closure.

Primary targets:

- `docs/bir_prealloc_fusion/phase_e5_prepared_bir_module_demotion_or_retirement_gate.md`
- Any follow-up ideas created in Step 4.
- `todo.md`

Actions:

- Run whitespace and lifecycle reference checks for the changed artifacts.
- Record supervisor-selected regression guard or baseline evidence for the
  gate.
- Confirm no implementation files changed as part of the E5 gate.
- Summarize final draft 155 disposition, field-group outcomes, blockers, proof
  strategy, and follow-up idea list.

Completion check:

- `todo.md` contains closure-ready notes and validation evidence sufficient for
  the plan owner to decide whether source idea 239 is complete.
