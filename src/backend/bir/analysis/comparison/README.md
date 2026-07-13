# Comparison and Select Analysis Contract

Status: closed architecture contract. Implementation is deferred until the BIR
architecture is accepted.

`ComparisonSelect` is a target-independent `CanonicalSemantic` analysis used
by B2 / P02 and later semantic consumers. It derives relationships from typed
instructions, values, stable IDs, and ordinary def-use. It is not a pipeline
stage and never mutates or publishes BIR.

## 1. Exact input key

The v1 descriptor is closed as follows:

```text
id: AnalysisId::ComparisonSelect
schema_version: 1
scope: Function
domain: CanonicalSemantic
dependencies: {}
observes_module_tables: true
observes_function_bodies: true
observes_target_layout: false
observes_preparation_facts: false
```

The analysis accepts a verified Raw, `TypesLegal`, or later canonical-semantic
view. A function result is keyed by the complete framework key:

```text
AnalysisId::ComparisonSelect
schema version 1
ModuleEpoch + ModuleRevision
FunctionId + FunctionRevision
analysis-options fingerprint
```

The module revision is required because comparison and cast interpretation can
observe module-owned type and constant tables. The options fingerprint covers
the v1 `max_transparent_chain_depth` bound; no other semantic option exists.
Worker count, cache state, pointer values, renderer settings, and environment
state are excluded. This canonical-domain key has no target-layout or
preparation component.

Computation reads one frozen view. If any key component changes before atomic
cache publication, the candidate result is discarded. A checked handle retains
the complete key; dereference against another revision returns
`StaleAnalysis` and never rebinds the handle.

## 2. Closed immutable output

The result contains only immutable, revision-bound facts keyed by stable BIR
IDs:

- each compare result's `InstId`, result `ValueId`, predicate/domain, and
  ordered operand `ValueId`s;
- typed cast and boolean-normalization producers on a condition's transparent
  producer chain;
- every ordinary consumer, classified as conditional branch, select,
  boolean/scalar operation, phi, call/return transport, store, or other use;
- each select's condition, true value, false value, result, and direct nesting
  relationships;
- deterministic select-chain membership and root/leaf relationships;
- condition equivalence or inversion only when a closed semantic rule proves
  it from canonical predicates and operands;
- explicit `Unknown` classifications where a valid BIR form has no stronger
  fact. Unknown is data, not permission to guess.

Externally visible collections are ordered by canonical module/function/block/
instruction/operand order. Dense indices and worklists may exist inside one
result but cannot escape it or become identity. The result contains no pointers
into mutable storage, callbacks, edit capabilities, rendered instruction text,
legacy route records, selected instruction information, calling locations,
register identities, allocation facts, or persistent rewrite decisions.

## 3. Dependency and invalidation contract

Core def-use is an input invariant, not a separate analysis dependency. The v1
descriptor has no analysis dependencies; relationships are derived directly
from the frozen function view and module-owned type/constant tables.

The result is invalidated by any change to an observed instruction opcode,
predicate, operand, result, value type, constant, cast, select, phi incoming,
use list, block membership/order, terminator condition/successor, or observed
module type/constant table.

Preservation across a new revision is allowed only when the pass framework's
registered mutation-specific validator proves semantic equality of the entire
result and installs a new immutable result under the new complete key. Old
handles remain stale. An empty `MutationSummary` may reuse the existing handle
because the revision did not change.

## 4. Reject and failure behavior

Registry construction rejects duplicate/unknown IDs, a schema mismatch,
undeclared inputs or dependencies, a dependency cycle, or any target-bound
input in this analysis domain. A request fails with a structured error for a
wrong input profile, stale view or dependency, malformed core def-use,
unsupported schema/options, deterministic resource exhaustion, or
cancellation.

Failure and cancellation publish no partial result. Valid but non-provable
relationships produce explicit `Unknown` facts; malformed input never does.
Consumers must treat a missing/stale result as unavailable analysis, not
reconstruct it from text or retain facts from an earlier revision.

## 5. Consumer boundary

B2 / P02 may use these facts to plan its own closed transactional rewrites, but
the analysis cannot prescribe operand swaps, folds, fusion, or pass order.
Later select/condition consumers may query the same schema only through a
handle for their exact input revision. Any target-bound consumer must compute
its own later-domain facts rather than adding target information to this
result.
