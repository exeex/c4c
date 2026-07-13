# P06 Aggregate Canonicalization Pass Contract

Status: closed architecture contract. Implementation is deferred until the BIR
architecture is accepted.

`B6 / P06` is the aggregate-canonicalization module transformation. It consumes
the exact committed B5 / P05 output and publishes unique target-independent
aggregate semantics without recreating a form owned by P01-P05.

## 1. Exact input and canonical form

The input is one immutable whole-module candidate stamped with the exact
`ModuleEpoch`, `ModuleRevision`, ordered function-revision digest, plan
fingerprint, and cumulative properties through `MemoryCanonical`. P05
postconditions, the canonical CFG/SSA profiles, core def-use, and the configured
input verifier must accept that same candidate.

Canonical v1 has one typed form for aggregate values and copies, insert/extract
operations, layout-independent field/index paths, semantic complex/multivalue
operations, and by-value call/return boundaries. Paths retain semantic type and
field/index identity and preserve P05 address/GEP forms. They do not encode a
storage decomposition or calling-convention realization. The exact output is
the only input admitted by P07.

This layout-independent typed path is the complete v1 canonical disposition
for every admitted array, struct, union, complex, multivalue, and by-value
case. If semantic type plus field/index identity cannot represent a legacy
case losslessly without physical layout or ABI placement, that case is not an
alternate canonical form: import or P06 rejects it with the stable unsupported
diagnostic.

## 2. Closed P06 authority

P06 owns deterministic normalization of every `aggregate`-owned Raw form in the
pipeline first-owner map. A closed disposition table assigns each admitted
descriptor one canonical rewrite or a stable unsupported diagnostic. Opaque or
incomplete records cannot be inspected; incompatible copies, paths, or
declaration/definition boundaries fail closed rather than guessing from source
spelling or legacy records.

P06 may repair typed uses caused by its own replacements, but cannot alter CFG
topology, invent phi inputs, change memory effects, expand an aggregate into a
fresh earlier-stage scalar/memory operation, select a helper, or interpret
inline-assembly text. `InlineAsm` remains one ordinary-value opaque semantic
node; an aggregate-typed operand or result remains an ordinary typed edge.

## 3. Transaction, preservation, and failure

`PassId::AggregateCanonicalize` is a `Module` pass requiring
`MemoryCanonical`, establishing `AggregatesCanonical`, and using
`RepeatContract::Idempotent`. It inventories all aggregate boundaries in
canonical order, builds one closed whole-module plan, reserves IDs, applies the
complete edit set to a private occurrence candidate, derives its mutation
summary, invalidates all analyses observing affected types, values, calls, or
bodies, and runs every cumulative P01-P06 postcondition plus verifier-on-commit.

Success atomically publishes one immutable B6 / P06 candidate that preserves
all P01-P05 properties. A true no-op keeps every revision. Stale input/analysis,
unowned or unsupported semantics, incomplete cross-declaration repair,
recreation of an earlier noncanonical form, deterministic resource exhaustion,
cancellation, merge conflict, or verifier rejection rolls back the complete
occurrence. No partial edit, revision, property, cache entry, or stage token is
published, and P07 does not run.
