# P05 Memory Canonicalization Pass Contract

Status: closed architecture contract. Implementation is deferred until the BIR
architecture is accepted.

`B5 / P05` is the memory-canonicalization module transformation. It consumes
the exact committed B4 / P04 output and publishes one target-independent memory
profile while preserving the canonical CFG and SSA profiles.

## 1. Exact input and output

The input is one immutable whole-module candidate stamped with the exact
`ModuleEpoch`, `ModuleRevision`, ordered function-revision digest, pipeline-plan
fingerprint, and cumulative properties through `SsaCanonical`. P04
postconditions, core def-use, exact `EdgeKey` phi coverage, and the configured
input verifier must accept that same candidate. Stale analysis handles or a
candidate assembled from different revisions reject the occurrence.

The output has one typed representation for local/global/TLS/label addresses,
semantic pointer offset/GEP paths, loads, stores, memory copies/sets, dynamic
allocation and stack-state operations, atomics, fences, volatility, alignment,
ordering, scope, and explicit effect operands. Object and address-space identity
remain semantic BIR facts; freshness, alias, provenance, and effect conclusions
remain recomputable analyses. The output preserves the P03 CFG and P04 SSA
profiles exactly or fails, and is the only input admitted by P06.

## 2. Closed P05 authority

P05 owns deterministic normalization of every `memory`-owned Raw form in the
pipeline first-owner map. Its closed disposition table assigns each admitted
descriptor exactly one canonical rewrite or a stable unsupported diagnostic.
Unknown descriptors, incomplete object identity, or semantics that cannot be
represented losslessly fail closed. P05 never guesses from names, rendered
text, legacy route records, or prepared side data.

P05 may repair typed uses caused directly by its own replacement, but cannot
change successor topology, invent phi inputs, recreate a P01-P04 form, decompose
an aggregate, select a helper, interpret inline-assembly text, or decide storage
realization. `InlineAsm` remains one ordinary-value opaque semantic node whose
generic operands/results and effect summary are merely observed.

## 3. Analyses and invalidation

Planning may request immutable exact-revision CFG, dominance, publication/value-
flow, memory-effects, and provenance handles. These are observations, not edit
capabilities. P05 does not write derived conclusions into core BIR or patch an
analysis result after mutation.

P05 owns normalization of semantic effect operands only. Reconciliation of a
derived whole-module effect summary is analysis-only authority of the
revision-bound memory-effects analysis. It cannot mutate the candidate, become
a stored P05 postcondition, or independently justify this pass's module scope;
the module barrier remains necessary for global-object and descriptor
consistency.

Committed edits invalidate every result observing an affected operand, object,
address, access, atomic, call effect, definition/use, or body revision, including
dependent results transitively. Preservation is valid only through the pass
framework's complete mutation validator and installation under the new key;
old handles remain stale.

## 4. Transaction, output, and failure

`PassId::MemoryCanonicalize` is a `Module` pass requiring `SsaCanonical`,
establishing `MemoryCanonical`, and using `RepeatContract::Idempotent`. It builds
one deterministic whole-module plan, reserves all IDs, applies all function and
module edits to one private occurrence candidate, derives the authoritative
mutation summary, and runs cumulative CFG/SSA/memory postconditions plus
verifier-on-commit. A true no-op retains every revision.

Success publishes one immutable B5 / P05 candidate and its property atomically.
Stale input, an unowned form, lossless-representation failure, incomplete use or
effect repair, deterministic resource exhaustion, cancellation, merge conflict,
or verifier rejection rolls back the entire occurrence. No partial function,
revision, property, analysis cache entry, or stage token is published, and P06
does not run.
