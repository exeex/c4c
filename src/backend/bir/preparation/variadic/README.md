# C5 Immutable Variadic Requirement Product Contract

Contract-Status: converged planned contract under idea 732
Implementation-Status: absent
Phase-ID: C5
Upstream: exact Canonical/C1/C2/C3/C4 tuple
Downstream: immutable `VariadicPlan` for C6-C9 and D2

## Purpose

C5 derives typed variadic entry, call, promotion, traversal, and save-area
requirements without emitting a save area, changing calls, or mutating BIR.

## Owns

Versioned variadic-rule selection, total variadic-site requirement rows,
validation, coverage, fingerprinting, and atomic product publication.

## Does Not Own

C5 does not insert promotions, save-area objects/actions, va-list traversal
nodes, call transport, frame state, locations, pseudos, or graph revisions.

## Inputs

The unchanged Canonical owner and exact C1/C2/C3/C4 products with complete
common keys and predecessor fingerprints.

## Input NodeKind/Tag Vocabulary

All five Canonical groups by immutable reference. Variadic call/operation
queries identify sites; no preparation or transport tag is attached to nodes.

## Required Analyses and Products

C3 `AbiPlan` and C4 `CallPlan` are required. No new semantic analysis is
needed; missing variadic semantics in predecessors is failure.

## Ordered Behavior

1. Validate the entire exact predecessor tuple.
2. Select one variadic registry row from target/ABI identity.
3. Derive entry/call/promotion/traversal/save-area requirements in stable order.
4. Validate total mutually consistent coverage and publish atomically.

## NodeKind/Tag Lowering Matrix

| Canonical input subset | Reference outcome | Variadic product outcome | Node tags added/removed | Identity | Failure |
|---|---|---|---|---|---|
| variadic function entry/signature | retain reference | typed entry/save-area/traversal requirements only | none | unchanged | unsupported signature rejects |
| variadic call | retain call node | typed promoted-argument and transport requirements consistent with C3/C4 | none | unchanged | disagreement rejects |
| canonical va-start/end/copy/arg semantic operation | retain semantic node | exact operation/traversal requirement, no expansion | none | unchanged | incomplete type/state rejects |
| non-variadic call/value/effect/control/phi | retain reference | explicit no-requirement row | none | unchanged | inferred variadic behavior forbidden |
| opaque token/inline asm | retain reference | no variadic row unless explicitly typed upstream | none | unchanged | text inference forbidden |
| unknown/illegal/omitted/later-stage kind | reject product | none | none | none | `VariadicVocabularyInvalid` |

## Identity and Provenance

Requirements cite exact Canonical function/call/value IDs. Save-area and
transport requirements are product facts, not nodes, locations, or identities.

## Outputs

One immutable total `VariadicPlan` keyed to the unchanged tuple, with registry
version, per-site requirements/no-requirement rows, coverage, and fingerprint.

## Verification and Publication

Validate exact keys, C3/C4 agreement, promotions/types, complete site coverage,
and absence of graph edits, emitted save areas, locations, frame state, or
pseudos. Publish once or not at all.

## Analysis Preservation and Invalidation

Any Canonical/target/layout/ABI/call/variadic-registry or observed-site change
invalidates C5 and successors. Products cannot float across equal signatures.

## Failure and Diagnostics

Stale/mixed keys, unsupported variadic form, predecessor conflict, incomplete
coverage, cancellation, or resource failure publishes no partial/default plan.

## Adjacent-Stage Contract

C4 supplies exact call requirements. C6 consumes the unchanged tuple plus this
fingerprint. D2 later materializes transport; C5 emits no save area or node.

## Implementation State

Absent. Legacy variadic preparation/emission does not implement this product.

## Proof Requirements

Prove variadic/non-variadic entries/calls/operations, exact promotions,
predecessor agreement, stale/mixed rejection, no save-area emission, and atomicity.

## Open Questions

New variadic forms require a versioned registry row, never a fallback ABI.
