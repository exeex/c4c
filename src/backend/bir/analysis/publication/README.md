# Publication and Value-Flow Analysis Contract

Status: closed architecture contract. Implementation is deferred until the BIR
architecture is accepted.

`PublicationValueFlow` is an immutable target-independent function analysis.
It describes where semantic values are produced, transported, consumed,
returned, or cross typed call boundaries. Its historical name does not grant
stage-token or graph-publication authority: core def-use, typed calls/returns,
and terminators remain the source semantics.

## 1. Exact descriptor and key

```text
id: AnalysisId::PublicationValueFlow
schema_version: 1
scope: Function
domain: CanonicalSemantic
dependencies: { AnalysisId::Cfg schema 1, AnalysisId::Dominance schema 1 }
observes_module_tables: true
observes_function_bodies: true
observes_target_layout: false
observes_preparation_facts: false
```

The complete key is schema version 1, `ModuleEpoch`, exact `ModuleRevision`,
`FunctionId`, exact `FunctionRevision`, and the canonical empty v1 options
fingerprint. Module revision is observed for declarations, signatures,
symbols, and constants. Both dependency handles must match the same body
revision. Computation reads a frozen view and publishes atomically only if all
key axes still match. Checked handles reject stale dereference and never
rebind.

## 2. Closed immutable output

Facts are keyed only by stable BIR IDs and exact edge occurrences:

- each `ValueId` definition and its ordinary typed uses;
- formal parameters and their function-entry values;
- return operands and their typed result positions;
- call operands/results mapped to semantic parameter/result positions and the
  declared call boundary, without ABI locations;
- phi/block-argument transport as `{EdgeKey, incoming ValueId, destination
  value}` facts, retaining parallel edges;
- explicit producer/consumer chains through registered transparent semantic
  carriers, plus `Unknown` when no stronger valid classification is proven;
- deterministic special-carrier discovery from typed opcodes and operands.

Collections use canonical module/function/block/instruction/operand/edge order.
The result contains no mutable pointer, edit callback, stage capability,
prepared lookup, route record, traversal coordinate, target layout, calling
location, constraint realization, physical register, spill slot, or allocation
fact. It never overrides core def-use or manufactures a call/return relation
from names or text.

## 3. Invalidation, consumers, and failure

Any observed change to a definition, use, operand/result, phi incoming,
terminator slot, block membership/order, call bundle/effects, signature,
declaration, return, special carrier, or dependency invalidates the result.
Cross-revision preservation requires a registered validator to prove the
complete fact set unchanged and install a new immutable result under the new
key; old handles remain stale. No-change reuse requires unchanged revisions.

P04 and later semantic planning may query the exact-revision result but cannot
delegate mutation, SSA repair, canonical publication, preparation, or
allocation decisions to it. A wrong profile, malformed core semantics, stale
view/dependency, unsupported schema/options, deterministic resource
exhaustion, or cancellation fails atomically. Failure publishes no partial
facts, and consumers may not fall back to legacy tables or an earlier result.
