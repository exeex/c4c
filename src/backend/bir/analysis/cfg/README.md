# CFG Analysis Contract

Status: closed architecture contract. Implementation is deferred until the BIR
architecture is accepted.

CFG analysis is a target-independent function analysis available before B3 /
P03 planning and recomputed from the resulting terminators for B4 / P04 and
later consumers. It derives all edges from typed terminators and their ordered
successor slots, never mutates BIR, and never publishes a second stored graph.

## 1. Exact descriptor and revision key

```text
id: AnalysisId::Cfg
schema_version: 1
scope: Function
domain: CanonicalSemantic
dependencies: {}
observes_module_tables: false
observes_function_bodies: true
observes_target_layout: false
observes_preparation_facts: false
```

The complete key is schema version 1, `ModuleEpoch`, `FunctionId`, exact
`FunctionRevision`, and the analysis-options fingerprint. V1 has no semantic
option, so that fingerprint is the registry's canonical empty-options value.
The result may be requested for a structurally verified Raw or later view.
Computation reads one frozen body; a revision change before atomic cache
publication discards the candidate. Checked handles reject dereference against
another key with `StaleAnalysis` and never retarget themselves.

## 2. Immutable result

For every terminator successor occurrence the result contains:

```text
EdgeKey { source BlockId, successor-slot ordinal }
EdgeFact { EdgeKey key, BlockId destination, typed edge kind }
```

It also contains successor occurrences per source, incoming occurrences per
destination, entry reachability, unreachable blocks, deterministic reverse
postorder, and analysis-local dense indices. Incoming and successor
collections retain distinct keys when several slots reach the same block.
Externally visible collections use canonical stable-ID/slot order.

Dense indices, worklists, pointers, labels, layout positions, renderer text,
legacy route snapshots, and prepared branch records cannot escape the result
or act as graph identity. The terminator remains the only stored edge source;
analysis facts are immutable observations bound to one revision.

## 3. Invalidation and failures

Any change to a block, block order/membership, terminator opcode, successor
slot/order/destination, asm-goto topology, or entry block invalidates the
result. An instruction-only edit may preserve it only through the framework's
registered mutation validator. Cross-revision reuse installs a fresh immutable
result under the new key; old handles remain stale. No-change reuse is allowed
only when the function revision is unchanged.

A request fails for malformed terminators, foreign blocks, an invalid slot,
wrong input profile, stale view, unsupported schema/options, deterministic
resource exhaustion, or cancellation. Failure publishes no partial cache
entry. Consumers may not recover unavailable facts from names, text, layout,
or an earlier revision.
