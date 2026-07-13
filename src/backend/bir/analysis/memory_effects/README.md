# Memory and Effect Analysis Contract

Status: closed architecture contract. Implementation is deferred until the BIR
architecture is accepted.

`MemoryEffects` is an immutable target-independent function analysis. It
classifies semantic accesses and effects without becoming a second memory graph
or pass authority.

## 1. Exact descriptor and key

```text
id: AnalysisId::MemoryEffects
schema_version: 1
scope: Function
domain: CanonicalSemantic
dependencies: { AnalysisId::Cfg schema 1 }
observes_module_tables: true
observes_function_bodies: true
observes_target_layout: false
observes_preparation_facts: false
```

The complete key is schema version 1, `ModuleEpoch`, exact `ModuleRevision`,
`FunctionId`, exact `FunctionRevision`, dependency key, and the canonical empty
v1 options fingerprint. Module tables are observed for globals, declarations,
intrinsic registry entries, and declared call effects. Computation reads one
frozen view; publication is atomic only while every key axis still matches.
Checked handles reject stale dereference and never retarget themselves.

## 2. Closed immutable result

For each instruction and call site the result records an ordered set of typed
effects: read, write, read-write, allocation/lifetime, volatility, atomic
ordering/scope, fence/barrier, escape, call, and `Unknown`. Facts name stable BIR
IDs, semantic objects/address spaces when proven, and exact CFG occurrences for
path-sensitive summaries. `Unknown` is conservative and never authorizes a
rewrite. Function summaries are deterministic joins of those facts.

The result contains no edit callback, mutable pointer, alias assertion stored as
truth, legacy access view, prepared lookup, route record, storage decision, or
machine fact. Calls use typed callee/bundle/effect descriptors; `InlineAsm` is
one ordinary-value opaque semantic node and contributes only its declared
conservative semantic effects. Neither assembly nor constraint text is
interpreted.

## 3. Invalidation, consumers, and failure

Any observed operand/result, instruction, memory descriptor, object/address,
atomic, call/intrinsic effect, inline-assembly payload/effect, terminator/CFG,
declaration, global, registry, or body revision change invalidates the result.
Cross-revision reuse requires a registered validator to prove the complete fact
set unchanged and publish a new immutable result under the new key; old handles
remain stale. No-change reuse requires unchanged revisions.

P05-P07 and provenance may query the exact-revision result, but cannot delegate
mutation, canonical publication, or support policy to it. Malformed semantics,
wrong profile, stale view/dependency, unsupported schema/options, deterministic
resource exhaustion, or cancellation fails atomically. No partial cache entry is
published and consumers may not fall back to an older result.
