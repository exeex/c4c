# Dominance Analysis Contract

Status: closed architecture contract. Implementation is deferred until the BIR
architecture is accepted.

Dominance is an immutable target-independent function analysis over one exact
CFG result. It is not a serial pipeline stage and has no mutation authority.

## 1. Descriptor, dependency, and key

```text
id: AnalysisId::Dominance
schema_version: 1
scope: Function
domain: CanonicalSemantic
dependencies: { AnalysisId::Cfg schema 1 }
observes_module_tables: false
observes_function_bodies: true
observes_target_layout: false
observes_preparation_facts: false
```

Its complete key is schema version 1, `ModuleEpoch`, `FunctionId`, exact
`FunctionRevision`, and the options fingerprint. The v1 option selects the
closed product `Dominators` or `DominatorsAndPostdominators`; dominance
frontiers are included with dominators. The dependency handle must carry the
identical epoch/function/revision and its own exact schema/options key. A stale
or mismatched CFG handle fails the request rather than being silently rebuilt
under an old dominance request.

## 2. Immutable result

The result contains the reachable set; immediate dominator tree; deterministic
tree children and preorder intervals; dominance frontiers; and, when selected,
the postdominator forest with explicit treatment of exits and non-terminating
regions. Queries cover block dominance and instruction/value dominance using
stable `BlockId`, `InstId`, `ValueId`, and core instruction order. Parallel
`EdgeKey`s remain visible when frontiers are derived; they are never collapsed
into a stored replacement graph.

Unreachable blocks have explicit `Unreachable` results rather than guessed
dominators. Dense algorithm indices and traversal worklists remain private.
The result contains no edit plan, phi insertion, block ordering directive,
target information, allocation fact, mutable pointer, or legacy publication
record.

## 3. Invalidation and failure behavior

Invalidation follows the exact CFG dependency and every observed instruction
order, definition, and use needed by instruction/value queries. A terminator or
successor-slot edit therefore invalidates both CFG and dominance. A new
revision may preserve dominance only when registered validators prove the
complete selected product unchanged and publish a new immutable result under
the new key. Existing handles always remain bound to their original revision.

Malformed core def-use/order, a stale view/dependency, wrong profile,
unsupported schema/options, deterministic resource exhaustion, or cancellation
fails atomically and publishes no partial result. Consumers such as P04 must
treat failure as unavailable analysis, never infer dominance from block order,
names, renderer output, or prior revisions.
