# Prepared Inline-Assembly Explicit-Register Constraint Runbook

Status: Active
Source Idea: ideas/open/724_prepared_inline_asm_explicit_register_allocation_constraints.md
Activated after closing: ideas/closed/723_pre_regalloc_value_constraint_carrier_research.md

## Purpose

Implement the research-approved narrow RV64 semantic constraint path from
inline-assembly operand identity through common allocation and direct-edge
publication proof.

## Goal

Make one genuine RV64 explicit-register inline-assembly operand family causal
for common regalloc without exposing fixture allocation controls.

## Core Rule

Semantic operand meaning must own the request, structured prepared facts must
validate it before allocation, and every allocator route must enforce it.

## Read First

- `ideas/open/724_prepared_inline_asm_explicit_register_allocation_constraints.md`
- `docs/pre_regalloc_value_constraints/index.md`
- `docs/pre_regalloc_value_constraints/02_semantic_owner_and_schema.md`
- `docs/pre_regalloc_value_constraints/03_proof_and_followup_boundary.md`

## Current Scope

- One supported RV64 explicit-register inline-assembly operand family.
- Prepared request ingress, validation, normalized constraint publication, and
  common allocator enforcement.
- Deterministic semantic positive and focused fail-closed negatives.

## Non-Goals

- No arbitrary named-value hints or fixture register maps.
- No broad parser, ABI, emission, scheduling, multi-target, Route 3/Route 5, or
  joined-branch work.
- No post-regalloc/prepared mutation or consumer reconstruction.

## Execution Rules

- Localize the existing semantic spelling path before changing schema.
- Stop if preservation requires broad inline-assembly redesign.
- Keep structured target identity and typed provenance authoritative.
- Apply one constraint-aware candidate helper to normal and eviction paths.
- Preserve precise fail-closed statuses and natural coalescing behavior.

## Ordered Steps

### Step 1: Localize and preserve one semantic explicit-register operand

Goal: identify one already supported RV64 operand spelling and retain its
explicit target identity through BIR metadata without broad parsing work.

Actions:

- Trace frontend/lowering spelling into `InlineAsmOperandMetadata`.
- Distinguish explicit identity from class-only and tied constraints.
- Add the minimal structured metadata and focused semantic-lowering proof.
- Stop for lifecycle review if the route requires broad redesign.

Completion check:

- A genuine semantic operand publishes structured RV64 identity before
  preparation, with malformed/unsupported spellings rejected precisely.

### Step 2: Publish and admit prepared value constraints

Goal: create authenticated pre-regalloc requests and normalized rows.

Actions:

- Publish requests keyed by interned function/value identity and typed operand
  provenance before `run_regalloc`.
- Validate identity, architecture, bank/class, width, span, liveness match,
  ambiguity, and contradictions.
- Normalize accepted identity into coherent `PreparedAllocationConstraint`
  fields and typed status/provenance.

Completion check:

- Supported requests produce one coherent normalized row; missing, stale,
  ambiguous, mismatched, unsupported, and contradictory inputs fail closed.

### Step 3: Enforce constraints in common allocation

Goal: make normalized fixed authority causal for every assignment route.

Actions:

- Build one constraint-aware candidate-span filtering/ordering helper.
- Use it in all normal and eviction replacement passes.
- Implement deterministic fixed-conflict and requires-register failure behavior.
- Preserve existing unconstrained allocation and spill behavior.

Completion check:

- Required identities are assigned regardless of allocator order or unrelated
  pressure, and no fallback pass bypasses the row.

### Step 4: Prove semantic edge publication and negatives

Goal: demonstrate downstream capability without testcase-shaped controls.

Actions:

- Build the research-defined predecessor-output / phi-use semantic positive
  with distinct explicit identities.
- Prove a genuine non-coalesced move and typed direct-edge `Available` result.
- Cover focused conflict, publication, freshness, and natural-coalescing negatives.
- Run matching focused proof, then supervisor-selected broader backend guard.

Completion check:

- Semantic positive and negative matrix pass without fixture injection,
  mutation, target fallback, or expectation changes; broader regression guard
  reports no new failures.
