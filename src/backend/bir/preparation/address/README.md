# Address Plan

Status: converged design contract (unimplemented).

## Contract

Address planning is `C6`. It consumes the exact `VerifiedPreparationInput`
borrow, matching `VerifiedTargetLayout`, and the published ABI, call, and
variadic products. It selects a reviewed abstract address-materialization
strategy for each Canonical address/object/relocation semantic identity while
preserving that identity and meaning.

The immutable `AddressPlan` may record base/index eligibility, scale and
displacement domains, relocation kind requirements, dynamic-stack/object
requirements, and whether a later closed pseudo expansion is needed. It does
not construct target instructions, choose concrete registers, allocate general
values, establish frame offsets, or turn an analysis conclusion into semantic
BIR truth. D1 materializes the admitted generic pseudo form and D4 performs
every required one-to-many target expansion before allocation. F1 may only map
the resulting directly realizable node one-to-one; it cannot reopen an address
strategy as late instruction selection or repair.

After D5 resolution, the E4-owned `FrameRealizationTransaction` consumes the
exact C6 strategy lineage and resolved allocation products to choose exact
frame-object bases, offsets, and displacements. If a dynamic-frame interaction
or displacement would require address materialization or more than the one
already admitted record, it fails atomically before publication; F1 cannot
choose an alternate address sequence.

## Binding and consumers

The product key contains the complete Canonical `PipelineStageStamp`, exact
`TargetFingerprint`, layout and address schema fingerprints, and exact ordered
ABI/call/variadic product fingerprints. Inline-assembly preparation is the
immediate consumer; D1 pseudo lowering and D4 target legalization are later
consumers. E4/F1 retain only exact product lineage and the already-realized
pseudo node.

## Publication

All address identities are planned in one transaction. Unsupported relocation
or object forms, missing provenance required by a strategy, impossible
width/alignment, incomplete identity coverage, stale analysis, predecessor/key
mismatch, or diagnostics publish no `AddressPlan`. Inputs remain unchanged.

Any change to the Canonical stage stamp, target fingerprint, layout/address
schema, an ABI/call/variadic predecessor fingerprint, or a named
address/object/relocation identity invalidates the complete plan and all C7-C9
successors.

Legacy coverage: addressing, local/global/static object access, dynamic stack,
pointer carriers, decoded storage, and target relocation requirements.
