# Phase A BIR Normalization Candidate Audit Runbook

Status: Active
Source Idea: ideas/open/151_phase_a_bir_normalization_candidate_audit.md

## Purpose

Audit `Prepared*` facts that are actually BIR normalization relationships and
produce durable handoff material for the BIR/prealloc thinning program.

## Goal

Create `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md` with a
candidate/reject classification, dependency order, and concrete follow-up idea
payloads for BIR-owned normalization work.

## Core Rule

This plan is analysis-only. Do not rewrite BIR, prealloc, MIR consumers,
build metadata, or test expectations while executing this runbook.

## Read First

- `ideas/open/151_phase_a_bir_normalization_candidate_audit.md`
- `docs/bir_prealloc_fusion/README.md`
- `src/backend/bir/`
- `src/backend/prealloc/`
- `src/backend/mir/aarch64/codegen/`
- recent closure notes for ideas 130-150 under `ideas/closed/`

## Current Targets

- Durable artifact:
  `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- Source domains:
  - same-block producer and same-block materialization relationships
  - select-chain dependency relationships
  - current-block entry publication relationships
  - edge-publication and parallel-copy source relationships
  - call-boundary semantic relationships that are not ABI placement
  - memory/addressing access-identity relationships
  - comparison/materialized-condition producer relationships

## Non-Goals

- Do not treat every `Prepared*` fact as a BIR-normalization candidate.
- Do not move ABI placement, stack offsets, target register spelling, scratch
  policy, final instruction routing, or target addressing modes into BIR.
- Do not create vague follow-up ideas such as "move prepared into BIR".
- Do not delete or weaken prepared facts before MIR consumers have equivalent
  BIR-owned relationships.
- Do not claim compiler capability progress from this analysis-only plan.

## Working Model

BIR should own target-neutral semantic relationships before prealloc runs.
Prealloc may still own allocation, layout, ABI placement, target routing, and
final emission preparation. The audit must separate normalizable semantic
relationships from facts that must remain out of BIR.

## Execution Rules

- Keep durable analysis in the phase artifact, not only in `todo.md`.
- Use closure notes for ideas 130-150 as historical evidence, but do not edit
  closed ideas.
- When proposing follow-ups, include bounded file targets, proof routes, and
  reject signals.
- Prefer smaller route-specific follow-up ideas over broad program-level ideas.
- If a domain is rejected from BIR normalization, record the concrete reason.

## Step 1: Initialize Phase Artifact And Evidence Inventory

Goal: create the durable artifact shell and gather the inspection inventory.

Primary target:
`docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`

Concrete actions:

- Create or refresh the artifact with sections for:
  - BIR-normalization candidates
  - facts rejected from BIR normalization
  - dependency order
  - follow-up idea payloads
  - proof-route recommendations
- Inventory relevant `Prepared*` structs, fields, helper functions, and query
  APIs in `src/backend/prealloc/`.
- Inventory BIR-owned node, block, function, and edge data surfaces in
  `src/backend/bir/`.
- Inventory AArch64 MIR consumers in `src/backend/mir/aarch64/codegen/`.
- Read closure notes for ideas 130-150 and extract only facts relevant to
  prepared/BIR ownership.

Completion check:

- The artifact exists with the required section skeleton.
- `todo.md` names the files and closure notes inspected.
- No implementation files or test expectations changed.

## Step 2: Classify Candidate BIR Relationships

Goal: identify `Prepared*` facts whose meaning is target-neutral BIR semantics.

Concrete actions:

- Classify same-block producer/materialization relationships.
- Classify select-chain dependency relationships.
- Classify current-block entry publication relationships.
- Classify edge-publication and parallel-copy source relationships.
- Classify call-boundary semantic relationships that are not ABI placement.
- Classify memory/addressing relationships that represent semantic access
  identity rather than frame offsets or target addressing modes.
- Classify comparison/materialized-condition producer relationships.
- For each candidate, record current owner, current consumers, proposed BIR
  owner surface, and why it is semantic rather than allocation/layout policy.

Completion check:

- The artifact contains a candidate table with enough evidence for a reviewer
  to accept or reject each candidate independently.

## Step 3: Classify Rejected Prepared Facts

Goal: prevent target-specific or placement-specific facts from leaking into BIR.

Concrete actions:

- Identify facts that encode ABI placement, stack offsets, register spelling,
  scratch policy, target routing, final emission, or concrete target addressing
  modes.
- Record why each rejected fact must stay in prealloc, MIR/codegen, ABI, stack
  layout, or another non-BIR owner.
- Cross-check that no rejected fact is needed as a dependency for a candidate
  without a separate semantic relationship.

Completion check:

- The artifact contains a reject table with concrete owner/reason entries.
- Candidate rows do not depend on smuggling target-local facts into BIR.

## Step 4: Order Safe Normalization Routes

Goal: turn accepted candidates into an implementation dependency order.

Concrete actions:

- Group candidates into concrete normalization routes.
- Order routes so MIR consumers can switch only after equivalent BIR-owned
  relationships exist.
- Mark routes that need schema or annotation decisions from later phases.
- Identify dependencies between producer, publication, edge, call, memory, and
  comparison routes.

Completion check:

- The artifact contains a dependency order that can be consumed by Phase B and
  by future implementation ideas.

## Step 5: Draft Follow-Up Idea Payloads

Goal: produce bounded follow-up ideas for each concrete normalization route.

Concrete actions:

- For every accepted route, draft a proposed idea filename or filename prefix.
- Include goal, scope, out-of-scope guardrails, acceptance criteria, proof
  route, and reviewer reject signals.
- Keep each follow-up narrow enough to implement and validate without broad
  backend rewrites.
- Explicitly note candidate groups that should not create follow-up ideas.

Completion check:

- The artifact contains concrete follow-up payloads rather than broad
  placeholder ideas.

## Step 6: Assemble Closure Payload

Goal: prepare the lifecycle close package for the source idea.

Concrete actions:

- Summarize the artifact contents in `todo.md`.
- Ensure the closure payload can include:
  - link to the artifact
  - candidate table
  - reject table
  - dependency order
  - follow-up ideas
  - proof-route recommendations
- Verify the run remained analysis-only.

Completion check:

- `todo.md` records the close-ready summary.
- The artifact has the required durable handoff data.
- No implementation files or backend expectations were changed by the audit.
