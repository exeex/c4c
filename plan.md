# Phase F1 Final Prepared Field-Group Demotion Gate Runbook

Status: Active
Source Idea: ideas/open/247_phase_f1_final_prepared_field_group_demotion_gate.md

## Purpose

Decide whether any remaining `PreparedBirModule` or
`PreparedFunctionLookups` field group is now safe for final demotion,
deletion, privatization, or follow-up implementation after the Phase F1 x86,
riscv, and compatibility-retention prerequisite ideas closed.

## Goal

Produce a reviewer-ready field-group gate: either open narrow follow-up ideas
for safe final demotion/deletion candidates or close with a blocker map when
no field group is safe.

## Core Rule

Do not delete, privatize, or weaken prepared public authority in this runbook.
This plan is a gate and classification pass. Any implementation work must be
split into separate narrow ideas only after this gate proves the candidate.

## Read First

- `ideas/open/247_phase_f1_final_prepared_field_group_demotion_gate.md`
- `ideas/closed/243_phase_f0_x86_riscv_bir_portability_convergence_audit.md`
- `ideas/closed/244_phase_f1_x86_route6_call_wrapper_diagnostic_oracle_replacement.md`
- `ideas/closed/245_phase_f1_riscv_route5_route3_edge_publication_adapter.md`
- `ideas/closed/246_phase_f1_prepared_publication_status_compatibility_retention.md`
- Prepared lookup declarations and construction sites for
  `PreparedBirModule` and `PreparedFunctionLookups`
- x86/riscv wrapper, diagnostic, helper-oracle, fallback, and baseline tests

## Current Targets

- `src/backend/bir/`
- `src/backend/mir/x86/`
- `src/backend/mir/riscv/`
- `tests/backend/bir/`
- `ideas/open/` for any narrow follow-up implementation ideas

## Non-Goals

- Do not directly delete or privatize a field group.
- Do not open broad aggregate retirement under draft 155 without complete
  field-by-field proof.
- Do not treat x86-only, riscv-only, diagnostic-only, or compatibility-only
  proof as whole prepared aggregate readiness.
- Do not move target ABI, layout, register, stack, emission, formatting, or
  wrapper policy into BIR.
- Do not weaken expectations, wrapper-output contracts, supported-path
  contracts, or baselines to make a field group appear unused.

## Working Model

- A field group can be a duplicate semantic cache, private pass context,
  target-policy product, compatibility adapter, blocked public authority, or
  deletion candidate.
- A deletion or demotion candidate needs cross-target route-native proof for
  positive, missing, mismatch, duplicate, fallback, wrapper-output, and
  baseline behavior unless explicitly scoped as target-local private pass
  context.
- Prepared aggregate retirement must be justified field-by-field, not by a
  broad "BIR owns semantics" claim.

## Execution Rules

- Keep routine execution notes and packet proof in `todo.md`.
- If a safe demotion/deletion candidate exists, create one narrow source idea
  per candidate under `ideas/open/`; do not implement it here.
- If proof remains blocked, record the blocker map in `todo.md` and close or
  defer the gate only when the source idea criteria are satisfied.
- Preserve the reviewer reject signals from the source idea when writing any
  follow-up idea.
- For lifecycle-only close, use matching `test_before.log` and
  `test_after.log` plus the regression guard. For any future implementation
  idea spawned by this gate, require full backend or broader validation as the
  follow-up idea's proof contract.

## Step 1: Verify Prerequisites And Inventory Field Groups

Goal: confirm the prerequisite Phase F1 ideas are closed and name every
remaining prepared field group in scope.

Actions:

- Inspect the closed records for ideas 243, 244, 245, and 246.
- Inspect current `PreparedBirModule` and `PreparedFunctionLookups`
  declarations, constructors, readers, and public helper surfaces.
- Build an inventory covering Route 3, Route 4, Route 5, Route 6, wrappers,
  diagnostics, fallback, compatibility, and target policy.
- Record the inventory and any missing prerequisite proof in `todo.md`.

Completion Check:

- `todo.md` names every in-scope field group and explicitly states whether all
  prerequisite ideas are closed or which prerequisite is blocking the gate.

## Step 2: Classify Each Field Group

Goal: classify every inventoried field group by ownership and demotion
readiness.

Actions:

- For each field group, classify it as duplicate semantic cache, private pass
  context, target-policy product, compatibility adapter, blocked public
  authority, or deletion candidate.
- For each deletion or demotion candidate, check for cross-target
  route-native positive, missing, mismatch, duplicate, fallback,
  wrapper-output, and baseline proof.
- Keep target-local ABI, layout, register, stack, emission, formatting, and
  wrapper policy outside BIR.
- Record blockers when proof is target-local, diagnostic-only,
  compatibility-only, or otherwise incomplete.

Completion Check:

- `todo.md` contains a reviewer-ready map from each field group to its
  classification, proof evidence, and blocker or candidate status.

## Step 3: Decide Draft 155 And Follow-Up Ideas

Goal: decide whether draft 155 should be rewritten, superseded, kept blocked,
or retired, and create only justified narrow follow-up ideas.

Actions:

- Compare the Step 2 classification map against draft 155's broad prepared
  aggregate retirement scope.
- Choose exactly one disposition for draft 155: rewrite with narrow
  field-group packets, supersede with smaller ideas, keep blocked with named
  blockers, or retire obsolete because successor packets cover its useful
  scope.
- If a field group is safe for final demotion or deletion, create one narrow
  `ideas/open/*.md` follow-up for that field group with concrete proof
  requirements and reviewer reject signals.
- If no field group is safe, record the blocker map instead of creating an
  implementation idea.

Completion Check:

- `todo.md` records the draft 155 disposition and either lists the newly
  created narrow follow-up ideas or states that no field group is safe with a
  concrete blocker map.

## Step 4: Prepare Closure Evidence

Goal: make the gate closure decision auditable without implying implementation
progress.

Actions:

- Confirm `plan.md` and `todo.md` still point at
  `ideas/open/247_phase_f1_final_prepared_field_group_demotion_gate.md`.
- Ensure the field-group inventory, classification map, draft 155 disposition,
  and follow-up/blocker outcome are present in `todo.md`.
- Produce matching lifecycle proof logs for close, using the supervisor-chosen
  command and canonical `test_before.log` / `test_after.log`.
- Run the regression guard before accepting close.

Completion Check:

- The lifecycle owner can close the source idea with a durable closure note, or
  explicitly reject close with named missing evidence.
