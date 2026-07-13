# P07 Intrinsic Canonicalization Pass Contract

Status: closed architecture contract. Implementation is deferred until the BIR
architecture is accepted.

`P07` is the `S08` module transformation and the last canonicalizer. It consumes
the exact committed P06 output and produces the sole candidate admitted by the
G01 Canonical publication gate.

## 1. Exact input and final canonical form

The input is one immutable whole-module candidate stamped with the exact
`ModuleEpoch`, `ModuleRevision`, ordered function-revision digest, plan
fingerprint, and cumulative properties through `AggregatesCanonical`. Every
P01-P06 postcondition, core def-use, and the configured input verifier must
accept that same candidate.

Canonical v1 has a closed intrinsic registry. Each entry fixes semantic identity,
signature, generic operands/results, immediates, portable feature requirements,
and conservative effects. Runtime-helper-eligible operations retain semantic
identity only. Unsupported or unrepresentable semantics receive a stable
fail-closed diagnostic.

`InlineAsm` remains exactly one ordinary-value opaque semantic node. Its inputs
and results live only in the generic def-use graph; its original template and
constraint payload remain opaque and lossless; ordered clobber spellings and
declared side effects remain semantic payload. P07 does not interpret, rewrite,
or bind either text and does not manufacture a parallel asm instruction graph.

## 2. Closed P07 authority

P07 owns deterministic normalization of every `intrinsics`-owned form in the
pipeline first-owner map, including registry/signature/effect reconciliation,
semantic intrinsic aliases, atomic forms assigned to this family, and the
closed inline-assembly payload shape. It may repair typed uses caused directly
by its own replacements.

P07 cannot recreate a P01-P06 noncanonical scalar, edge, phi, address, memory,
or aggregate form; choose a runtime helper; decide whether a portable feature is
available on a particular machine; or perform instruction, storage, calling, or
constraint realization. Missing registry ownership, unknown semantics, or an
operation requiring any such decision fails the occurrence.

## 3. Transaction and exact P07 output

`PassId::IntrinsicCanonicalize` is a `Module` pass requiring
`AggregatesCanonical`, establishing `IntrinsicsCanonical`, and using
`RepeatContract::Idempotent`. It constructs one deterministic whole-module
plan, applies it to a private occurrence candidate, derives the authoritative
mutation summary, invalidates every affected call-graph, memory-effect,
provenance, publication/value-flow, and dependent result, and runs all
cumulative P01-P07 properties plus verifier-on-commit.

Success freezes one immutable S08 candidate with its exact full stage stamp.
That unforgeable stamp records P07 as ordinal 7 under the canonical-v1 plan and
is consumed only by G01; a separately assembled or merely similar candidate is
not P07 output. A true no-op preserves revisions while still establishing the
P07 property on the same occurrence lineage.

Stale input/analysis, registry inconsistency, unsupported semantics, incomplete
cross-module repair, recreation of an earlier form, deterministic resource
exhaustion, cancellation, merge conflict, or verifier rejection rolls back the
complete occurrence. No partial edit, revision, property, cache entry, frozen
P07 candidate, or publication capability escapes, and G01 does not run.
