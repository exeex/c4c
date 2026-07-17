# C6 Immutable Address Requirement Product Contract

Contract-Status: converged planned contract under idea 732
Implementation-Status: absent
Phase-ID: C6
Upstream: exact Canonical/C1/C2/C3/C4/C5 tuple
Downstream: immutable `AddressPlan` for C7-C9 and D/E

## Purpose

C6 derives target-aware address-materialization and relocation requirements for
canonical sites without choosing address modes, rewriting GEPs, or mutating BIR.

## Owns

Versioned address-rule selection, total address/object/site requirements,
relocation requirement identity, validation, fingerprinting, and publication.

## Does Not Own

C6 does not lower addresses, fold target offsets, choose instructions, assign
registers/frame locations, mutate memory nodes, emit relocations, or build MIR.

## Inputs

The unchanged Canonical owner plus exact C1 fingerprint and C2-C5 products,
including every common predecessor/schema/coverage fingerprint.

## Input NodeKind/Tag Vocabulary

All five Canonical groups by immutable reference. Address/memory/provenance
queries identify declared sites; they do not add a target/address-mode tag.

## Required Analyses and Products

Fresh exact-Canonical `Provenance`, C2 layout, and C3-C5 cumulative products.
Unknown provenance is explicit failure/absence under a rule, never numeric or
spelling inference.

## Ordered Behavior

1. Validate the complete unchanged tuple and Provenance key.
2. Select one versioned address/relocation requirement registry.
3. Derive every object/path/site requirement in stable-ID order.
4. Validate total coverage and atomically publish one immutable product.

## NodeKind/Tag Lowering Matrix

| Canonical input subset | Reference outcome | Address product outcome | Node tags added/removed | Identity | Failure |
|---|---|---|---|---|---|
| address/GEP-producing value | retain reference | exact base/path/space/materialization requirement | none | unchanged | missing provenance/path rejects |
| memory access/effect | retain reference | exact addressability/relocation/access requirement | none | unchanged | incompatible type/address space rejects |
| global/symbol/object reference | retain reference | stable relocation/object requirement identity | none | unchanged | spelling/numeric fallback forbidden |
| call/aggregate/control/phi | retain reference | only explicitly declared address requirement | none | unchanged | inferred address mode forbidden |
| opaque token/inline asm | retain reference | ordinary typed address operands only; bytes stay opaque | none | unchanged | text parsing forbidden |
| unknown/illegal/omitted/later-stage kind | reject product | none | none | none | `AddressVocabularyInvalid` |

## Identity and Provenance

Address product rows cite exact Canonical node/value/object IDs. Provenance is
an immutable fact, not permission to reuse identity or construct numeric
pointers.

## Outputs

One total immutable `AddressPlan` with exact common key, rule/analysis versions,
per-site requirements, coverage digest, and fingerprint; no graph or mode choice.

## Verification and Publication

Validate all keys, total site/path/object coverage, relocation identity, C2-C5
agreement, and absence of graph edits, selected modes/opcodes, locations, frame,
or machine facts. Publish all-or-nothing.

## Analysis Preservation and Invalidation

Any Canonical/target/layout/predecessor/Provenance/registry/site change
invalidates C6 and all successors; equal-looking requirements cannot be rekeyed.

## Failure and Diagnostics

Stale/mixed products, unknown provenance, invalid path/space, unsupported rule,
cancellation, or resource failure publishes no partial/fallback plan.

## Adjacent-Stage Contract

C5 supplies the exact tuple. C7 consumes the unchanged tuple plus this exact
fingerprint. D owns address-mode/pseudo lowering; C6 never performs it.

## Implementation State

Absent. Existing GEP/import/legacy address lowering is not this product.

## Proof Requirements

Prove address/object/path/relocation families, unknown provenance, stale/mixed
keys, total coverage, atomicity, and no address-mode lowering.

## Open Questions

New relocation/materialization requirements need versioned explicit rows.
