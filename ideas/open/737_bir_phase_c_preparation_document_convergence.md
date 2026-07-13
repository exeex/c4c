# BIR Phase C Preparation Document Convergence

Status: Open
Type: Documentation-only architecture convergence
Phase Owner: C — target facts and immutable preparation
Predecessor: accepted `ideas/closed/736_bir_phase_b_canonical_document_convergence.md`
Successor: `ideas/open/738_bir_phase_d_pseudo_document_convergence.md`

## Goal

Converge the target/profile boundary, target-layout derivation, immutable
preparation chain, and constraint binding without mutating `CanonicalBir` or
inventing an implementation owner for the external C1 authority.

## Why This Exists

Phase C crosses from target-independent semantics into target-keyed immutable
products, so an unowned C1 seam or mixed product key would contaminate every
later pseudo, allocation, and MIR contract.

## Scope and Exact Owner Order

1. C1: audit the external `TargetProfile` selection/validation authority for
   requested triple, architecture, OS, ABI, relocation, float ABI and
   capabilities. Because the root has no linked C1 owner document, determine
   whether an explicit Markdown boundary/placeholder is required and where it
   is indexed; record it as external authority, never a BIR implementation.
2. C2: `target_layout/README.md`, deriving a verified exact-profile-keyed
   finite pseudo layout.
3. C3: `preparation/abi/README.md`.
4. C4: `preparation/calls/README.md`.
5. C5: `preparation/variadic/README.md`.
6. C6: `preparation/address/README.md`.
7. C7: `preparation/inline_asm/README.md`.
8. C8: `preparation/runtime_helpers/README.md`, followed by
   `preparation/README.md` as C8 sequencing and atomic cumulative-publication
   support. The shared preparation owner may be read earlier as context, but
   its owned review point is here after the six preparation products.
9. C9: `regalloc/constraints/README.md`, the sole parser/typer/binder and sole
   later-revision projection authority.
10. Audit relevant call-graph, provenance, publication/value-flow, analysis
   framework, verifier, diagnostics and root clauses at their earliest use.

## Uniform Contract and Matrix Method

Apply the umbrella metadata/core-first contract and per-document input/output
matrices. Each row must identify exact Canonical/profile/product/revision keys,
stable IDs, optional/error forms, producer and consumer clauses, verifier,
failure atomicity, invalidation, and checked implementation status. Analyses
are requested at earliest consumers. Every C2-C9 product is immutable; no
planner mutates Raw or Canonical storage. A missing real Markdown owner may be
created only as a documentation placeholder and indexed coherently.

## Target and Product Obligations

- `target_profile` and rendered `data_layout` are audited
  validation/origin/parity-only source context with no semantic Raw
  destination. C1 independently selects one exact `TargetProfile`, and C2
  derives layout facts rather than importing allocator state or parsing
  Raw/Canonical layout text.
- Product keys include the Canonical revision, exact target fingerprint,
  planner version/options, and dependency keys; mismatch/staleness rejects.
- C3-C8 publish one complete cumulative verified preparation bundle
  atomically. C7 supplies vocabulary/context only and does not parse or bind
  constraints. C9 alone creates Canonical-keyed `BoundConstraintSet` and owns
  exact-revision `ProjectedConstraintSet` transactions for later mutations.
- The downstream handoff is accepted `CanonicalBir` plus verified layout,
  cumulative preparation products, and typed constraints—never target facts
  written backward into Canonical storage.

## Non-Goals

C++/tests/build edits; choosing a target for the user; inventing C1
implementation; pseudo lowering, ABI call transport, allocation, MIR/emission;
or changing phase order.

## Acceptance and Closure Criteria

- C1 external authority and any required Markdown boundary are explicitly
  classified with no duplicate BIR owner; C2 and C3-C9 have exact unique
  ownership and ordering.
- All input/output matrices prove target/profile/revision binding, immutable
  publication, invalidation, verifier, failure and D1 consumer acceptance.
- The accepted handoff to D is a verified preparation/constraint bundle tied
  to accepted Canonical input and exact target key.
- Closure records C1 boundary disposition, owner inventory/placeholders,
  matrices, implementation-truth findings, and exact B/C and C/D adjacency.

## Reviewer Reject Signals

- Code/test/build edits, implementation invention for C1, duplicate target or
  constraint authority, mutation of Raw/Canonical, or phase mixing.
- Parsing `data_layout` to manufacture semantics; C7 parsing constraints; a
  planner publishing partial/mixed-key products; or stale projection reuse.
- Assertion-only handoff, heading/status-only changes, or unchecked claims
  that scaffolded planners exist.
- Named-case matchers, rendered-output probes, allowlists, unsupported/status
  downgrades, expectation weakening, helper renames, or classification-only
  changes claimed as capability.
- The old missing-target/product seam retained under a renamed abstraction.
