# Call Graph and Call-Semantics Analysis Contract

Status: closed architecture contract. Implementation is deferred until the BIR
architecture is accepted.

`CallGraph` is an immutable target-independent module analysis. It derives
semantic call topology and visibility from typed declarations and bodies; it
does not own call transformation or calling-convention realization.

## 1. Exact descriptor and key

```text
id: AnalysisId::CallGraph
schema_version: 1
scope: Module
domain: CanonicalSemantic
dependencies: {}
observes_module_tables: true
observes_function_bodies: true
observes_target_layout: false
observes_preparation_facts: false
```

The complete key is schema version 1, `ModuleEpoch`, exact `ModuleRevision`, the
ordered digest of every `(FunctionId, FunctionRevision)`, and the canonical
empty v1 options fingerprint. Module revision alone never establishes freshness
when a body is observed. Computation reads one frozen module view and publishes
atomically only if all axes still match. Checked handles reject stale
dereference and never retarget themselves.

## 2. Closed immutable result

The result contains canonical-order nodes for definitions, declarations, and an
explicit external/unknown callee class; call-site facts keyed by `InstId`; and
typed edges for direct, finite-set indirect when proven, and unknown indirect
calls. It also contains deterministic SCC/recursion classification, callers and
callees, address-taken/external visibility, and declared semantic call effects.
Parallel call sites remain distinct even when they connect the same functions.

The graph never invents a callee from a name, renderer text, provenance hint, or
legacy call-chain lookup. `InlineAsm` remains one ordinary-value opaque semantic
node, not a call edge. The result contains no edit callback, stage capability,
prepared lookup, route record, calling location, move, storage, allocation, or
machine fact.

## 3. Invalidation, consumers, and failure

Any observed callee operand, call/intrinsic descriptor or effect, function
signature/body, declaration/definition, symbol/linkage/visibility,
address-taken use, inline-assembly semantic edge, or module/function revision
change invalidates the result. Cross-revision preservation requires a registered
validator to prove the complete graph unchanged and install a new immutable
result under the new key; old handles remain stale. No-change reuse requires an
unchanged complete key.

P05-P07 and later read-only planning may query the exact-revision result but
cannot delegate mutation, canonical publication, or support policy to it. A
wrong profile, malformed typed call, stale view, unsupported schema/options,
nonconvergence, deterministic resource exhaustion, or cancellation fails
atomically. Failure publishes no partial graph and consumers may not fall back
to an older result or legacy lookup.
