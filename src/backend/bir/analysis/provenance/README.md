# Provenance Analysis Contract

Status: closed architecture contract. Implementation is deferred until the BIR
architecture is accepted.

`Provenance` is an immutable target-independent function analysis. It explains
typed value and address origin from semantic BIR; it never makes raw pointers,
producer snapshots, or route records authoritative.

## 1. Exact descriptor and key

```text
id: AnalysisId::Provenance
schema_version: 1
scope: Function
domain: CanonicalSemantic
dependencies: { AnalysisId::Cfg schema 1,
                AnalysisId::Dominance schema 1,
                AnalysisId::MemoryEffects schema 1 }
observes_module_tables: true
observes_function_bodies: true
observes_target_layout: false
observes_preparation_facts: false
```

The complete key is schema version 1, `ModuleEpoch`, exact `ModuleRevision`,
`FunctionId`, exact `FunctionRevision`, all dependency keys, and the canonical
empty v1 options fingerprint. Module revision covers symbols, globals,
constants, and types. Computation reads one frozen view and publishes atomically
only if every revision and dependency still matches. Checked handles reject
stale dereference and never rebind.

## 2. Closed immutable result

Each fact is keyed by stable BIR IDs and classifies a value/address as an exact
semantic object or symbol origin, a derived path/offset from another fact, a phi
join retaining exact `EdgeKey` occurrences, an escaped origin, or `Unknown`.
Derivation follows only registered transparent carriers, typed GEP/address
operations, phi inputs, calls with declared semantics, and memory-effect facts.
Cycles reach a deterministic fixed point; ambiguity joins conservatively.

The result contains no owning pointer, edit capability, traversal coordinate,
legacy freshness record, prepared lookup, route snapshot, storage realization,
or machine fact. It cannot legalize an invalid pointer operation, repair core
def-use, prove aliasing from spelling, or override typed object identity.
`InlineAsm` remains one ordinary-value opaque semantic node: ordinary inputs may
flow to `Unknown` outputs/effects according to its declaration, but its text is
never inspected.

## 3. Invalidation, consumers, and failure

Any observed definition/use, operand, phi incoming, CFG occurrence, semantic
address/path, memory effect, call/intrinsic descriptor, object/symbol/type,
inline-assembly semantic edge, or body revision change invalidates the result.
Cross-revision preservation requires proof of the complete unchanged fact set
and installation under a new exact key; old handles remain stale. No-change
reuse requires unchanged revisions.

Canonical passes and later read-only planning may query this result but cannot
delegate mutation, verification, canonical publication, or support policy to
it. A wrong profile, malformed core fact, stale view/dependency, unsupported
schema/options, nonconvergence, deterministic resource exhaustion, or
cancellation fails atomically and publishes no partial cache entry.
