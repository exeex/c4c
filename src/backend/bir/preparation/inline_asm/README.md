# C7 Immutable Inline-Assembly Target Context Product Contract

Contract-Status: converged planned contract under idea 732
Implementation-Status: absent
Phase-ID: C7
Upstream: exact Canonical/C1/C2/C3/C4/C5/C6 tuple
Downstream: immutable `InlineAsmTargetContext` for C8-C9

## Purpose

C7 publishes only target constraint vocabulary, class/group eligibility,
clobber vocabulary, and contextual rule versions. Assembly and constraint bytes
remain opaque; C7 neither parses nor binds them.

## Owns

Versioned target constraint/clobber vocabulary and eligibility tables, context
validation, fingerprint, and immutable publication.

## Does Not Own

C7 does not parse constraint strings or templates, bind operands, infer ties,
choose registers, mutate inline-asm nodes, lower calls, allocate, or assemble.

## Inputs

The unchanged Canonical owner and exact C1-C6 tuple, including C2 class/group
domain and complete predecessor fingerprints.

## Input NodeKind/Tag Vocabulary

All five Canonical groups are retained by reference. Only
`B.CanonicalOpaqueTargetToken` and declared ordinary operand types/sites are
context consumers; opaque bytes are never semantic input to C7 derivation.

## Required Analyses and Products

C2 and C3-C6 products only. No parser result or bound constraint product may
exist yet; those belong to C9.

## Ordered Behavior

1. Validate the exact tuple and target vocabulary registry version.
2. Derive finite constraint/class/group/clobber/context tables in registry order.
3. Validate all references against C2 and target capabilities.
4. Publish one immutable context atomically without inspecting asm bytes.

## NodeKind/Tag Lowering Matrix

| Canonical input subset | Reference outcome | C7 product outcome | Node tags added/removed | Identity | Failure |
|---|---|---|---|---|---|
| opaque inline-asm token | retain exact node/bytes | target vocabulary/context eligibility only | none | unchanged | bytes are not parsed/bound |
| ordinary inline-asm operand/result type/site | retain reference | eligible class/group vocabulary references | none | unchanged | unsupported type is explicit failure/absence |
| non-asm value/effect/control/phi | retain reference | explicit no-context row | none | unchanged | inferred constraint forbidden |
| unknown/illegal/omitted/later-stage kind | reject product | none | none | none | `InlineAsmContextVocabularyInvalid` |

## Identity and Provenance

Context rows cite exact Canonical site IDs only for coverage. Original bytes
remain payload; vocabulary spelling is registry identity, not node identity.

## Outputs

One complete immutable `InlineAsmTargetContext` keyed to the common tuple, with
finite vocabularies/tables, registry versions, coverage, and fingerprint.

## Verification and Publication

Validate exact keys, unique vocabulary IDs, C2 references, target capabilities,
and total declared site coverage. Reject any parsed/bound operand, selected
register, graph edit, or modified opaque byte. Publish all-or-nothing.

## Analysis Preservation and Invalidation

Any Canonical/target/C2-C6/vocabulary-registry/site change invalidates C7 and
C8-C9. Equal spellings under another registry/version cannot be relabelled.

## Failure and Diagnostics

Stale/mixed keys, invalid vocabulary reference, unsupported context,
cancellation, or resource failure publishes no partial table or parser result.

## Adjacent-Stage Contract

C6 supplies the exact tuple. C8 carries this context unchanged; C9 alone parses,
types, and binds original constraint bytes against ordinary operands/results.

## Implementation State

Absent. Existing target asm tables/parsers do not implement this keyed context.

## Proof Requirements

Prove supported/unsupported vocabulary, exact C2 references, opaque-byte
preservation, no parsing/binding/register choice, key rejection, and atomicity.

## Open Questions

New constraint/clobber vocabulary requires a versioned C7 registry update.
