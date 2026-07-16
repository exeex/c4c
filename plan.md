# LIR CFG And PHI Raw-Binding Evidence Runbook

Status: Active
Source Idea: ideas/open/850_lir_cfg_phi_raw_bindings_evidence.md
Activated after: ideas/closed/849_lir_intrinsic_binding_evidence.md

## Purpose

Produce the documentation evidence required to classify `LirPhi` and
terminator operand/incoming-binding seams before any producer handoff, 734
receiver work, or 797 convergence can claim coverage.

## Goal

Map every `LirPhi` and terminator form through producer fields, verifier
checks, and new-BIR receiver paths, distinguishing native IDs from raw rendered
seams.

## Core Rule

This is research and architecture documentation only. Do not edit
implementation files, tests, expectations, unsupported markers, allowlists,
runtime behavior, or lifecycle history beyond this active plan.

## Read First

- `ideas/open/850_lir_cfg_phi_raw_bindings_evidence.md`
- `docs/lir_string_authority_remaining_routes/handoff_to_813.md`
- `docs/lir_string_semantic_authority_completion/input_validation.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/open/797_lir_to_new_bir_final_coverage_convergence.md`

## Current Scope

- Every `LirPhi` incoming binding and terminator operand seam.
- Producer, verifier, and BIR receiver evidence for native block, value, type,
  owner, predecessor, and edge relations.
- Positive and applicable malformed, foreign, and mismatch expectations.
- Exact dependency/return relation to producer handoffs, 734, and 797.

## Non-Goals

- No implementation changes.
- No general operand, call, or body-parameter authority work.
- No reopening accepted bounded CFG/PHI rows.
- No text-based recovery of block, value, type, owner, predecessor, or edge
  facts.
- No 734 receiver or 797 convergence claim before an exact accepted handoff.

## Working Model

850 is the ordered evidence successor after 849. It may identify a raw seam or
exact producer-successor requirement, but 734 remains only the downstream
receiver owner.

## Execution Rules

- Create exactly the required documentation directory and files:
  `docs/lir_cfg_phi_raw_bindings_evidence/index.md` and
  `docs/lir_cfg_phi_raw_bindings_evidence/01_cfg_phi_raw_binding_route.md`.
- The numbered answer file must contain the per-form map; the index only
  summarizes and links it.
- Label native IDs and raw seams explicitly.
- Preserve accepted 734 CFG/PHI rows as historical evidence, not a family-wide
  closure.

## Ordered Steps

### Step 1 - Inventory PHI And Terminator Forms

Goal: enumerate every `LirPhi` and terminator form in the LIR model.

Actions:
- Inspect LIR IR definitions, producers, printer, verifier, and Raw-BIR
  receiver/importer paths.
- List each form and identify its block/value/type fields.
- Mark fields as native structured IDs or raw/display seams.

Completion check:
- The numbered answer contains a complete per-form inventory with source
  locations and native/raw labels.

### Step 2 - Trace Verifier And Receiver Boundaries

Goal: map each form to verifier and Raw-BIR receiver behavior.

Actions:
- Record positive verifier and receiver evidence where present.
- Record malformed predecessor/value, foreign block/value, type, and edge
  mismatch expectations where applicable.
- State when evidence is missing or only a bounded 734 row is accepted.

Completion check:
- The numbered answer separates accepted 734 receipts from unproved raw seams.

### Step 3 - Write Dependency And Return Conclusions

Goal: define the exact downstream path without claiming implementation.

Actions:
- State whether any exact typed handoff is proved.
- If not, identify the kind of producer successor needed.
- Preserve 734 as receiver owner and 797 as downstream convergence owner.

Completion check:
- The numbered answer and index agree on return ordering and do not claim a
  new 734 receipt.

### Step 4 - Validate Documentation Shape

Goal: prove the deliverable is complete and bounded.

Actions:
- Check that `docs/lir_cfg_phi_raw_bindings_evidence/` contains exactly
  `index.md` and `01_cfg_phi_raw_binding_route.md`.
- Run `git diff --check`.
- Run any lightweight text/link checks the repository already provides if a
  matching docs command exists.

Completion check:
- Required files exist with the required shape, no extra files are present in
  the directory, `git diff --check` passes, and no implementation files are
  modified.
