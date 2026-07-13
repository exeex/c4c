# Revision-Bound Dominance Analysis Contract

Contract-Status: under-review
Implementation-Status: absent
Kind: analysis
Applies-To: B4 / P04 earliest consumer and later exact-CFG semantic queries
Upstream: exact immutable B3 `CfgCanonical` revision plus freshly recomputed `Cfg`
Downstream: `PublicationValueFlow` dependency and B4 / P04 planning
Owner-Path: `src/backend/bir/analysis/dominance/README.md`
Last-Reconciled-Commit: `5eb4d6f43`

## Purpose

`Dominance` is an immutable target-independent function analysis over the
freshly recomputed exact-B3 CFG. It supplies block, instruction and value
dominance plus frontier facts immediately before P04. It is not a pipeline
stage, graph authority, phi-placement mutation or verifier capability.

## Owns

- closed `AnalysisId::Dominance`, schema version 1, function-scope traits;
- its complete revision/dependency/options key and checked handle;
- reachable dominator tree, deterministic children/intervals, exact-edge
  dominance frontiers and optional post-dominance product;
- stable-ID queries, semantic equality, failure, invalidation and checked
  new-key installation.

## Does Not Own

- CFG edges/terminators, core def-use/order, revisions or stable IDs;
- phi placement, SSA construction, block mutation or pass scheduling;
- dense indices, pointers, names, vector positions, layout or rendered text as
  identity or proof;
- target/profile/layout, ABI/helper, preparation, allocation, MIR or emission.

## Inputs

One frozen immutable B3 function view with `RawVerified`, `TypesLegal`,
`ScalarsCanonical` and `CfgCanonical` is required, together with the freshly
recomputed `AnalysisId::Cfg` handle for that exact revision. The P03
postcondition and configured verifier accept the same view.

### Exact descriptor and input-key matrix

| Input/key axis | Exact required value | Optional/empty form | Failure / forbidden substitution |
|---|---|---|---|
| descriptor | `AnalysisId::Dominance`, schema 1, `Function`, `CanonicalSemantic` | none | unknown/duplicate ID/schema is `DominanceRegistryInvalid` |
| stage/property | immutable B3 checkpoint with cumulative properties through `CfgCanonical` | declaration/empty body remains a valid B3 function | wrong/missing capability is `DominanceWrongInput` |
| module key | exact `ModuleEpoch` plus checkpoint `ModuleRevision` | no module table is observed; revision binds the accepted checkpoint | stale/foreign module is `StaleAnalysis` |
| function key | exact `FunctionId` plus `FunctionRevision` | empty body retains the exact key | stale/foreign function is `StaleAnalysis` |
| CFG dependency | schema-1 `AnalysisId::Cfg` handle at the identical epoch/module/function revision | empty CFG is a complete dependency | missing/stale/mismatched key is `DominanceStaleDependency` |
| dependency key | ordered singleton `{AnalysisId::Cfg, schema 1, complete key}` | none | undeclared/cyclic/additional dependency is `DominanceRegistryInvalid` |
| options key | schema-bound closed `Dominators` or `DominatorsAndPostdominators` product | P04 requests `Dominators`; post-dominance is optional | unsupported option is `DominanceOptionsInvalid` |
| semantic input | exact CFG stable block/`EdgeKey` facts plus core instruction/definition/use order | declaration/empty body yields the complete empty result | malformed order/def-use/CFG is `DominanceInputInvalid` |
| target/preparation axes | target-layout key `None`; preparation digest empty | none | later-domain input is `DominanceForbiddenInput` |

The cache key contains descriptor/schema/domain, exact epoch/module/function
revisions, the complete CFG dependency key, selected product fingerprint, no
target-layout key and an empty preparation digest. Dependency and result keys
must match; the manager never silently rebuilds CFG under an old request.

## Outputs

One immutable result uses stable BIR IDs and exact edge occurrences. Dense
algorithm numbering is reversible result-local storage only.

### Exhaustive dominance result-family matrix

| Result family | Exact stable facts | Optional/non-provable form | Consumer and invalidation |
|---|---|---|---|
| reachable membership | exact CFG-reachable `BlockId` set | declaration/empty body is known empty | P04 admission; CFG dependency change invalidates |
| immediate dominator | reachable non-entry `BlockId -> BlockId` | entry has `Absent` parent; unreachable is explicit `Unreachable` | P04 renaming/placement; edge/block change invalidates |
| dominator tree order | deterministic stable-ID children and preorder/postorder intervals | empty body yields empty products | P04 traversal; CFG or order-query input change invalidates |
| block dominance query | `Known(true/false)` for two reachable stable block IDs | either unreachable yields `Unreachable`, never a guess | P04 placement; CFG change invalidates |
| instruction dominance query | `Known(true/false)` from block facts plus exact core instruction order | absent entity is query error, not false | P04 use verification; order/def/use change invalidates |
| value-on-edge dominance | `Known(true/false)` at exact `EdgeKey` occurrence | unreachable occurrence is `Unreachable` | P04 phi incoming selection; edge/definition change invalidates |
| dominance frontier | exact destination `BlockId` plus contributing `EdgeKey` occurrences | empty frontier is known empty; parallel occurrences remain distinct | P04 phi placement; edge multiplicity change invalidates |
| post-dominator forest | selected exits, roots, forest and non-terminating regions | `Absent` when option is `Dominators`; `NoExit` is explicit | later selected consumers; option/CFG change invalidates |
| semantic equality | all selected stable-ID/edge products and query semantics | no equality inferred across keys | framework preservation only; never pass authority |

All valid registered CFGs have decidable dominance. The result uses
`Known`, `Absent`, `Unreachable` and `NoExit` as defined above; it does not turn
malformed order, stale CFG or unsupported products into `Unknown`.

## Adjacent-Stage Contract

[CFG analysis](../cfg/README.md) supplies the mandatory freshly recomputed
exact-B3 dependency. `Dominance` is then requested for the exact same B3 view.
[Publication/value-flow](../publication/README.md) consumes both exact-current
handles, and [P04 SSA](../../passes/ssa/README.md) is the earliest mutation
consumer. Neither analysis may place a phi or authorize a rewrite.

## Ordered Behavior

1. Validate descriptor/domain, exact B3 key, selected product, CFG dependency
   equality and target/preparation exclusion.
2. Freeze the function, CFG facts and exact core order/def-use inputs.
3. Compute reachable dominators in deterministic stable-ID/edge order, then
   children, intervals and exact occurrence-aware frontiers.
4. If selected, compute post-dominator forest with explicit exits and
   non-terminating regions.
5. Validate complete stable-ID coverage and semantic query invariants.
6. Publish atomically only while the full result/dependency keys remain current.

## Invariants

- CFG is a dependency, not duplicated stored graph authority.
- Parallel `EdgeKey` occurrences remain visible in frontier and edge queries;
  predecessor-block sets cannot collapse them.
- Instruction/value queries use stable IDs and core order, never dense index,
  pointer, name, renderer text or layout.
- Unreachable blocks have explicit state, not guessed roots or dominators.
- Facts contain no edit plan, phi, property, stage token or target fact.

## Failure and Diagnostics

Stable failures are `DominanceRegistryInvalid`, `DominanceWrongInput`,
`DominanceStaleDependency`, `StaleAnalysis`, `DominanceOptionsInvalid`,
`DominanceInputInvalid`, `DominanceForbiddenInput`, deterministic resource
exhaustion and cancellation. Diagnostics contain the full result/dependency
keys and stable block/instruction/value/edge anchor in deterministic order.

Failure publishes no partial tree/frontier/query/handle/cache entry. A key
mismatch returns `StaleAnalysis`; an old handle never retargets or recomputes.

## Analysis and Invalidation

Any CFG dependency invalidation transitively invalidates dominance. Changes to
block/instruction order, definition, result, use or phi incoming also invalidate
queries observing them. After a revision increment every old handle is stale.
Checked preservation may install a complete immutable result under the new key
only when CFG is preserved/recomputed at that key and registered validators
prove every selected product semantically equal. It never rebinds an old handle.

## Target and ABI Rules

`Dominance` is `CanonicalSemantic` and has no target/preparation key. It rejects
profiles/layout, ABI/helper selection, constraints, homes, spills, frames, MIR,
renderer state and environment features.

## Implementation State

Implementation is absent. No checked-in descriptor, algorithm/result schema,
dependency route, cache/invalidation validator, build target or runtime proof
for `AnalysisId::Dominance` exists.

## Proof Requirements

- prove complete key/dependency/options equality and stale rejection;
- cover empty/unreachable/no-exit forms, exact parallel-edge frontiers and
  stable block/instruction/value/edge queries;
- prove atomic failure, transitive invalidation and checked new-key preservation;
- prove exact-B3 CFG dependency and P04 earliest use;
- reconcile implementation/build truth.

## Open Questions

None for v1. New products require a schema/options revision.

## Review Checklist

- [x] Exact B3 checkpoint and freshly recomputed CFG dependency required.
- [x] Stable-ID and parallel-edge products are exhaustive.
- [x] Optional, unreachable, no-exit and malformed forms are distinct.
- [x] Stale rejection, invalidation and preservation are exact.
- [x] P04 is the earliest mutation consumer.
- [x] Target/later-domain inputs are excluded.
- [x] Implementation truth is absent.
