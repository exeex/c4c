# Pass-Ready BIR Schema And Legacy Quarantine Research Runbook

Status: Active
Source Idea: ideas/open/715_pass_ready_bir_schema_and_legacy_quarantine_research.md

## Purpose

Establish an evidence-backed target contract for a mutable, pass-ready BIR and
an observational legacy quarantine before any post-BIR pipeline rewrite.

## Goal

Produce `docs/backend/pass_ready_bir/index.md` and exactly six numbered answer
files that inventory current BIR, define the target schema and APIs, and order
a reviewable migration with explicit compatibility exits.

## Core Rule

This is research only. Separate confirmed current-code facts, inferences,
design decisions, and unresolved choices; do not change implementation,
tests, expectations, supported behavior, or lifecycle source intent.

## Read First

- `ideas/open/715_pass_ready_bir_schema_and_legacy_quarantine_research.md`
- `src/backend/bir/lir_to_bir/`
- `src/backend/bir/`
- `ref/claudes-c-compiler/src/backend/`
- accepted artifacts from the 703--714 series when available

## Current Scope

- Current LIR-to-BIR schema, construction, ownership, identity, side tables,
  consumers, mutation assumptions, and authority.
- Exact field-family classification and a non-authoritative
  `LegacyBirCompatibilityCapsule` contract.
- Stable IDs, mutation APIs, verifier, recomputable analyses, invalidation,
  and module/function ownership rules for pass-ready BIR.
- Source-cited comparison with the reference backend.
- Concrete target file/type/API blueprint and raw-to-canonical stage boundary.
- Ordered migration packets, proof boundaries, rollback points, compatibility
  deletion checkpoints, and later follow-up proposals.

## Non-Goals

- Do not implement schema, IDs, mutators, passes, analyses, verifiers,
  quarantine, preparation, or MIR changes.
- Do not delete or rewrite tests, expectations, unsupported markers,
  allowlists, runtime behavior, or supported semantics.
- Do not create follow-up source ideas during this runbook.
- Do not present unfinished 703--714 outcomes as accepted current behavior;
  label their influence as provisional.
- Do not classify regalloc, ABI/call planning, frame layout, or instruction
  selection as canonical BIR-to-BIR work without a distinct stage/output.

## Working Model

- Use AST-backed symbol and reference queries where practical, supplemented by
  focused source inspection.
- Trace each field family from construction through every direct consumer and
  mutation site.
- Treat terminators as the proposed CFG source of truth and analyses as
  recomputable, invalidatable results.
- Keep stable semantic IDs distinct from dense short-lived analysis indices.
- Keep the compatibility capsule observational, inaccessible to core passes,
  and governed by a shrinking reader/field allowlist with a final-zero gate.

## Execution Rules

- Create only the seven required files under `docs/backend/pass_ready_bir/`.
- Cite concrete files and symbols for factual claims.
- Give every current field/family exactly one target classification.
- Record unresolved human choices in the relevant answer and summarize them
  in `index.md`.
- Validate document shape and internal links after each step; perform a final
  exact-file-count and source-reference audit in Step 7.

## Step 1: Inventory Current LIR-to-BIR Schema

Goal: establish the exact present data model and authority map.

Primary target: `docs/backend/pass_ready_bir/01_current_lir_to_bir_schema.md`

Actions:

- Inventory `Module`, `Function`, `Block`, `Inst`, and `Terminator` fields and
  construction paths, including owned and referenced side tables.
- Trace semantic identities and conversions among IDs, pointers, names,
  vector positions, route indices, and prepared lookup keys.
- Map producers, consumers, mutation sites, and append-only, stability,
  ordering, or freeze assumptions.
- Demonstrate hazards for instruction insertion/removal/reorder, RAUW, block
  split, edge redirect, function movement, and module growth.

Completion check:

- The document accounts for every emitted field/family and supports authority
  and mutation claims with concrete source/symbol references.

## Step 2: Classify Fields And Define Legacy Quarantine

Goal: give every current family one future destination and define a measurable
legacy exit.

Primary target: `docs/backend/pass_ready_bir/02_field_classification_and_quarantine.md`

Actions:

- Classify each family exactly once as future core IR, recomputable analysis,
  lowering-only input, prepared/MIR output, or legacy compatibility/debug.
- State authority and lifetime rules for every class.
- Specify `LegacyBirCompatibilityCapsule`, its consumer allowlist, prohibited
  access from new core passes, and no-new-writer/consumer rules.
- Define machine-checkable monotonically decreasing field/reader checkpoints
  ending at zero fields and zero readers.

Completion check:

- No inventoried family is missing, duplicated across destinations, or left
  implicitly authoritative in legacy state.

## Step 3: Define The Pass-Ready BIR Contract

Goal: specify invariants and APIs required for safe composable mutation.

Primary target: `docs/backend/pass_ready_bir/03_pass_ready_bir_contract.md`

Actions:

- Define stable `FunctionId`, `BlockId`, `InstId`, and `ValueId` semantics,
  storage lifetime, handle validity, and iteration order.
- Sketch builders and mutators for insertion, replacement, erasure, traversal,
  RAUW, block split, and edge redirect.
- Define terminator-derived CFG, recomputable analyses, verifier invariants,
  and invalidation/preservation rules.
- Define safe per-function processing while `Module` owns cross-function
  symbols, types, globals, functions, and facts.

Completion check:

- Each mutation hazard from Step 1 is prevented or detected by an explicit
  invariant, API, verifier rule, or invalidation contract.

## Step 4: Compare The Reference Backend

Goal: adopt useful principles without copying its identity or invalidation
weaknesses.

Primary target: `docs/backend/pass_ready_bir/04_reference_backend_comparison.md`

Actions:

- Trace concrete reference symbols for module/function/block ownership,
  `BlockId`, values, `FlatAdj`, CFG analysis, pass execution, and phi removal.
- Explain its function/block-index analysis granularity before and after CFG
  construction.
- Record adopted principles: terminator-derived CFG, stable semantic IDs,
  dense ephemeral analysis indices, disposable contexts, and direct function
  transformations.
- Record rejected weaknesses: absent stable instruction identity, implicit or
  manual invalidation, leaked position identity, and target-local mutable state
  substituting for stage contracts.

Completion check:

- Every adopted or rejected principle is tied to both concrete reference code
  and the corresponding c4c requirement.

## Step 5: Produce The Target Schema And API Blueprint

Goal: define concrete files, types, storage, APIs, and stage ownership.

Primary target: `docs/backend/pass_ready_bir/05_target_schema_and_api_blueprint.md`

Actions:

- Propose the source-tree separation for core storage, mutation, verification,
  analyses, canonical passes, compatibility, preparation, and MIR lowering.
- Sketch stable arena/handle access, traversal, mutation, verification, and
  analysis cache/result APIs.
- Define `lower_lir_to_raw_bir -> canonical BIR pass pipeline -> verified
  CanonicalBir -> preparation/BIR-to-MIR`, using a wrapper/state token when
  duplicate full structures are unnecessary.
- Assign ABI, frame, allocation, call-move, and instruction-selection facts to
  explicit prepared/MIR outputs rather than core canonical BIR.

Completion check:

- The blueprint names implementable files/types/APIs and leaves no mixed-stage
  authority implicit.

## Step 6: Order Migration And Follow-Ups

Goal: turn the blueprint into reviewable, reversible implementation packets.

Primary target: `docs/backend/pass_ready_bir/06_staged_migration_and_followups.md`

Actions:

- Order schema isolation, stable IDs, mutation APIs, verifier, analysis
  boundary, and LIR-to-BIR construction through the new interface first.
- Order later legalization, CFG/SSA, memory/address, aggregate/intrinsic,
  out-of-SSA, allocation, ABI, and frame transitions by ownership boundary.
- Give each packet compatibility adapters, proof boundaries, rollback points,
  and capsule field/reader deletion checkpoints.
- Propose ordered follow-up implementation ideas without creating lifecycle
  files, beginning with a no-semantic-change packet.

Completion check:

- No packet requires a big-bang cutover, and every retained compatibility
  surface has a named later deletion checkpoint.

## Step 7: Integrate And Audit The Research Package

Goal: make the seven-file package internally consistent and acceptance-ready.

Primary target: `docs/backend/pass_ready_bir/index.md`

Actions:

- Link all six numbered answers and summarize the decision, invariants,
  migration order, provisional predecessor dependencies, and unresolved human
  choices.
- Confirm the directory contains exactly `index.md` plus the six required
  numbered answer files.
- Audit field classification completeness, source/symbol citations, authority
  consistency, capsule allowlist/deletion gates, proof boundaries, and links.
- Confirm the diff contains documentation only and no semantic, test,
  baseline, lifecycle-source, or implementation changes.

Completion check:

- All source-idea acceptance criteria are explicitly satisfied or the package
  records a precise unresolved blocker rather than asserting completion.
