# Revision-Bound Publication and Value-Flow Analysis Contract

Contract-Status: under-review
Implementation-Status: absent
Kind: analysis
Applies-To: B4 / P04 earliest consumer and later exact-revision semantic queries
Upstream: exact immutable B3 `CfgCanonical` plus exact-current `Cfg` and `Dominance`
Downstream: B4 / P04 SSA planning and exact-revision later semantic consumers
Owner-Path: `src/backend/bir/analysis/publication/README.md`
Last-Reconciled-Commit: `5eb4d6f43`

## Purpose

`PublicationValueFlow` is an immutable target-independent function analysis
describing where semantic values are defined, transported, consumed, returned
or cross typed call boundaries. Its historical name grants no checkpoint,
stage-token or graph-publication authority: core def-use, typed calls/returns,
terminators and exact phi occurrences remain source semantics.

## Owns

- closed `AnalysisId::PublicationValueFlow`, schema 1, function-scope traits;
- complete exact revision/dependency/options key and checked handle;
- stable-ID definition/use, entry/return/call, phi-edge transport, transparent
  carrier and special-carrier facts;
- explicit `Known`, `Absent` and stable-reason `Unknown` classifications;
- semantic equality, failure, invalidation and checked new-key installation.

## Does Not Own

- core def-use, definitions, uses, calls, returns, terminators, phi or IDs;
- SSA construction/repair, publication of BIR/checkpoints/properties or mutation;
- inference from names, pointers, positions, dense indices, text or legacy maps;
- ABI call locations, helper choice, target layout, preparation, allocation,
  MIR, renderer or emission.

## Inputs

One frozen immutable B3 view with cumulative properties through
`CfgCanonical`, its freshly recomputed exact-B3 `Cfg`, and matching
`Dominance(Dominators)` are required. Module declarations/signatures/constants,
core def-use and exact `EdgeKey` phi transport are observed.

### Exact descriptor and input-key matrix

| Input/key axis | Exact required value | Optional/non-provable form | Failure / forbidden substitution |
|---|---|---|---|
| descriptor | `AnalysisId::PublicationValueFlow`, schema 1, `Function`, `CanonicalSemantic` | none | unknown/duplicate ID/schema is `PublicationRegistryInvalid` |
| stage/property | immutable B3 checkpoint through `CfgCanonical` | declaration/empty body remains valid | wrong/missing capability is `PublicationWrongInput` |
| module key | exact `ModuleEpoch` and `ModuleRevision` | no declaration use still retains module key | stale/foreign module is `StaleAnalysis` |
| function key | exact `FunctionId` and `FunctionRevision` | empty body retains function key | stale/foreign function is `StaleAnalysis` |
| CFG dependency | schema-1 `AnalysisId::Cfg` at identical complete key | empty CFG is complete | stale/mismatched dependency is `PublicationStaleDependency` |
| dominance dependency | schema-1 `AnalysisId::Dominance(Dominators)` at identical complete key and matching CFG dependency | empty dominance is complete | stale/mismatched/transitively foreign dependency is `PublicationStaleDependency` |
| dependency key | ordered `{Cfg complete key, Dominance complete key}` | none | missing/additional/cyclic dependency is `PublicationRegistryInvalid` |
| options key | canonical empty schema-bound `AnalysisOptionsFingerprint` | v1 has no semantic option | nonempty/unsupported option is `PublicationOptionsInvalid` |
| semantic/target axes | exact def-use, types, calls/returns, phis, module declarations/signatures/constants; target key `None`, preparation empty | unfamiliar valid transparent relation becomes stable `Unknown(reason)` | malformed semantics is `PublicationInputInvalid`; later-domain fact is `PublicationForbiddenInput` |

The result key binds descriptor/schema/domain, exact epoch/module/function
revisions, both dependency complete keys, canonical-empty options, no target
layout and empty preparation digest. Dependencies must be mutually same-revision;
the manager cannot silently refresh either beneath an old request.

## Outputs

Facts use stable BIR IDs, operand roles and exact edge occurrences, sorted by
canonical module/function/block/instruction/operand/edge order.

### Exhaustive value-flow result-family matrix

| Result family | Exact facts and identity | Optional/non-provable form | Consumer and invalidation |
|---|---|---|---|
| definitions | `ValueId -> {InstId/result role or parameter role, type}` | no definition is `Absent`; malformed missing definition is failure | P04 ownership/renaming; definition/type change invalidates |
| ordinary uses | `ValueId -> {user InstId, typed operand role/index}` | no uses is known empty | P04 use repair; operand/use change invalidates |
| entry parameters | typed parameter position and entry `ValueId` | no parameters is known empty | P04 initial definitions; signature/parameter change invalidates |
| return transport | return `InstId`, typed result position and `ValueId` | no return is `Absent`; multiple returns remain distinct | P04 verification; return/use/CFG change invalidates |
| call boundaries | call `InstId`, semantic callee/declaration ID, typed parameter/result positions and values | indirect/unknown callee identity is `Unknown(IndirectCallee)`, never ABI inference | P04 and later semantic planning; call/declaration change invalidates |
| phi transport | destination phi/result `ValueId`, exact incoming `EdgeKey` and incoming `ValueId` | no phi is known empty; parallel edges remain distinct | P04 incoming construction/repair; edge/phi/value change invalidates |
| transparent carriers | ordered stable producer/carrier/consumer IDs and registered rule ID | absent carrier is `Absent`; unproved chain is `Unknown(CarrierUnproved)` | P04 alias/trivial planning; any member/use change invalidates |
| special carriers | typed opcode/bundle/operand/result IDs for registered special forms | unfamiliar valid form is `Unknown(SpecialCarrierUnproved)` | P04 disposition; opcode/bundle/use change invalidates |
| availability query | `Known(value dominates ordinary use / exact incoming edge)` using matching dominance | unreachable is `Absent`; non-provable valid relation is `Unknown(AvailabilityUnproved)` | P04 rewrite proof; dominance/definition/use/edge change invalidates |
| semantic equality | all stable-ID/edge facts and explicit status/reason values | no equality inferred across keys | framework preservation only; never mutation authority |

`Unknown` is result data and denies a rewrite that requires proof; it is not
failure, false, or permission to guess. Malformed def-use/type/edge/call
semantics fail the request and never become `Unknown`.

## Adjacent-Stage Contract

[CFG](../cfg/README.md) and [dominance](../dominance/README.md) supply exact-B3
dependencies. [P04 SSA](../../passes/ssa/README.md) is the earliest mutation
consumer and must match all three handles to its immutable B3 input. It may use
`Known` facts, preserve when a valid fact is `Unknown`/`Absent`, or reject only
under its closed disposition; this analysis cannot repair or publish BIR.

## Ordered Behavior

1. Validate exact B3 key, dependency equality/order, empty options and target
   exclusion.
2. Freeze module declarations/signatures/constants, function body, def-use,
   CFG and dominance products.
3. Inventory definitions, uses, parameters, returns, calls and phi transports
   in stable-ID/role/edge order.
4. Follow only registered transparent/special carriers and emit `Known`,
   `Absent` or stable-reason `Unknown` facts.
5. Validate complete def-use/edge/type coverage and semantic equality record.
6. Publish atomically only while every result/dependency key remains current.

## Invariants

- Core def-use and typed nodes remain authority; facts are disposable views.
- Each phi transport retains its exact `EdgeKey`; parallel occurrences never
  collapse to predecessor block or vector position.
- Call facts are semantic positions only, never ABI locations or helper routes.
- Stable IDs/roles escape; names, text, pointers, dense indices and legacy
  publication records do not.
- Facts contain no editor, candidate, property, checkpoint or stage token.

## Failure and Diagnostics

Stable failures are `PublicationRegistryInvalid`, `PublicationWrongInput`,
`PublicationStaleDependency`, `StaleAnalysis`, `PublicationOptionsInvalid`,
`PublicationInputInvalid`, `PublicationForbiddenInput`, deterministic resource
exhaustion and cancellation. Diagnostics carry full result/dependency keys and
stable entity/operand/edge anchors in deterministic order.

Failure publishes no partial fact/handle/cache entry. A stale dereference
returns `StaleAnalysis`; an old handle never rebinds or implicitly recomputes.

## Analysis and Invalidation

Any CFG/dominance invalidation transitively invalidates publication flow. Any
observed definition, result, use, operand, phi incoming, terminator slot, block
order/membership, call bundle/effect, signature, declaration, return, constant
or special-carrier change also invalidates it. After revision increment every
old handle is stale. Checked preservation installs a full immutable new-key
result only when both dependencies are exact-current and validators prove all
facts/status reasons semantically equal; it never retargets a handle.

## Target and ABI Rules

The analysis is `CanonicalSemantic`, with target key `None` and empty
preparation digest. It rejects layout, ABI location, helper selection,
constraints, homes, spills, frames, MIR, renderer and environment facts.

## Implementation State

Implementation is absent. No checked-in descriptor, value-flow algorithm,
result schema, dependency route, invalidation validator, build target or
runtime proof for `AnalysisId::PublicationValueFlow` exists.

## Proof Requirements

- prove exact revision/dependency/options keys and stale/mismatch rejection;
- cover stable definition/use/call/return/phi facts, parallel-edge transport
  and Known/Absent/Unknown versus malformed distinctions;
- prove atomic failure, transitive invalidation and checked preservation;
- prove P04 earliest use and non-authoritative historical naming;
- reconcile implementation/build truth.

## Open Questions

None for v1. New carrier classifications require a registered schema revision.

## Review Checklist

- [x] Exact B3, CFG and dominance dependencies are same-revision.
- [x] Stable-ID/role/parallel-edge value flow is exhaustive.
- [x] Known, Absent, Unknown and malformed forms are distinct.
- [x] Stale rejection and transitive invalidation are explicit.
- [x] P04 is earliest mutation consumer; no publication authority is implied.
- [x] Target/ABI/preparation/allocation facts are excluded.
- [x] Implementation truth is absent.
