# Revision-Bound Memory and Effect Analysis Contract

Contract-Status: under-review
Implementation-Status: absent
Kind: analysis
Applies-To: available from verified Raw; exact-current B4 input for B5 / P05
Upstream: one exact immutable verified Raw-or-later semantic function revision
Downstream: `Provenance`, B5 / P05 and later exact-revision effect consumers
Owner-Path: `src/backend/bir/analysis/memory_effects/README.md`
Last-Reconciled-Commit: `b6cabf1d2`

## Purpose

`MemoryEffects` is an immutable target-independent function analysis available
from verified Raw BIR. It classifies path-independent semantic accesses and
effects without becoming a second memory graph, alias oracle, pass authority
or stored postcondition. P05 requires a freshly requested exact-B4 result.

## Owns

- closed `AnalysisId::MemoryEffects`, schema version 1, function-scope traits;
- complete revision/options key, deterministic fact schema and checked handle;
- per-instruction/call typed effects, semantic object/address-space references
  when explicit, conservative function summary and status reasons;
- semantic equality, failure, stale rejection, invalidation and checked
  installation under a new exact key.

## Does Not Own

- core memory/address/atomic/effect nodes, mutation, verification or properties;
- CFG/path reachability, alias equivalence, object provenance or freshness;
- target size/layout/address-mode legality, ABI/helper selection, preparation,
  constraints, allocation, MIR, renderer or emission;
- inference from pointers, names, text, positions or legacy access records.

## Inputs

One frozen immutable verified Raw-or-later function is required. At P05 the
view is the exact B4 `SsaCanonical` revision. Typed opcodes, operands/results,
semantic object/address-space identities, source-semantic sizes/alignments,
calls/declarations and explicit effect descriptors are observed.

### Exact descriptor and input-key matrix

| Input/key axis | Exact required value | Optional/non-provable form | Failure / forbidden substitution |
|---|---|---|---|
| descriptor | `AnalysisId::MemoryEffects`, schema 1, `Function`, `CanonicalSemantic` | none | unknown/duplicate ID/schema is `MemoryEffectsRegistryInvalid` |
| accepted stage | immutable verified Raw or exact later canonical checkpoint | declaration/empty body is valid | unverified/mixed capability is `MemoryEffectsWrongInput` |
| module key | exact `ModuleEpoch` and `ModuleRevision` | no module reference still retains module key | stale/foreign module is `StaleAnalysis` |
| function key | exact `FunctionId` and `FunctionRevision` | empty body retains function key | stale/foreign function is `StaleAnalysis` |
| dependency key | empty ordered `dependencies` set | v1 deliberately has no CFG dependency, preserving Raw availability | any dependency is `MemoryEffectsRegistryInvalid` |
| options key | canonical empty schema-bound `AnalysisOptionsFingerprint` | v1 has no semantic option | nonempty/unsupported option is `MemoryEffectsOptionsInvalid` |
| semantic inputs | typed memory/address/atomic/call/asm opcodes, operands, objects, address spaces, declarations and explicit effects | absent effect on a pure registered node is known none; unfamiliar valid relation becomes `Unknown(reason)` | malformed types/descriptor/def-use is `MemoryEffectsInputInvalid` |
| source-semantic attributes | exact typed size, alignment and address-space values already in BIR | unspecified source alignment is `Absent`, never target default | layout/ABI reinterpretation is `MemoryEffectsForbiddenInput` |
| target/preparation axes | target-layout key `None`; preparation digest empty | none | target/helper/address-mode/allocation fact is `MemoryEffectsForbiddenInput` |

The complete key binds descriptor/schema/domain, exact epoch/module/function
revisions, empty dependencies/options, no target-layout key and empty
preparation digest. Worker count, pointer identity, cache state and traversal
order are not inputs.

## Outputs

One immutable result is sorted by stable function/block/instruction/value/
operand identity. It describes semantic effects only; path-sensitive ordering
requires separate exact-current CFG consumers and is not smuggled into v1.

### Exhaustive memory/effect result-family matrix

| Result family | Exact stable facts | Optional/non-provable form | Consumer and invalidation |
|---|---|---|---|
| instruction effect set | `InstId ->` ordered closed read/write/read-write/none classes | pure registered node is known empty; unfamiliar valid class is `Unknown(EffectClassUnproved)` | P05 preservation/rewrite proof; opcode/effect change invalidates |
| semantic access | `InstId`, operand role, object/address `ValueId`, address space, source size/alignment | object/provenance unproved is `Unknown(ObjectUnproved)`; unspecified alignment is `Absent` | P05 access normalization; operand/type/attribute change invalidates |
| volatility | exact volatile flag on access | non-memory node is `Absent` | P05 must preserve; flag change invalidates |
| atomic effect | exact ordering, scope and read/write/RMW class | non-atomic is `Absent`; unfamiliar valid ordering relation is `Unknown(AtomicRelationUnproved)` | P05 atomic normalization; atomic field change invalidates |
| fence/barrier | exact semantic fence kind, ordering and scope | absent barrier is `Absent` | P05 canonical fence; field change invalidates |
| allocation/lifetime | exact allocate/free/start/end/stack-state semantic event and stable operands | non-event is `Absent`; opaque lifetime is `Unknown(LifetimeUnproved)` | P05 planning; opcode/operand change invalidates |
| call effect | call `InstId`, semantic callee/declaration/bundle and declared effect set | indirect/undeclared effect is `Unknown(CallEffectUnproved)` | P05/P07 planning; call/declaration change invalidates |
| inline-asm effect | asm `InstId` and explicit conservative semantic effect summary | missing optional declared detail yields `Unknown(AsmEffectUnproved)` | observation only; payload/declared-effect change invalidates |
| escape effect | stable source/value/object and closed escape reason | no escape is known empty; ambiguity is `Unknown(EscapeUnproved)` | provenance/P05 preservation; use/call/return change invalidates |
| non-local checkpoint effect | checkpoint `InstId`, closed return behavior, semantic continuation point, ordered visible-object accesses/modifications | ordinary returns-once call is `Absent`; incomplete registered semantics is `Unknown(NonLocalBoundaryUnproved)` | B4/B5 boundary derivation; call/effect/order/object change invalidates |
| function summary | deterministic join of all exact and conservative `Unknown` facts | empty body is known none | provenance/later consumers; any contributing fact change invalidates |
| semantic equality | complete stable fact/status/reason set | no equality inferred across keys | framework preservation only; never pass authority |

`Unknown` is conservative result data and never authorizes a fold, reorder,
deletion or narrowing. `Absent` means the optional semantic fact is not present.
Malformed core semantics fail and are not represented as `Unknown`.

The non-local result family remains path-independent: it reports explicit call
semantics, instruction order, accesses, modifications, volatility, and escapes
for the exact revision. B4 owns visibility, B5 owns retained memory identity,
and B3 remains the only CFG authority. This analysis never infers `setjmp` or
`longjmp` from symbol text and never emits allocation clobbers or frame homes.

## Adjacent-Stage Contract

The analysis may be requested on verified Raw, but every canonical mutation
makes an older handle stale unless checked new-key preservation succeeds. At
B5, [P04 SSA](../../passes/ssa/README.md) supplies the exact B4 checkpoint,
[provenance](../provenance/README.md) consumes the matching result, and
[P05 memory](../../passes/memory/README.md) is the earliest mutation consumer
of these facts. No analysis result itself becomes a P05 postcondition.

## Ordered Behavior

1. Validate descriptor/domain, exact view/key, empty dependencies/options and
   target exclusion.
2. Freeze module declarations/registry facts and the exact function body.
3. Inventory typed instructions/calls/asm in canonical stable-ID order.
4. Classify registered semantic effects and emit `Known`, `Absent` or
   stable-reason `Unknown` without path or target inference.
5. Join the deterministic conservative function summary and validate coverage.
6. Publish atomically only while the full key remains current.

## Invariants

- Source-semantic size, alignment and address-space facts are observed exactly;
  no target layout, pointer width or ABI default is substituted.
- Effects are observations, not editable operands or stored alias truth.
- `InlineAsm` payload/constraints are opaque; only explicit semantic effect
  descriptors are observed.
- Stable IDs/roles escape; pointers, names, positions, text and legacy records
  never identify an access or object.
- The result contains no editor, CFG, route, property, checkpoint or stage token.

## Failure and Diagnostics

Stable failures are `MemoryEffectsRegistryInvalid`, `MemoryEffectsWrongInput`,
`StaleAnalysis`, `MemoryEffectsOptionsInvalid`, `MemoryEffectsInputInvalid`,
`MemoryEffectsForbiddenInput`, deterministic resource exhaustion and
cancellation. Diagnostics carry the complete key and stable instruction/value/
operand/object anchor in deterministic order.

Failure publishes no partial fact/summary/handle/cache entry. Stale dereference
returns `StaleAnalysis`; an old handle never rebinds or implicitly recomputes.

## Analysis and Invalidation

Any observed opcode, operand/result, memory descriptor, object/address,
source-semantic attribute, atomic, call/intrinsic effect, asm payload/effect,
declaration, global, registry or function revision change invalidates the
result and its transitive dependents. After revision increment old handles are
stale. Checked preservation installs a complete immutable new-key result only
when registered validators prove every fact/status reason semantically equal.

## Target and ABI Rules

`MemoryEffects` is `CanonicalSemantic`, with target key `None` and empty
preparation digest. It rejects target layout/size, address modes, ABI/helper
selection, constraints, homes, spills, frames, MIR and renderer facts.

## Implementation State

Implementation is absent. No checked-in descriptor, classification algorithm,
result schema, cache/invalidation validator, build target or runtime proof for
`AnalysisId::MemoryEffects` exists.

## Proof Requirements

- prove Raw availability and exact-current B4 key use with empty dependencies;
- cover all effect families and Known/Absent/Unknown/malformed distinctions;
- preserve source-semantic attributes while rejecting target interpretation;
- prove stale rejection, atomic failure, invalidation and checked preservation;
- reconcile implementation/build truth.

## Open Questions

None for v1. Path-sensitive products require a separate dependency/schema.

## Review Checklist

- [x] Raw availability and exact-current B4 use coexist without CFG dependency.
- [x] Stable effect/object facts and optional/unknown/error forms are exhaustive.
- [x] Source-semantic attributes are exact; target defaults are forbidden.
- [x] Stale rejection and transitive invalidation are explicit.
- [x] P05 is earliest mutation consumer.
- [x] Implementation truth is absent.
