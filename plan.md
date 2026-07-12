# Pre-Regalloc Value Constraint Carrier Research Runbook

Status: Active
Source Idea: ideas/open/723_pre_regalloc_value_constraint_carrier_research.md
Activated after parking: ideas/open/722_direct_edge_publication_available_move_contract.md

## Purpose

Resolve the ownership and schema uncertainty that blocks a genuine
distinct-register direct-edge publication without authorizing implementation.

## Goal

Produce the four required documents under
`docs/pre_regalloc_value_constraints/` with an evidence-backed semantic owner,
input boundary, and deterministic follow-up proof design.

## Core Rule

This runbook is documentation-only. Trace existing facts before recommending a
carrier, and do not encode the joined-branch fixture shape as architecture.

## Read First

- `ideas/open/723_pre_regalloc_value_constraint_carrier_research.md`
- `ideas/open/722_direct_edge_publication_available_move_contract.md`
- `src/backend/prealloc/regalloc.hpp`
- `src/backend/prealloc/regalloc.cpp`

## Current Scope

- Current fixed/preferred allocation-constraint production and consumption.
- Semantic ownership and schema alternatives.
- Deterministic route-independent proof and follow-up boundaries.

## Non-Goals

- Do not edit implementation or tests.
- Do not mutate allocator results or prepared homes.
- Do not resume idea 722 or x86 consumer work.
- Do not create an implementation idea until the research conclusion supports it.

## Execution Rules

- Use AST-backed symbol and call-chain evidence where available.
- Keep exactly one answer file per numbered question plus `index.md`.
- Cite concrete repository paths and symbols.
- Separate established facts, inference, and recommendation.

## Ordered Steps

### Step 1: Trace existing constraint flow

Goal: document all current producers and consumers of fixed/preferred value
constraints.

Actions:

- Trace semantic BIR and liveness identity into `PreparedAllocationConstraint`.
- Trace fixed/preferred names and placements through allocation and validation.
- Write `docs/pre_regalloc_value_constraints/01_existing_constraint_flow.md`.

Completion check:

- The document identifies supported inputs, derived policy, consumers, and the
  earliest missing general-value input boundary with concrete citations.

### Step 2: Decide semantic owner and schema

Goal: decide whether a general carrier is valid and where it belongs.

Actions:

- Compare BIR, prepared semantic, liveness, and regalloc ownership.
- Specify identity, class/width, target-legality, conflict, and failure invariants.
- Write `docs/pre_regalloc_value_constraints/02_semantic_owner_and_schema.md`.

Completion check:

- The document recommends one bounded schema/owner or provides evidence not to
  add a carrier; ambiguous ownership is not accepted as completion.

### Step 3: Define deterministic proof and follow-up

Goal: ensure any later implementation proves a semantic capability rather than
allocator luck or testcase shaping.

Actions:

- Define a route-independent positive program and adjacent negative matrix.
- Define deterministic proof requirements and forbidden shortcuts.
- Write `docs/pre_regalloc_value_constraints/03_proof_and_followup_boundary.md`.

Completion check:

- The proposed proof does not depend on incidental register order, fixture fact
  injection, post-prepare mutation, or target fallback.

### Step 4: Integrate and review the research result

Goal: make the research set navigable and ready for lifecycle disposition.

Actions:

- Write `docs/pre_regalloc_value_constraints/index.md` linking all three answers.
- Check exact file count and cross-document consistency.
- Review against the source idea’s reject signals.

Completion check:

- Exactly four required Markdown files exist, conclusions are consistent, and
  the result clearly identifies either a narrow follow-up initiative or a stop decision.
