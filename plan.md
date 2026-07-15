# BIR Exceptional Control, Allocation, And Frame Design Completion Runbook

Status: Active
Source Idea: ideas/open/803_bir_exceptional_control_allocation_and_frame_design_completion.md

## Purpose

Complete six hard BIR backend design contracts in Markdown while preserving
the accepted A1-F3 phase spine and strict apply-only F1 boundary.

## Goal

Leave exceptional control flow, instruction-point SSA visibility, bounded
promotion, allocation policy, frame layout, and over-aligned objects with
single normative owner chains that later implementation can follow without
inventing ownership, repair, retry, or verifier behavior.

## Core Rule

This runbook is documentation-only. Modify only BIR or strictly necessary
adjacent Markdown. Proposed types, APIs, and algorithms belong only in fenced
code blocks and must be labeled as documentation sketches, never as landed
capability.

## Read First

- `ideas/open/803_bir_exceptional_control_allocation_and_frame_design_completion.md`
- `src/backend/bir/README.md`
- `src/backend/bir/LEGACY_COVERAGE.md`
- affected B3-B5, C6, D4, E1-E4, F1, analysis, verifier, MIR, ABI, and target
  Markdown owners
- `ref/claudes-c-compiler/` only as read-only behavioral evidence

## Scope

- Normative placement/conflict matrix for all six design areas.
- End-to-end Markdown contracts for non-local return and `asm goto`.
- One deterministic B4 phi-growth/promotion policy.
- Versioned E1/E2 allocation facts and decision policy.
- Closed subordinate E4 frame-object and packing contract.
- Complete B5 -> C6 -> D4 -> E1/E2 -> E4 -> F1 over-aligned-object route.
- Cross-document reconciliation, truthful implementation status, and
  Markdown-only structural proof.

## Non-Goals

- No code, headers, tests, build files, scripts, generated files, binaries,
  canonical regression logs, or implementation claims.
- No A1-F3 reorder, new major phase, second CFG/graph/allocation/frame
  authority, or weakened F1 boundary without a proven irreducible conflict.
- No redesign of shared BIR storage, stable identity, NodeKind/tag algebra, or
  unrelated optimization and target-tuning policy.
- No routine follow-up ideas; only a concrete irreducible phase contradiction
  may produce a separately scoped architecture question.

## Working Model

- Preserve B3 CFG topology, B4 SSA, B5 memory/effects, E1
  liveness/interference, E2 allocation choice, E3 explicit rewrite, E4
  frame/final publication, and F1 apply-only ownership.
- Preparation and analysis products are immutable and exact-revision keyed.
- Mutations happen in private candidates and publish only after verification.
- One-to-many target realization and every allocatable temporary are explicit
  before E1.
- Correctness constraints are normative; tunable profitability policy is
  versioned and deterministic.

## Execution Rules

- Audit the authoritative owner before editing a consumer; link to one owner
  instead of duplicating independent rules.
- For each design area, state inputs/products, mutation owner, downstream
  consumer, verifier/publication gate, invalidation, failure, and adjacency.
- Preserve exact edge occurrence, stable identity/provenance, exact revision,
  and failure-atomicity contracts.
- Keep implementation status factual and label proposed surfaces explicitly.
- Before accepting each step, inspect changed paths and run focused Markdown,
  link, ownership, and contradiction checks appropriate to that packet.
- Update `todo.md` after each accepted step; do not rewrite this runbook for
  routine packet progress.

## Step 1 — Establish the placement and conflict baseline

### Goal

Map every requirement to its exact A-F owner chain before local contract work.

### Actions

- Inventory the root and subordinate owners for B3-B5, C6, D4, E1-E4, F1,
  relevant analyses/verifiers, and adjacent MIR/target boundaries.
- Build the normative placement/conflict matrix with first owner, upstream
  facts and keys, mutation owner, consumer, gate, invalidation, failure, and
  adjacency rationale for every requirement.
- Record existing gaps and contradictions without changing the A1-F3 order.
- If a requirement cannot fit the accepted spine, stop only that portion and
  document the exact smallest irreducible architecture question.

### Completion Check

- Every requirement in all six areas has one provisional owner chain.
- Every affected Markdown owner is inventoried and conflicts are explicit.
- The changed-path set is permitted Markdown only.

## Step 2 — Define non-local control-transfer safety

### Goal

Specify `setjmp`, `longjmp`, `returns_twice`, and equivalent behavior from B4
through E4 verification without inventing a second CFG.

### Actions

- Define B4 promotion and definition visibility, B5 memory observability and
  ordering, and the roles of `volatile`, address escape, and memory identity.
- Define immutable exceptional-boundary facts, E1 liveness/clobber/reload
  exposure, E2 assignment constraints, E3 rewrite placement, and bounded retry.
- Define E4 memory-home/frame obligations and stage-local fail-closed verifier
  diagnostics for stale values, unsafe assignments, and missing homes.
- Trace at least one complete non-local-return scenario through the owner chain.

### Completion Check

- Stale register-only state after non-local return is structurally forbidden.
- Topology, semantic effect, allocation, spill, and frame responsibilities are
  explicit and adjacent-compatible.

## Step 3 — Define `asm goto` instruction-point SSA snapshots

### Goal

Separate B3 edge topology from B4 value visibility at the exact instruction
point, including duplicate edge occurrences and fallthrough outputs.

### Actions

- Define B3 fallthrough/label edge occurrences and B4 definition-stack
  snapshots at the `asm goto` program point.
- Specify label versus fallthrough visibility for zero, one, and multiple
  targets, outputs, clobbers, and critical-edge normalization.
- Key phi inputs by exact edge occurrence and prohibit later/block-end
  definitions from leaking backward.
- Define revision, ambiguity, missing-snapshot, and illegal-input verifier
  failures, then trace one end-to-end scenario.

### Completion Check

- Every successor sees exactly the permitted instruction-point state.
- No rule relies on rendered labels, block names, or ambiguous block-end state.

## Step 4 — Resolve bounded B4 promotion planning

### Goal

Choose and fully specify one deterministic pre-mutation phi-growth and
promotion policy.

### Actions

- Evaluate fail-closed complete promotion against deterministic partial
  promotion under the accepted B4/B5 vocabulary.
- Select one policy and record the rationale in the normative B4 owner and
  root placement matrix.
- Define versioned metrics, checked arithmetic, hard bounds, stable ordering,
  diagnostics, identity behavior, private-candidate rollback, and B5 admission.
- Forbid mid-pass fallback, incomplete selected promotion, and success after
  resource or construction failure.

### Completion Check

- Identical graph and policy version produce the same plan.
- Every admitted selected value is completely promoted or the candidate is
  discarded; any retained memory form is explicitly admitted and verified.

## Step 5 — Define allocation facts and deterministic choice policy

### Goal

Give E1 and E2 a versioned division between immutable cost facts and bounded,
deterministic allocation/coalescing/eviction decisions.

### Actions

- Define E1 facts for loop/profile weight, use/def frequency, range shape,
  pressure, fixed homes, call crossing, rematerialization, copies, and spills.
- Define E2 correctness constraints, profitability weights, coalescing,
  assignment, victim selection, eviction, progress, and stable tie breaks.
- Bind policy versions to exact-revision product keys and invalidation.
- Require E3 to realize the selected rewrite without re-deciding policy, and
  keep `E3 -> E1` as the bounded retry route.

### Completion Check

- Pointer/hash/incidental traversal order cannot affect a decision.
- Correctness constraints, tuning inputs, chosen decisions, and E3 realization
  are owned and verifiable at their proper stages.

## Step 6 — Define the E4 frame-object and packing contract

### Goal

Create or converge one subordinate E4 authority with a closed object taxonomy
and explicit stack-slot sharing legality.

### Actions

- Cover addressable/fixed locals, spills, copy scratch, ABI areas, callee-save
  and bookkeeping, fixed/dynamic/runtime-aligned objects, and explicitly
  introduced emergency/address scratch.
- For each class define size/alignment, lifetime source, interference, escape,
  fixed placement, sharing prohibitions/permissions, ordering, reachability,
  deterministic packing, bounds, and verifier evidence.
- Define when locals, spills, ABI areas, and scratch may share storage using
  proved non-interference and compatible semantics, never coincident offsets.
- Trace one representative frame construction and publication scenario.

### Completion Check

- Every admitted frame object belongs to exactly one governed class.
- E4 privately constructs and verifies the frame; no consumer repairs layout.

## Step 7 — Complete over-aligned object realization

### Goal

Close the continuous B5 -> C6 -> D4 -> E1/E2 -> E4 -> F1 contract for fixed
and dynamic over-aligned objects.

### Actions

- Preserve semantic extent, lifetime, observability, and alignment in B5.
- Define C6 target-keyed natural alignment, realignment, legal address forms,
  offset ranges, base requirements, and dynamic-stack facts without mutation.
- Require D4 to expose all align/mask/round, large-offset, address calculation,
  one-to-many operations, and allocatable temporaries before E1.
- Define allocation, E4 placement/base/restore behavior, unwind/non-local
  interaction, and target rejection; keep F1 strictly apply-only.
- Trace fixed, VLA/runtime-aligned, and unencodable-offset scenarios.

### Completion Check

- No hidden temporary, expansion, frame action, or repair occurs after
  allocation or inside F1.
- Unsupported target realizations fail at their documented gate.

## Step 8 — Reconcile all owners and prove documentation closure

### Goal

Make the six owner chains mutually consistent and decide whether idea 803 is
complete or has proven one minimal architecture conflict.

### Actions

- Reconcile the root placement matrix, normative owners, linked consumers,
  implementation status, exact-revision keys, identity, failure atomicity,
  retry boundaries, and F1 handoff.
- Trace one normative end-to-end scenario for each design area.
- Audit stale contradictory ownership with focused searches and verify the
  unchanged A1-F3 table and strict F1 boundary.
- Check all changed paths, Markdown links/paths, fenced sketch labeling, and
  `git diff --check`.
- Record the chosen phi policy, every changed/added owner, all six chains,
  conflict-audit result, and deliberately unresolved tuning before requesting
  an explicit closure decision.

### Completion Check

- Every source acceptance criterion and reject signal is accounted for.
- The diff contains only permitted Markdown and no implementation claim.
- No follow-up idea exists unless a concrete irreducible spine conflict was
  proven and bounded.
