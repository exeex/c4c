# Revision-Bound CFG Analysis Contract

Contract-Status: under-review
Implementation-Status: absent
Kind: analysis
Applies-To: B3 / P03 earliest planning consumer and mandatory post-P03 recomputation
Upstream: exact immutable B2 / P02 `ScalarsCanonical` revision
Downstream: B3 planning, then exact-B3 dominance/publication/SSA consumers
Owner-Path: `src/backend/bir/analysis/cfg/README.md`
Last-Reconciled-Commit: `878e56a97`

## Purpose

`Cfg` is the target-independent `CanonicalSemantic` function analysis requested
immediately before B3/P03 planning and recomputed from P03's resulting
terminators before B4 dependencies are admitted. It derives immutable graph
facts from typed terminators and their ordered successor slots. It never
mutates BIR and never publishes a second stored graph.

## Owns

- the closed `AnalysisId::Cfg`, schema version 1, function-scope descriptor;
- its complete exact checkpoint/revision/options key and checked handle;
- terminator-derived successor occurrences, incoming occurrences, edge kinds,
  reachability, unreachable blocks, canonical traversal and local indices;
- deterministic computation, failure, semantic equality, invalidation and
  checked same-fact installation under a new complete key.

## Does Not Own

- terminators, successor slots, `BlockId`, `EdgeKey`, revisions or def-use;
- block/terminator/edge mutation, phi repair or P03 canonical dispositions;
- dominance, SSA availability, publication, liveness, memory or provenance;
- names, pointers, layout adjacency, dense indices, rendered text, legacy
  route records or stored predecessor/edge side tables as semantic authority;
- target/profile/layout, helper, ABI, preparation, allocation, MIR or emission.

## Inputs

One frozen immutable B2 function view is required. Its exact module and
function checkpoint carries `RawVerified`, `TypesLegal` and
`ScalarsCanonical`, and the P02 postcondition/configured verifier accepts that
same revision. The terminator registry, stable IDs and def-use are already
structurally valid; analysis cannot repair inherited failure.

### Exact descriptor and input-key matrix

| Input/key axis | Exact required value | Optional/empty form | Failure / forbidden substitution |
|---|---|---|---|
| descriptor | `AnalysisId::Cfg`, schema 1, `Function`, `CanonicalSemantic` | none | unknown/duplicate ID or schema is `CfgRegistryInvalid` |
| stage/property | immutable B2 checkpoint with `RawVerified`, `TypesLegal`, `ScalarsCanonical` | a proven P02 no-op still carries the B2 occurrence/property | wrong/missing capability is `CfgWrongInput` |
| module key | exact `ModuleEpoch` plus checkpoint `ModuleRevision` | no module table is observed; revision still binds the accepted checkpoint | stale/foreign module is `StaleAnalysis` |
| function key | exact `FunctionId` plus `FunctionRevision` | declaration/empty body still carries the function key | stale/foreign/missing function is `StaleAnalysis` |
| dependency key | empty ordered `dependencies` set | none | any analysis dependency is `CfgRegistryInvalid` |
| options key | schema-bound canonical empty `AnalysisOptionsFingerprint` | v1 has no semantic option | nonempty/unsupported option is `CfgOptionsInvalid` |
| semantic input | typed terminator opcode and exact ordered `(SuccessorRole, index, BlockId)` slots, entry block and block membership/order | declaration/empty body yields the declared empty result | malformed terminator, slot, foreign block or entry is `CfgInputInvalid` |
| target/preparation axes | target-layout key `None`; preparation digest empty | none | target/ABI/helper/allocation/renderer input is `CfgForbiddenInput` |

The complete cache key is:

```text
AnalysisId::Cfg + schema 1 + CanonicalSemantic
+ ModuleEpoch + ModuleRevision + FunctionId + FunctionRevision
+ dependencies empty + canonical-empty options
+ target_layout none + preparation_facts empty
```

No body digest substitutes for the exact function revision. Worker count,
pointer identity, cache state and traversal order are not key inputs.

## Outputs

The result is one immutable stable-ID keyed observation of that exact input.
Every local edge occurrence has the core structural identity:

```text
EdgeKey { source BlockId, SuccessorRole role, uint32_t index }
EdgeFact { EdgeKey key, BlockId destination, closed EdgeKind kind }
```

### Exhaustive CFG result-family matrix

| Result family | Exact facts and stable identity | Optional/non-provable form | Consumer and invalidation |
|---|---|---|---|
| function entry | exact `FunctionId` and optional entry `BlockId` | declaration/empty body is `Absent` | P03 reachability planning; entry/body change invalidates |
| successor occurrences | ordered `EdgeKey -> {destination BlockId, EdgeKind}` for every terminator slot | zero-successor return/trap is an empty known set | P03 edge/block planning; terminator/slot change invalidates |
| incoming occurrences | destination `BlockId ->` exact ordered `EdgeKey` multiset | entry or unreachable block may have an empty known set | P03 phi/edge repair; any source occurrence change invalidates |
| parallel-edge multiplicity | distinct role/index keys retained even when source and destination repeat | multiplicity one needs no special annotation | P03 exact occurrence repair; slot/order/destination change invalidates |
| reachable set | exact stable `BlockId` set reached from entry | declaration/empty body is empty; no valid topology is guessed | P03 removal/planning; entry or edge change invalidates |
| unreachable set | exact block-membership difference from reachable set | empty is valid | P03 closed-region removal; membership/entry/edge change invalidates |
| traversal products | deterministic reverse postorder and postorder over reachable stable IDs | empty body yields empty orders | P03 planning only; block/edge/order change invalidates |
| analysis-local indices | reversible dense index table rooted in the result's stable-ID order | consumer may ignore it | never identity; every graph-key change invalidates |
| semantic equality record | exact comparison of entry, stable block set and all keyed edge/result families | no cross-key equality is assumed | framework-only checked preservation; never a pass authority |

All valid v1 topology is decidable from registered typed terminators, so CFG
does not use an `Unknown` graph answer. Optional facts are explicit `Absent` or
known empty collections. An unfamiliar valid terminator family is
`CfgUnsupportedTerminator`, and malformed topology is `CfgInputInvalid`; neither
is converted to a partial/non-provable result.

## Adjacent-Stage Contract

[P02 scalar](../../passes/scalar/README.md) supplies the exact immutable
`ScalarsCanonical` checkpoint. [P03 CFG](../../passes/cfg/README.md) is the
earliest planning consumer and may only use a handle whose complete key equals
that input function. Analysis facts may propose work but cannot authorize or
perform a mutation.

After P03, the pre-planning handle is never the B4 handoff. P03 requires a CFG
recomputation from the resulting terminators and the manager publishes a
checked handle under the exact B3 key. [Dominance](../dominance/README.md),
[publication](../publication/README.md) and [B4/P04 SSA](../../passes/ssa/README.md)
may consume only that post-P03 result and its exact-key dependents.

## Ordered Behavior

1. Validate descriptor/domain, exact B2 checkpoint/key, empty dependencies and
   target/preparation exclusion.
2. Freeze the exact function body and inventory blocks/terminators in canonical
   stable-ID order.
3. Decode every typed successor slot into its exact role/index `EdgeKey`,
   destination and closed edge kind; retain parallel occurrences separately.
4. Build incoming multisets by edge occurrence, then compute entry reachability,
   unreachable membership and deterministic traversal products.
5. Validate complete coverage, stable-ID reversibility and result invariants.
6. Publish the whole immutable result only if the input key is still current;
   otherwise discard it and return `StaleAnalysis`.

## Invariants

- Terminators and their typed ordered successor slots are the sole stored edge
  authority. A CFG result is disposable observation only.
- `BlockId` identifies a block; `EdgeKey` identifies one successor occurrence.
  Equal destinations never collapse distinct role/index slots.
- Incoming collections retain the exact live `EdgeKey` multiset, including
  parallel edges. Set-of-predecessor-blocks is not a substitute.
- Stable IDs and canonical edge order escape; dense indices, pointers, names,
  vector positions, layout adjacency and rendered text do not.
- `MayUnwind` without a local successor is an escape effect, not an invented
  local edge. Typed asm-goto slots remain ordinary exact occurrences.
- The result contains no editor, mutation plan, phi value, property stamp,
  stage token or target/preparation fact.

## Failure and Diagnostics

Stable failures are `CfgRegistryInvalid`, `CfgWrongInput`, `StaleAnalysis`,
`CfgOptionsInvalid`, `CfgInputInvalid`, `CfgUnsupportedTerminator`,
`CfgForbiddenInput`, deterministic resource exhaustion and cancellation.
Diagnostics carry the complete key, stable rule and typed entity/slot anchor in
deterministic order.

Failure/cancellation publishes no partial edge, traversal, handle or cache
entry. A handle dereferenced against a different epoch/module/function revision
or options key returns `StaleAnalysis`; it never rebinds or implicitly
recomputes.

## Analysis and Invalidation

Any entry, block membership/order, terminator opcode, successor role/index/
order/destination, asm-goto topology or relevant function-body revision change
invalidates `Cfg` and all transitive dependents. Instruction-only edits may be
preserved only when registered traits and a mutation-specific validator prove
the complete result semantically equal.

An empty `MutationSummary` at the same revision may reuse the same handle.
After any revision increment, every old handle is stale. Checked preservation
may install a complete immutable result under the new exact key only after
dependency, mutation and semantic-equality validation; it never retargets an
old handle. P03 deliberately requires post-mutation CFG recomputation rather
than declaring its pre-planning result preserved.

## Target and ABI Rules

`Cfg` is `CanonicalSemantic`. It has no target-layout key or preparation digest
and rejects target profiles/layout, ABI/helper selection, constraints, homes,
spills, frames, MIR, renderer state and environment features. Target branch
encodings, fallthrough preference and exception ABI cannot alter canonical CFG
facts.

## Implementation State

Implementation is absent. No checked-in descriptor registration, computation,
result/handle schema, cache route, invalidation validator, build target or
runtime proof for `AnalysisId::Cfg` exists. This document is design authority,
not implementation coverage.

## Proof Requirements

- verify metadata/core-first order and both substantive matrices;
- prove exact key axes, empty dependencies/options and target exclusion;
- cover every terminator slot, parallel-edge multiplicity, empty functions,
  stable-ID order and malformed/unsupported/error distinctions;
- test stale dereference, concurrent revision change, atomic failure, mutation
  invalidation, checked new-key preservation and transitive dependency eviction;
- prove P03 is the earliest planning consumer and mandatory post-P03
  recomputation is the only CFG input admitted to dominance/publication/B4;
- reconcile implementation/build claims against checked-in storage.

## Open Questions

None for the v1 contract. A future terminator family must extend the closed
terminator registry and this result contract before it can produce CFG facts.

## Review Checklist

- [x] Exact B2 checkpoint, descriptor, revision/options/dependency key defined.
- [x] Terminator-only edge authority and exact parallel multiplicity retained.
- [x] Optional, known-empty, unsupported and malformed forms distinguished.
- [x] Stable IDs escape; dense/name/text/layout substitutes are forbidden.
- [x] Failure, stale rejection, invalidation and checked preservation defined.
- [x] Earliest P03 use and mandatory post-P03 recomputation are explicit.
- [x] Target/ABI/preparation/allocation inputs are excluded.
- [x] Implementation truth is absent.
