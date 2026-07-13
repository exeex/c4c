# Revision-Bound Provenance Analysis Contract

Contract-Status: under-review
Implementation-Status: absent
Kind: analysis
Applies-To: B5 / P05 earliest consumer plus exact verified later target-independent semantic checkpoints, including B8, for read-only address queries
Upstream: exact immutable verified B4-or-later semantic function plus exact-current same-revision CFG, dominance, value-flow and effects
Downstream: B5 / P05 memory/address normalization and C6/later read-only planning
Owner-Path: `src/backend/bir/analysis/provenance/README.md`
Last-Reconciled-Commit: `5e7909f3e`

## Purpose

`Provenance` is an immutable target-independent function analysis explaining
typed value/address origin from an exact verified B4-or-later target-independent
semantic BIR checkpoint. B4 serves the earliest P05 mutation consumer; exact
B8 Canonical and other admitted later semantic revisions serve read-only
queries such as C6. The analysis follows registered carriers and exact phi edge
occurrences without making pointers, snapshots, names, layout or route records
authoritative.

## Owns

- closed `AnalysisId::Provenance`, schema version 1, function-scope traits;
- complete revision/dependency/options key and checked handle;
- exact object/symbol origins, derived semantic paths/offsets, phi joins,
  escapes and explicit conservative unknown reasons;
- deterministic fixed point, semantic equality, failure, invalidation and
  checked new-key installation.

## Does Not Own

- object/address/value identity, core def-use, CFG, dominance or memory effects;
- alias truth, pointer legalization, memory mutation or canonical publication;
- target layout/offset/address modes, ABI/helper selection, preparation,
  constraints, allocation, MIR or emission;
- inference from host pointers, names, spelling, positions, text or legacy data.

## Inputs

One frozen exact verified target-independent semantic function is required. It
may be the B4 `SsaCanonical` checkpoint used by P05 or an exact later semantic
checkpoint, including immutable B8 `CanonicalBir`, used by a read-only query.
Schema-1 `Cfg`, `Dominance(Dominators)`, `PublicationValueFlow` and
`MemoryEffects` handles must all be exact-current for that same module/function
revision and checkpoint; the manager cannot carry B4 dependencies into B8 or
silently refresh one dependency beneath an old request. Typed values/types,
exact phi `EdgeKey` transports, semantic objects, symbols, address spaces,
source paths/offsets and module tables are observed.

### Exact descriptor and input-key matrix

| Input/key axis | Exact required value | Optional/non-provable form | Failure / forbidden substitution |
|---|---|---|---|
| descriptor | `AnalysisId::Provenance`, schema 1, `Function`, `CanonicalSemantic` | none | unknown/duplicate ID/schema is `ProvenanceRegistryInvalid` |
| stage/property | immutable B4 checkpoint through `SsaCanonical`, or exact verified later target-independent semantic checkpoint including B8 `CanonicalBir` | declaration/empty body is valid | unverified, target-aware, mixed-checkpoint or wrong/missing capability is `ProvenanceWrongInput` |
| module/function key | exact `ModuleEpoch`, `ModuleRevision`, `FunctionId`, `FunctionRevision` | empty body retains all keys | stale/foreign view is `StaleAnalysis` |
| CFG dependency | schema-1 `AnalysisId::Cfg` exact-current under the accepted checkpoint's module/function key | empty CFG complete | B4 reuse at B8, stale/mismatch is `ProvenanceStaleDependency` |
| dominance dependency | schema-1 `AnalysisId::Dominance(Dominators)` exact-current and matching that CFG/checkpoint key | empty dominance complete | stale/mismatch is `ProvenanceStaleDependency` |
| value-flow dependency | schema-1 `AnalysisId::PublicationValueFlow` exact-current and matching CFG/dominance/checkpoint key | unknown carrier facts are valid inputs | stale/mismatch is `ProvenanceStaleDependency` |
| effects dependency | schema-1 `AnalysisId::MemoryEffects` exact-current at the identical checkpoint/module/function key | known-empty effects valid | stale/mismatch is `ProvenanceStaleDependency` |
| dependency key | ordered complete keys `{Cfg, Dominance, PublicationValueFlow, MemoryEffects}` | none | missing/additional/cyclic dependency is `ProvenanceRegistryInvalid` |
| options/target key | canonical empty options; target key `None`; preparation empty | v1 has no semantic option | unsupported option/later-domain input is `ProvenanceForbiddenInput` |
| semantic input | typed stable values/objects/symbols/paths, exact def-use and phi edge transports | ambiguity/unregistered valid carrier yields `Unknown(reason)`; absent origin is `Absent` | malformed type/edge/def-use/object is `ProvenanceInputInvalid` |

The result key binds the accepted semantic checkpoint identity, exact epoch/
module/function revisions, all four complete same-revision dependency keys,
empty options, no target layout and empty preparation digest. The manager
cannot silently refresh one dependency beneath an old request, substitute a
B4 handle for B8, or retag a result under a later checkpoint.

## Outputs

Facts use stable BIR IDs, semantic path components and exact edge occurrences.
Cycles converge by deterministic monotone joins; ambiguity never selects an
arbitrary origin.

### Exhaustive provenance result-family matrix

| Result family | Exact stable facts | Optional/non-provable form | Consumer and invalidation |
|---|---|---|---|
| exact object origin | address/value `ValueId -> ObjectId` plus address space/type | absent object is `Absent`; ambiguity is `Unknown(ObjectAmbiguous)` | P05 object-preserving rewrite; object/value/type change invalidates |
| exact symbol origin | `ValueId -> SymbolId` and semantic symbol class | no symbol is `Absent`; unresolved valid symbol is `Unknown(SymbolUnresolved)` | P05 global/TLS/label address form; symbol/linkage change invalidates |
| null origin | typed null address and address space | non-null is `Absent` | P05 null canonical form; constant/type change invalidates |
| derived semantic path | base fact plus ordered typed field/index/byte-offset carriers | no derivation is `Absent`; unproved carrier/path is `Unknown(PathUnproved)` | P05 GEP/offset normalization; path/operand change invalidates |
| phi origin join | destination `ValueId`, every exact incoming `EdgeKey -> origin` and join status | empty phi absent; differing/unknown origins yield `Unknown(PhiJoinAmbiguous)` | P05 preservation proof; edge/incoming/value change invalidates |
| call/result origin | call/result IDs and registered semantic origin rule | no rule is `Absent`; indirect/opaque result is `Unknown(CallOriginUnproved)` | P05/P07 planning; call/declaration change invalidates |
| allocation origin | allocation/lifetime event ID, result/object and scope | non-allocation is `Absent`; opaque allocator is `Unknown(AllocationUnproved)` | P05 allocation normalization; effect/call change invalidates |
| escape state | exact value/object and closed escape reason | no escape known false; uncertainty is `Unknown(EscapeUnproved)` | P05 preservation; use/effect/return change invalidates |
| inline-asm origin | ordinary typed inputs/results and explicit declared relation only | opaque output is `Unknown(AsmOriginUnproved)` | preserve only; payload/declaration change invalidates |
| alias query | `Known(MustSame/MustDistinct)` only from exact semantic identity | otherwise `Unknown(AliasUnproved)`, never guessed | P05 may use only Known; either fact/dependency change invalidates |
| semantic equality | complete stable facts/status/reasons and fixed-point result | no equality inferred across keys | framework preservation only; never mutation authority |

`Known`, `Absent` and stable-reason `Unknown` are result data. `Unknown` denies
a rewrite requiring provenance/alias proof. Malformed core semantics fail and
never degrade to an unknown result.

## Adjacent-Stage Contract

[P04 SSA](../../passes/ssa/README.md) supplies exact B4 typed values and phi
coverage. Exact-current [memory effects](../memory_effects/README.md), CFG,
dominance and publication/value-flow form the dependency closure at every
accepted checkpoint. [P05 memory](../../passes/memory/README.md) remains the
earliest mutation consumer and must match every complete key to its B4 input.

After all B1-B8 mutations, immutable B8 Canonical supplies the exact revision
for [C6 address preparation](../../preparation/address/README.md). C6 is a
read-only consumer and must request a fresh B8 result whose four dependencies
all match that same B8 module/function revision. It cannot reuse the P05/B4
handle even when the stable facts would compare equal.

## Ordered Behavior

1. Validate the exact accepted B4-or-later semantic checkpoint, all exact-
   current same-revision dependency keys/order, empty options and target
   exclusion.
2. Freeze module objects/symbols/types/constants, typed SSA, CFG/dominance,
   value-flow and effects.
3. Seed exact object/symbol/null/allocation origins in stable-ID order.
4. Follow only registered typed path/carrier/call/phi rules, retaining every
   parallel `EdgeKey`; join cycles to a deterministic conservative fixed point.
5. Emit `Known`, `Absent` or stable-reason `Unknown` and validate coverage.
6. Publish atomically only while all result/dependency keys remain current.

## Invariants

- Semantic object/address-space/type/path identities are preserved exactly;
  target byte layout or ABI placement is never computed.
- Phi joins retain exact parallel edge occurrences and never choose by
  predecessor block or incoming vector position.
- Source-semantic sizes/alignments are facts, not target address legality.
- Inline asm remains opaque; text/constraints are never parsed for provenance.
- Stable IDs escape; pointers, names, text, layout and legacy routes do not.

## Failure and Diagnostics

Stable failures are `ProvenanceRegistryInvalid`, `ProvenanceWrongInput`,
`ProvenanceStaleDependency`, `StaleAnalysis`, `ProvenanceInputInvalid`,
`ProvenanceForbiddenInput`, `ProvenanceNonconvergent`, deterministic resource
exhaustion and cancellation. Diagnostics carry complete result/dependency keys
and stable value/object/symbol/edge/rule anchors.

Failure publishes no partial fact/fixed point/handle/cache entry. Stale
dereference returns `StaleAnalysis`; old handles never rebind or recompute.

## Analysis and Invalidation

Any dependency invalidation transitively invalidates provenance. Any accepted
checkpoint/revision change or observed
definition/use, operand, phi incoming, CFG occurrence, address/path, object/
symbol/type, memory effect, call descriptor, asm semantic edge or body revision
change also invalidates it. After revision increment every old handle is stale.
Checked preservation requires exact-current dependencies and proof that the
complete fact/status/fixed-point result is semantically equal under the new key.

## Target and ABI Rules

`Provenance` is `CanonicalSemantic`, with target key `None` and empty
preparation digest. It rejects target offsets/layout/address modes, ABI/helper
selection, constraints, homes, spills, frames, MIR and renderer facts.

## Implementation State

Implementation is absent. No checked-in descriptor, fixed-point algorithm,
result schema, dependency route, invalidation validator, build target or
runtime proof for `AnalysisId::Provenance` exists.

## Proof Requirements

- prove exact B4 and exact later/B8 checkpoint keys, same-revision dependency
  closure, stale rejection and deterministic fixed point;
- cover object/symbol/null/path/phi/call/allocation/escape/asm/alias statuses;
- prove parallel-edge joins and Known/Absent/Unknown/malformed distinctions;
- prove target/layout/ABI exclusion, invalidation and checked preservation;
- reconcile implementation/build truth.

## Open Questions

None for v1. New carrier or alias proof classes require schema registration.

## Review Checklist

- [x] Exact B4 and later/B8 typed-value/CFG/dominance/value-flow/effect inputs require one same-revision checkpoint.
- [x] Stable object/path/parallel-edge facts are exhaustive.
- [x] Known, Absent, Unknown and malformed forms are distinct.
- [x] Stale rejection and transitive invalidation are explicit.
- [x] P05 remains earliest mutation consumer; C6 is a fresh exact-B8 read-only consumer and target inference is forbidden.
- [x] Implementation truth is absent.
