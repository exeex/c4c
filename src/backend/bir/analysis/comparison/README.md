# Comparison and Select Analysis Contract

Contract-Status: under-review
Implementation-Status: absent
Kind: analysis
Applies-To: B2 / P02 earliest consumer and later exact-revision semantic queries
Upstream: one immutable B1 / P01 `TypesLegal` revision
Downstream: one immutable `ComparisonSelect` result consumed by B2 / P02
Owner-Path: `src/backend/bir/analysis/comparison/README.md`
Last-Reconciled-Commit: none

## Purpose

`ComparisonSelect` is the target-independent `CanonicalSemantic` analysis
requested immediately before B2/P02. It derives comparison, condition,
transparent-producer, consumer and select relationships from the exact
immutable P01 `TypesLegal` revision. It is a disposable planning result, not a
pipeline stage, semantic owner or rewrite prescription.

Valid relationships that cannot be proved receive an explicit `Unknown` fact.
Malformed, stale, foreign or unsupported requests fail and publish no result.

## Owns

- the closed `AnalysisId::ComparisonSelect`, schema version 1, function scope
  and canonical-semantic domain descriptor;
- the complete exact revision/dependency/options key and checked handle;
- deterministic derivation of typed comparison/select relationship facts;
- explicit known, inverted/equivalent, non-provable `Unknown` and absent-
  relationship forms;
- semantic equality, invalidation traits and mutation-specific preservation
  validation for this result family;
- deterministic all-or-nothing cache publication and stable analysis failure.

## Does Not Own

- B1/P01 or B2/P02 mutation, canonical orientation, folding, fusion, operand
  swapping, cast removal, select rewriting, pass order or property publication;
- core IDs/types/def-use, analysis-manager caching, pass transactions,
  revision allocation, verifier rules or stage capabilities;
- CFG/dominance, publication/value-flow, memory/provenance, call-graph,
  liveness or allocation analysis facts;
- target/profile/layout, helper selection, ABI/preparation, constraints,
  register/stack placement, MIR, rendering or emission;
- names, pointers, vector positions, dense indices, rendered expressions,
  legacy comparison routes or persistent rewrite decisions as identity.

## Inputs

The only normal input is one immutable function view from the exact successful
P01 checkpoint carrying `RawVerified` plus `TypesLegal`. The view and analysis
key must agree on module epoch/revision, function identity/revision and options.
The descriptor has no analysis dependencies and no target/preparation axes.

### Exact descriptor and input-key matrix

| Key/input axis | Exact required value | Optional/non-provable form | Failure / forbidden substitution |
|---|---|---|---|
| descriptor | `AnalysisId::ComparisonSelect`, schema 1, `Function`, `CanonicalSemantic` | none | unknown/duplicate ID or schema is `ComparisonRegistryInvalid` |
| stage/property | immutable P01 checkpoint with `RawVerified` and `TypesLegal` | declaration functions produce a valid empty ordered result | wrong/missing capability or property is `ComparisonWrongInput` |
| module key | exact `ModuleEpoch` plus `ModuleRevision` | none; type/constant tables require the module axis | stale/foreign module is `StaleAnalysis` |
| function key | exact `FunctionId` plus `FunctionRevision` | empty body is valid; it does not omit the function key | stale/foreign/missing function is `StaleAnalysis` |
| dependency key | empty `dependencies` set | core def-use is an input invariant, not a dependency handle | any undeclared/dependent analysis is `ComparisonRegistryInvalid` |
| options key | schema-bound `AnalysisOptionsFingerprint` containing the bounded transparent-chain depth | a zero-depth option yields only direct facts and explicit `Unknown` chains | unsupported/unbounded option is `ComparisonOptionsInvalid` |
| semantic inputs | typed opcodes/predicates/casts/selects, stable IDs, exact def-use, module types/constants and canonical order | a valid unfamiliar producer/use remains `Unknown` | malformed def-use/type/descriptor is `ComparisonInputInvalid` |
| target/preparation axes | no target-layout key and empty preparation digest | none | any target/ABI/helper/allocation/renderer input is `ComparisonForbiddenInput` |

The complete cache key is therefore:

```text
AnalysisId::ComparisonSelect + schema 1 + CanonicalSemantic
+ ModuleEpoch + ModuleRevision
+ FunctionId + FunctionRevision
+ AnalysisOptionsFingerprint
+ dependencies {}
+ target_layout none + preparation_facts empty
```

Worker count, schedule, cache state, pointer values, host/environment state,
names and render settings are never key inputs.

## Outputs

The result is immutable and stamped with the complete input key. Externally
visible collections use stable IDs and canonical block/instruction/operand
order. Dense indices and worklists are result-local and cannot escape.

### Exhaustive result-family matrix

| Fact family | Exact immutable payload | Known/optional/non-provable form | Earliest consumer | Invalidated by |
|---|---|---|---|---|
| compare descriptor | compare `InstId`, result `ValueId`, closed predicate/domain and ordered operand IDs/types | valid unknown predicate relationship is `Unknown(UnsupportedRelation)`; malformed predicate is failure | P02 comparison orientation/folding planning | opcode/predicate/operand/result/type/constant change |
| transparent condition producer chain | ordered typed cast/boolean-normalization producer IDs up to the option bound | no producer is `Absent`; depth/semantic uncertainty is `Unknown(ChainUnproved)` | P02 cast/condition planning | producer, cast, def-use, type or bound-option change |
| ordinary consumer/use classes | exact user `InstId`, operand role/index and branch/select/scalar/phi/call/return/store/other class | unfamiliar valid use is `Unknown(UseClassUnproved)` | P02 use-safe rewrite planning | user, role/index, use-list, block/order or terminator-condition change |
| select descriptor | select `InstId`, condition/true/false/result IDs/types | absent nesting is `Absent`; unfamiliar valid form is `Unknown(SelectRelationUnproved)` | P02 select normalization | select opcode/operand/result/type/use change |
| select nesting/chain | deterministic root/member/parent/child IDs and bounded depth | valid cycle prevention/depth uncertainty is `Unknown(ChainUnproved)` | P02 bounded chain planning | any member edge/use/order or option change |
| equivalence/inversion | exact compare/result ID pair plus closed proof rule ID | no proof is `Unknown(EquivalenceUnproved)`, never false-by-default | P02 comparison/select planning and later exact-key consumers | either producer/predicate/operand/type/constant/use change |
| empty function result | exact function key plus empty ordered collections | valid for declarations/empty bodies | P02 no-op planning | function/body/module-table revision change |

`Unknown` and `Absent` are result data, not failures and not permission to
guess. The result contains no rewrite action, target fact, mutable reference,
stage token or semantic cache attached to BIR.

## Adjacent-Stage Contract

[P01 legalize](../../passes/legalize/README.md) publishes the sole accepted
`TypesLegal` input. The [analysis framework](../README.md) validates the exact
key, computes and caches the result, checks every handle dereference and owns
preservation/rebinding. [P02 scalar](../../passes/scalar/README.md) is the
earliest actual consumer and alone may propose a rewrite.

P02 must use a handle whose complete key equals its input checkpoint. It may
preserve a valid form when the analysis reports `Unknown`/`Absent`; it cannot
retarget the handle, reconstruct facts from text or treat analysis facts as
mutation/publication authority.

## Ordered Behavior

1. Validate the descriptor, exact P01 capability/property, complete key,
   dependency emptiness and target/preparation exclusion.
2. Freeze the exact function view and module type/constant tables.
3. Traverse blocks, instructions, operands and users in canonical order;
   derive direct typed facts before bounded transparent chains/nesting.
4. Emit `Known`, `Absent` or stable-reason `Unknown` for every result family;
   never guess through a malformed fact.
5. Sort all externally visible collections by stable semantic order and derive
   the semantic-equality fingerprint.
6. Recheck every key axis. Atomically publish the complete immutable result or
   discard it and return structured failure.

## Invariants

- Stable BIR IDs plus operand roles/indices are the only entity identity.
- Predicate equivalence/inversion requires one closed typed semantic proof;
  equal rendering or value names prove nothing.
- Transparent traversal is bounded, cycle-safe and preserves type, poison,
  undef, exceptional and floating semantics.
- Result facts describe only the exact keyed revision and cannot prescribe a
  rewrite or survive through a changed key.
- Computation order, output order, failure order and semantic equality are
  independent of worker schedule, cache state and hash iteration.

## Failure and Diagnostics

Stable failures are `ComparisonRegistryInvalid`, `ComparisonWrongInput`,
`StaleAnalysis`, `ComparisonOptionsInvalid`, `ComparisonInputInvalid`,
`ComparisonForbiddenInput`, deterministic resource exhaustion and
cancellation. Diagnostics carry the complete key, stable rule and typed entity
anchor in deterministic order.

Failure/cancellation publishes no partial result, key, handle or cache entry.
Malformed input never becomes `Unknown`. A handle dereferenced against any
different epoch/module/function revision/options key returns `StaleAnalysis`;
it never rebinds or triggers implicit recomputation.

## Analysis and Invalidation

The result observes function bodies plus module type/constant tables. It is
invalidated by any change to an observed opcode, predicate, operand/result,
value/type/constant, cast, select, phi incoming, use list, block membership or
order, terminator condition/successor, function body or observed module table.

An empty `MutationSummary` at the same revision may reuse the same handle.
After any revision increment, every old handle is stale. Preservation means
the manager may install a semantically equal immutable result under the new
complete key only after descriptor traits, dependencies and the registered
mutation-specific validator prove equality; it never retargets an old handle.
Failure, rollback or candidate-local computation changes no published cache.

## Target and ABI Rules

The descriptor is `CanonicalSemantic`: it has no target-layout key or
preparation digest and rejects target profiles, target layout, ABI/helper
selection, constraints, homes, spills, frames, MIR, renderers and environment
features. C1/C2 and later consumers compute their own later-domain facts; they
cannot add target axes to this schema.

## Implementation State

Implementation is absent. This directory contains only this README. No
`AnalysisId::ComparisonSelect` descriptor, computation, result/handle schema,
cache registration, build edge or focused proof is checked in. Legacy
comparison/select views are migration evidence only and cannot satisfy this
contract.

## Proof Requirements

- require exact metadata/core-first/detail order and resolved links;
- require the complete descriptor/key axes, dependency emptiness and no target
  axes;
- require every result family to state known, optional/non-provable, consumer
  and invalidation behavior;
- reject stale/foreign/mixed keys and prove old handles never retarget;
- prove deterministic all-or-nothing computation, cancellation isolation,
  semantic equality and checked preservation/rebinding;
- prove P02 is the earliest real consumer and `Unknown` grants no rewrite;
- verify implementation/build absence rather than inferring it from status.

## Open Questions

No open analysis question authorizes mutation, target facts, a new dependency,
an adjacent-owner edit or implementation. A new relationship requires a
versioned schema/result/invalidation proof before it becomes observable.

## Review Checklist

- [x] Metadata spine and core-first/detail order are exact.
- [x] Input is the exact immutable P01 `TypesLegal` revision and key.
- [x] Descriptor/dependencies/options/target exclusions are closed.
- [x] Known, Absent, Unknown, malformed and stale forms are distinct.
- [x] Results are immutable, stable-ID keyed and non-authoritative.
- [x] Invalidation/preservation/rebinding and failure publication are exact.
- [x] P02 is the earliest actual consumer and stale facts cannot cross.
- [x] Implementation is truthfully absent.
