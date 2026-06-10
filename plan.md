# Phase B BIR Annotation Schema Candidate Audit Runbook

Status: Active
Source Idea: ideas/open/152_phase_b_bir_annotation_schema_candidate_audit.md

## Purpose

Audit which `Prepared*` fields should become BIR-owned annotation schema after
Phase A established the BIR-owned semantic relationships.

## Goal

Produce the durable Phase B analysis artifact at
`docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md` and any
follow-up schema prototype ideas justified by Phase A routes or closure notes.

## Core Rule

This plan is analysis-only. Do not change implementation code or schema code in
this runbook; classify fields, document traceability, and create follow-up
ideas for later implementation work.

## Read First

- `ideas/open/152_phase_b_bir_annotation_schema_candidate_audit.md`
- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- `ideas/closed/151_phase_a_bir_normalization_candidate_audit.md`
- `ideas/closed/152_bir_producer_source_identity_foundation.md`
- `ideas/closed/153_bir_select_chain_direct_global_identity.md`
- `ideas/closed/154_bir_memory_access_identity.md`
- `ideas/closed/155_bir_block_entry_publication_identity.md`
- `ideas/closed/156_bir_cfg_edge_publication_identity.md`
- `ideas/closed/157_bir_call_boundary_source_facts.md`
- `ideas/closed/158_bir_comparison_condition_producer_identity.md`

## Current Targets

- Durable artifact:
  `docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`
- Optional follow-up ideas under `ideas/open/`, only where the artifact shows a
  schema prototype or blocker is justified by Phase A evidence.

## Non-Goals

- Do not edit compiler implementation files.
- Do not directly introduce annotation schema types or BIR container changes.
- Do not invent new normalization routes that belong in Phase A.
- Do not copy whole `Prepared*` structs into BIR as schema candidates.
- Do not define annotations that expose AArch64 register names, scratch policy,
  or instruction spelling as durable BIR schema.

## Working Model

For each Phase A route, classify candidate data as one of:

- durable BIR annotation schema candidate
- private analysis/query cache
- bridge/oracle data used only to prove prepared-query equivalence
- blocked pending a stable BIR annotation container
- target-facing convenience that must remain outside BIR

Annotation placement must be considered across:

- BIR value annotations
- BIR instruction annotations
- BIR terminator annotations
- BIR block annotations
- BIR edge annotations
- BIR function annotations
- BIR module-level lowering metadata

## Execution Rules

- Preserve traceability from every schema candidate back to Phase A candidate
  ids, Phase A sections, or implementation closure notes.
- Prefer typed BIR-owned annotations over parallel `PreparedBirModule` state.
- Keep cheap lookup identity intact; do not require consumers to recover basic
  relationships through expensive scans.
- Consider future x86 and riscv consumers while classifying schema shape.
- Write durable conclusions into the Phase B artifact, not only into `todo.md`.
- If a separate implementation initiative is discovered, create a follow-up
  idea instead of broadening this analysis-only plan.

## Step 1: Build The Phase A Evidence Index

Goal: collect the source facts that Phase B is allowed to use.

Primary target:
`docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`

Actions:

- Inspect the Phase A artifact and closed follow-up notes listed in
  `Read First`.
- Extract the seven Phase A routes and their established BIR-owned semantic
  relationships.
- Record the `Prepared*` surfaces, BIR identity facts, and closure-note
  blockers relevant to annotation schema classification.
- Start the Phase B artifact with provenance, source list, and a route coverage
  table skeleton.

Completion check:

- The artifact exists and contains a Phase A route coverage table with one row
  for each of the seven Phase A routes.
- Each row names the evidence source that justifies later schema decisions.

## Step 2: Classify Prepared Fields By Annotation Placement

Goal: decide which candidate facts belong in BIR annotations and where.

Primary target:
`docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`

Actions:

- Inspect value-home and storage facts.
- Inspect call argument, call result, and outgoing-stack facts.
- Inspect publication and edge-publication facts.
- Inspect move-bundle and parallel-copy facts.
- Inspect address and materialization facts.
- Inspect comparison and materialized-condition facts.
- Inspect return-chain facts.
- Classify each candidate into value, instruction, terminator, block, edge,
  function, or module-level annotation placement where justified.

Completion check:

- The artifact contains a candidate BIR annotation schema map.
- Every candidate has traceability to Phase A evidence and a placement rationale.

## Step 3: Record Rejects, Cache-Only Data, And Target-Facing Data

Goal: keep non-schema data out of the durable BIR annotation surface.

Primary target:
`docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`

Actions:

- List `Prepared*` fields that should remain private or cache-only.
- List bridge/oracle fields used only for equivalence proof against prepared
  queries.
- List target-facing conveniences that must not enter BIR.
- Record blockers where Phase A proved identity but did not establish a stable
  annotation container.

Completion check:

- The artifact clearly separates schema candidates from cache-only data,
  bridge/oracle data, target-facing data, and blockers.
- No whole `Prepared*` struct is accepted as a schema candidate.

## Step 4: Create Follow-Up Schema Ideas

Goal: turn justified schema domains into later implementation initiatives.

Primary targets:

- `docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`
- `ideas/open/*.md`

Actions:

- Identify schema prototype domains supported by Phase A relationships or
  closure-note blockers.
- Create follow-up ideas only for domains that need implementation or prototype
  work after this analysis phase.
- Include reviewer reject signals in each new follow-up idea.
- Link each follow-up idea from the Phase B artifact.

Completion check:

- The artifact lists follow-up ideas by domain, or explicitly says no follow-up
  idea is justified for a domain.
- Any new `ideas/open/*.md` files include concrete reviewer reject signals.

## Step 5: Finalize Phase B Artifact And Closure Summary

Goal: make the Phase B output directly consumable by Phase C, Phase D, and
Phase E.

Primary target:
`docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`

Actions:

- Add migration constraints for keeping AArch64 behavior stable while
  annotations are introduced.
- Verify the artifact includes the required route coverage table, schema map,
  rejects/cache-only list, target-facing list, follow-up ideas, and traceability.
- Prepare a concise closure summary for the source idea.

Completion check:

- The artifact satisfies every item in the source idea's `Expected Output`
  section.
- The active plan is ready for lifecycle closure review, without requiring any
  implementation diff.
