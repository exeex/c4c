# BIR B-F Pass Documentation Convergence Runbook

Status: Active
Source Idea: ideas/open/732_bir_stage_document_convergence_umbrella.md

## Purpose

Converge the Markdown architecture from factual Raw-BIR publication through
every B/C/D/E/F pass, using the landed shared Node/identity model and the
746/801/802 NodeKind/tag contract as fixed inputs.

## Goal

Leave every ordered B-F pass with an agent-executable Markdown contract for
input/output vocabulary, tag lowering, identity, verification, dependencies,
invalidation, failure, and adjacency, then close only after reconciling those
contracts with the landed LIR-to-BIR Raw publication behavior.

## Core Rule

This runbook is Markdown-only. Proposed C++, schemas, and APIs may appear only
inside fenced code blocks in Markdown. Never modify implementation, tests,
headers, CMake/build files, scripts, generated sources, logs, or other
non-Markdown artifacts.

## Read First

- `ideas/open/732_bir_stage_document_convergence_umbrella.md`
- `ideas/closed/746_bir_node_kind_centric_storage_pass_contract.md`
- `ideas/closed/801_bir_node_kind_tag_algebra_and_phase_vocabulary_lowering_contract.md`
- `ideas/closed/802_project_wide_cpp20_host_toolchain_contract.md`
- `docs/backend/bir_node_kind_tag_algebra_and_phase_vocabulary.md`
- `src/backend/bir/README.md`
- `src/backend/bir/core/README.md`
- current `src/backend/bir/core/ir.hpp` only as read-only interface evidence
- current LIR-to-BIR code/tests only as read-only Raw-publication evidence

## Scope

- Markdown owners for B1-B8, C1-C9, D1-D5, E1-E4, and F1-F3.
- BIR root, phase, pass, analysis, verifier, schema, and boundary Markdown
  needed to make those pass contracts adjacent-compatible.
- A final factual audit of the landed importer and Raw/B1 seam.
- Missing-owner Markdown placeholders when a real authority is absent.

## Non-Goals

- No production, test, build, script, generated, or log changes.
- No importer or LIR modification.
- No redesign of shared Node storage, stable identity, NodeKind schema, tag
  algebra, query helpers, or phase vocabulary.
- No implementation-completeness claim based on a proposed code block.
- No downstream implementation activation.

## Working Model

- Shared graph storage is common across phases; admitted NodeKind vocabulary is
  not inherited implicitly.
- The 801 contract is normative. Pass documents reference it and add only
  pass-specific retain/lower/reject decisions.
- `SsaEligible` is static classification. B4 proves dynamic SSA; D5 owns its
  required removal.
- Analyses and C-phase preparation are immutable exact-revision products, not
  semantic graph mutations.
- Every mutating pass builds a private candidate and publishes only after its
  verifier gate succeeds.
- The importer is the factual Raw producer. Until its parallel route lands,
  record observations as provisional evidence and reserve final reconciliation
  for Step 8.

## Execution Rules

- Before and after every step, verify the changed-path set contains only
  Markdown.
- Follow `src/backend/bir/README.md` order; do not silently reorder rows.
- Give every pass the per-pass section spine required by the source idea.
- Use explicit, closed NodeKind/tag lowering rows; do not use an unbounded
  “everything else passes through” rule.
- Label every fenced API/schema example as proposed or documentation-only.
- State implementation status from repository evidence; use `absent` or
  `partial` when appropriate.
- Record cross-phase disagreements at the narrowest Markdown owner. A required
  code change is a separate idea, not a 732 packet.
- Update `todo.md` after each accepted step; do not rewrite this runbook for
  routine progress.

## Step 1 — Audit current Markdown owners and factual inputs

### Goal

Establish the current documentation and implementation-truth baseline without
changing any code.

### Actions

- Inventory every `src/backend/bir/**/*.md` file and map each to root phase,
  pass, analysis, verifier, schema, boundary, support, or audit ownership.
- Inspect the landed shared-node/identity and NodeKind helper surfaces only as
  needed to identify facts that documents may claim.
- Record the current LIR-to-BIR branch/revision evidence available locally and
  distinguish landed Raw behavior from parallel work not yet present.
- Identify stale phase order, duplicate authority, missing owners, speculative
  APIs, and implementation-status drift.
- Publish the audit in Markdown and state which Raw facts remain provisional
  until Step 8.

### Completion Check

- Every current BIR Markdown file is accounted for.
- The audit names authoritative inputs, current implementation truth, missing
  owners, and the deferred importer-finalization evidence.
- The diff contains Markdown only.

## Step 2 — Converge the root vocabulary and common pass contract

### Goal

Make the root and shared documentation consume the landed Node/NodeKind
contracts and provide one uniform authoring spine for all B-F pass owners.

### Actions

- Reconcile `src/backend/bir/README.md`, core documentation, pass framework,
  pipeline, verifier, and analysis framework ownership with the 801 contract.
- Define stage-qualified vocabulary admission, dynamic SSA distinction,
  identity preservation/replacement, private-candidate publication, exact
  revision/product keys, and fail-closed unknown-kind handling once.
- Define the required per-pass lowering matrix and implementation-status
  language without duplicating the NodeKind registry or tag algebra.
- Correct stale top-level implementation claims while preserving normative
  B-F order.

### Completion Check

- Root/common documents form one non-duplicated authority spine.
- A later pass document can reference exact shared identity, tag, verifier,
  publication, and invalidation rules.
- No second NodeKind/tag authority or non-Markdown change exists.

## Step 3 — Plan phase B and Canonical publication

### Goal

Complete B1/P01 through B8 contracts from factual Raw admission to verified
Canonical publication.

### Actions

- Review and edit B1 legalize, B2 scalar, B3 CFG, B4 SSA, B5 memory, B6
  aggregate, B7 intrinsics, and B8 pipeline/verifier owners in order.
- For every pass, enumerate input/output NodeKind/tag vocabulary and explicit
  retain/replace/expand/merge/delete/reject behavior.
- Bind CFG, dominance, effects, provenance, call graph, comparison,
  publication/value-flow, and related analyses to their earliest consumers
  and invalidation points.
- Make B4 the sole dynamic SSA establishment/proof point and define exact phi
  admission without confusing it with `SsaEligible`.
- Prove B8 forbids target, allocation, pseudo, frame, and machine vocabulary.

### Completion Check

- B1-B8 each satisfy the per-pass Markdown contract.
- Raw-to-Canonical vocabulary and every adjacency are closed and explicit.
- Canonical publication is target-independent, unallocated, dynamically SSA
  valid, and fail-closed.

## Step 4 — Plan phase C immutable preparation

### Goal

Complete C1-C9 contracts without mutating Canonical BIR or inventing a second
graph authority.

### Actions

- Plan TargetProfile validation, target layout, ABI, calls, variadic, address,
  inline-asm context, runtime helpers, and constraint binding in order.
- Define exact Canonical revision and target/profile keys for every immutable
  product, cumulative bundle, and projected constraint product.
- Distinguish static NodeKind admission from product-based D1 readiness.
- Define stale-result rejection, invalidation, diagnostics, rollback, and the
  precise D1 admission seam.

### Completion Check

- C1-C9 each satisfy the per-pass/product-owner Markdown contract.
- No C owner silently mutates Canonical nodes or claims a new published graph
  stage without the normative 801 transition.
- D1 can consume one exact verified Canonical revision and matching products.

## Step 5 — Plan phase D pseudo lowering and out-of-SSA

### Goal

Complete D1-D5 contracts for Canonical/Prepared to verified, directly
realizable pre-allocation pseudo vocabulary.

### Actions

- Plan generic pseudo lowering, shared ABI-aware call lowering, Pseudo
  publication, target legalization/expansion, and out-of-SSA in order.
- Enumerate every retained/replaced input family and introduced pseudo family,
  including exact projected-constraint revision updates.
- Make all one-to-many target expansion occur before allocation.
- Define D5's dynamic SSA removal, edge-copy/parallel-copy semantics, scratch
  reservations, stable identity/provenance, and later copy-resolution owner.
- Prove every introduced value, definition, use, clobber, and constraint enters
  E1/E2 normally.

### Completion Check

- D1-D5 each satisfy the per-pass Markdown contract.
- No Canonical-only, unresolved expansion, ABI ambiguity, or unowned SSA form
  reaches E1.
- Pseudo publication and later private candidate gates are distinct and
  failure-atomic.

## Step 6 — Plan phase E allocation and MIR-ready publication

### Goal

Complete E1-E4 contracts from allocation facts through exact-revision
MIR-ready capability publication.

### Actions

- Plan liveness/interference, shared allocation, spill/reload insertion, retry,
  copy closure, frame actions, final recomputation, and E4 verification.
- Enumerate allocation/spill/frame NodeKind/tag vocabulary and forbidden
  residual kinds at each private/public gate.
- Define `E3 -> E1` invalidation/retry, termination/failure behavior, and exact
  current-product rejection.
- Apply the identity gate to inserted spill/reload, copy, and frame nodes.
- Prove E4 publishes no hidden expansion, unresolved copy, pressure deficit,
  stale product, or unrepresented frame work.

### Completion Check

- E1-E4 each satisfy the per-pass Markdown contract.
- Analysis products, graph revisions, private assigned candidates, and public
  Allocated/MIR-ready capabilities are unambiguous.
- The E3 retry and E4 atomic publication contracts are complete.

## Step 7 — Plan phase F machine construction and emission

### Goal

Complete F1-F3 contracts for the final vocabulary transition and terminal
consumers.

### Actions

- Plan strict apply-only machine construction, machine verification, and
  assembly/object/relocation/link emission boundaries.
- Enumerate the MIR-ready pseudo-to-machine mapping contract and exact
  rejection of anything requiring expansion, allocation, spilling, ABI work,
  or frame repair.
- State that machine identity is distinct and BIR IDs survive only as explicit
  provenance.
- Define opaque inline-assembly transport and late parsing without hidden
  value/allocation authority.
- Identify every F3 terminal output and its consumer/failure behavior.

### Completion Check

- F1-F3 each satisfy the per-pass Markdown contract.
- Every admitted MIR-ready kind has an explicit machine disposition and every
  machine/emission output has an owner.
- F cannot repair an earlier-stage failure.

## Step 8 — Audit landed importer and close the cross-phase contract

### Goal

Replace provisional Raw assumptions with landed importer evidence and prove
the complete Raw-to-F3 Markdown architecture is internally consistent.

### Actions

- Wait until the parallel LIR-to-BIR route has landed at an identifiable
  revision; do not infer its final behavior from intent.
- Audit its actual Raw kinds, payload alternatives, operand/result roles,
  types, CFG/terminators, stable source identities, metadata, verifier gate,
  rejection behavior, and publication atomicity.
- Reconcile A1/A2 and B1 admission documents with those facts.
- Audit every current BIR Markdown owner for unique indexing, truthful
  implementation status, vocabulary/identity adjacency, exact keys,
  invalidation, verifier ownership, and failure atomicity.
- Record remaining implementation gaps as truthful `absent`/`partial` state or
  separately scoped open ideas; do not modify code.
- Verify the complete accepted diff contains Markdown only and request
  plan-owner closure judgment.

### Completion Check

- The landed importer revision and factual Raw publication matrix are named.
- Every B-F producer/consumer seam accepts the exact predecessor artifact.
- No speculative implementation claim, duplicate authority, stale product,
  implicit kind carry-forward, or unresolved identity rule remains.
- Markdown-only scope is proven and all source acceptance criteria are met.
