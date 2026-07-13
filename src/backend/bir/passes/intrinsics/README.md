# P07 Intrinsic Canonicalization Pass Contract

Contract-Status: under-review
Implementation-Status: absent
Kind: pass
Phase-ID: B7 / P07
Upstream: exact B6 `AggregatesCanonical` plus exact-current call graph/effects
Downstream: one immutable stamped B7 candidate admitted only to the B8 Canonical gate
Owner-Path: `src/backend/bir/passes/intrinsics/README.md`
Last-Reconciled-Commit: `c0e3cdc6e`

## Purpose

P07 is the target-independent B7 module transformation and final canonicalizer.
It consumes the exact accepted B6 wave plus matching call-graph and per-function
memory-effects facts, normalizes intrinsic/call/variadic/opaque-asm semantics,
then freezes one verified B7 candidate or fails atomically. It does not mint
`CanonicalBir`; B8 alone owns that gate.

## Owns

- closed `PassId::IntrinsicCanonicalize` / B7 module occurrence;
- registered build-versioned intrinsic identity/signature/immediate/effect
  reconciliation and semantic alias normalization;
- registered intrinsic-family atomic forms and typed call/variadic bundle
  normalization assigned to P07;
- closed `InlineAsm` ordinary-node payload shape validation/preservation;
- P07 local/cumulative postconditions and `IntrinsicsCanonical` request.

## Does Not Own

- helper selection, target feature availability, ISA instruction choice,
  calling convention/ABI placement or varargs physical realization;
- inline-asm template/constraint/clobber interpretation or a parallel asm graph;
- P01-P06 form repair, CFG/SSA/memory/aggregate recreation or target lowering;
- preparation, constraints, allocation, MIR, renderer or emission;
- transaction, analysis, property, B7 checkpoint or `CanonicalBir` publication.

## Inputs

P07 consumes exactly one immutable whole-module B6 checkpoint, one matching
schema-1 module `CallGraph`, and a complete ordered schema-1 `MemoryEffects`
handle set for all functions. Every handle's complete key matches the B6
module/function revisions. P06 postconditions, core def-use and the configured
input verifier accept that same wave.

### Exact input and dependency matrix

| Input axis | Exact required state | Optional/non-provable form | Failure / forbidden substitution |
|---|---|---|---|
| owning checkpoint | exact successful immutable B6 module wave | empty module valid | wrong/missing capability is `IntrinsicWrongInput` |
| module/function key | exact `ModuleEpoch`, `ModuleRevision`, ordered function-revision digest, B6 stamp and plan fingerprint | empty module has exact empty digest | stale/foreign/mixed wave is `IntrinsicStaleInput` |
| cumulative properties | Raw + P01 `TypesLegal` + P02 `ScalarsCanonical` + P03 `CfgCanonical` + P04 `SsaCanonical` + P05 `MemoryCanonical` + P06 `AggregatesCanonical` | none | missing/self-asserted property is `IntrinsicWrongInput` |
| P06 acceptance | every P01-P06 postcondition, exact CFG/SSA/memory/aggregate profiles and configured verifier | no earlier form may remain | mismatch is `IntrinsicP06ContractFailure` |
| call graph | schema-1 `AnalysisId::CallGraph` at identical module/body-digest B6 key | empty graph complete; unknown-indirect facts valid | missing/stale/mismatched handle is `IntrinsicStaleAnalysis` |
| memory effects | complete ordered schema-1 `AnalysisId::MemoryEffects` set at exact B6 function keys | `Absent`/`Unknown` facts preserve valid rows unless a row rejects | missing/stale/mixed set is `IntrinsicStaleAnalysis` |
| effects reuse decision | unchanged same-key handle may reuse; changed revision requires validator-proven new-key installation or recomputation | P06 no-op may retain exact same key | any B4/B5-key handle or declared-only preservation is `IntrinsicStaleAnalysis` |
| intrinsic registry | exact module `RegistryVersion`, closed `IntrinsicId` entry, semantic signature/immediates/portable requirements/effects | module with no intrinsics valid | unknown/version/signature mismatch is `IntrinsicRegistryInvalid` |
| semantic graph | stable typed calls/intrinsics/variadic bundles/inline-asm ordinary nodes and complete def-use | unresolved indirect call is valid unknown graph fact | malformed types/bundles/use is inherited verifier failure |
| target/helper exclusion | no target feature result, helper route, ABI/constraint/preparation/allocation/MIR input | helper eligibility remains semantic only | later-domain fact is `IntrinsicForbiddenAuthority` |

P07 has no fallback registry, target feature query, helper table selection,
calling-convention placement, asm parser, name/text/pointer inference or legacy
route lookup.

## Outputs

One atomic B7 module candidate retains the epoch, derives exact module/function
revisions and authoritative `MutationSummary`, records P07 as canonical-v1
ordinal 7, retains every P01-P06 property and establishes
`IntrinsicsCanonical` only after all postconditions succeed.

### Exact output handoff matrix

| Output/product | Exact consumer | Required binding and acceptance | Failure / forbidden escape |
|---|---|---|---|
| immutable frozen B7 candidate | B8 Canonical verifier/publication gate only | exact epoch/module/function digest, plan/options fingerprint, ordinal-7 stamp and occurrence lineage | no B6 alias, editor, partial prefix or reconstruction substitutes |
| cumulative properties | B8 complete Canonical profile | Raw + every P01-P06 property retained; framework establishes `IntrinsicsCanonical` | P07 cannot mint capability/report/`CanonicalBir` |
| canonical intrinsic/call graph | B8 verification and later immutable consumers | registered target-independent semantic IDs/signatures/immediates/effects/calls | no helper route, target availability, ABI location or ISA identity |
| opaque `InlineAsm` nodes | B8 and C9 later interpretation | one ordinary typed node; template/constraints/clobber spellings/effects preserved byte-for-byte | no parsed/bound/parallel asm graph or edge-specific result |
| occurrence `MutationSummary` | pass/analysis frameworks | exact call/intrinsic/effect/use/declaration/module effects | declared preservation cannot override changes |
| invalidation outcome | analysis manager/B8 adjacency | call graph and memory-effects results exact-current under final B7 keys or evicted | no stale B6 handle becomes B8 proof |
| structured diagnostics | pipeline observation only | stable registry/rule/entity/call/intrinsic anchor and deterministic order | cannot authorize partial candidate or B8 |

B8 accepts only this exact frozen B7 candidate and reruns the full Canonical
profile on the same owning revision. P07 cannot return `CanonicalBir`, and a
green P07-local report cannot substitute for B8 publication.

## Adjacent-Stage Contract

[P06 aggregate](../aggregate/README.md) supplies the exact B6 wave.
[Call graph](../../analysis/call_graph/README.md) supplies the exact module/body
facts. [Memory effects](../../analysis/memory_effects/README.md) is read-only
design authority for exact-key reuse/recompute: after P06 mutation, P07 accepts
only a validator-proven B6-key installation or fresh computation; after a true
no-op, unchanged-key reuse is allowed.

The [pass framework](../README.md) owns transaction/property support, the
[pipeline](../../pipeline/README.md) owns ordered B8 adjacency, and the
[verifier](../../verify/README.md) owns the complete Canonical acceptance gate.
P07 produces only the candidate admitted to those distinct B8 authorities.

## Ordered Behavior

1. Validate exact B6 keys/properties/P06 postconditions, call graph, complete
   B6-key effects set and registry version.
2. Inventory every intrinsic, helper-eligible semantic operation, call,
   variadic bundle and inline-asm node in stable module/function/entity order.
   Assign exactly one closed matrix disposition.
3. Prove registered transformations from typed semantic identity plus `Known`
   call/effect facts. Preserve valid unknown rows; accumulate rejects before edit.
4. Fork one private whole-module occurrence transaction, reserve deterministic
   IDs and build the complete replacement/use/registry/effect repair plan.
5. Apply typed atomic RAUW and complete def-use/origin repair without changing
   CFG/SSA/memory/aggregate semantics or opaque asm bytes. Derive mutation
   effects from actual edits.
6. Run cumulative Raw+P01+P02+P03+P04+P05+P06+P07 postconditions and the
   configured verifier-on-commit over the entire candidate.
7. Atomically freeze/publish the B7 candidate and establish
   `IntrinsicsCanonical` only on complete success; otherwise discard it and
   retain B6/cache unchanged.

## Invariants

- `IntrinsicId` plus exact `RegistryVersion` is semantic identity. Names,
  renderer text, pointers and host registry state are not.
- Portable feature requirements and helper eligibility remain semantic
  metadata; P07 never queries a target or selects a route.
- Calls/variadic bundles retain typed semantic parameter/result positions and
  effects without ABI locations or physical varargs state.
- `InlineAsm` remains exactly one ordinary-value opaque semantic node. Generic
  inputs/results use core def-use; template/constraint/clobber bytes are exact.
- Paired asm-goto results remain ordinary definitions available on every typed
  successor. P07 never invents edge-specific availability.
- No P01-P06 form, target fact, allocation state or preparation result appears.

## Exhaustive P07 Intrinsic/Call/Asm Disposition Matrix

| Applicable B6 input form | Lossless facts / precondition | Disposition | Exact P07 result or stable failure | B8 visibility | Implementation |
|---|---|---|---|---|---|
| empty module, declaration or empty function | exact empty member/registry state | Preserve | unchanged; no failure | B8 sees exact empty member | absent |
| canonical registered intrinsic | exact registry version/ID/signature/immediates/effects | Preserve | unchanged; no failure | B8 sees canonical intrinsic | absent |
| registered semantic intrinsic alias | exact registry alias and typed operand/result mapping | Normalize | canonical `IntrinsicId` node; else `IntrinsicAliasUnproved` | B8 sees canonical intrinsic | absent |
| intrinsic registry version mismatch | module version cannot interpret ID under active build registry | Reject | `IntrinsicRegistryVersionMismatch` | no B7 candidate | absent |
| unknown/unregistered intrinsic ID | no closed semantic entry exists | Reject | `IntrinsicUnknownId` | no B7 candidate | absent |
| ISA/target-specific intrinsic identity | target-independent registry forbids machine identity | Reject | `IntrinsicTargetIdentity` | no B7 candidate | absent |
| canonical intrinsic signature | exact typed generic operands/results and declaration | Preserve | unchanged; no failure | B8 sees exact signature | absent |
| admitted signature/operand-role alias | registered lossless typed mapping | Normalize | canonical signature/roles; else `IntrinsicSignatureMismatch` | B8 sees canonical signature | absent |
| canonical immediate operand | exact registered type/range/semantic value | Preserve | unchanged; no failure | B8 sees exact immediate | absent |
| noncanonical immediate encoding | exact registered semantic value | Normalize | canonical immediate; else `IntrinsicImmediateInvalid` | B8 sees canonical immediate | absent |
| invalid/out-of-range immediate | registry cannot represent semantic value | Reject | `IntrinsicImmediateInvalid` | no B7 candidate | absent |
| portable feature requirement metadata | exact registry semantic requirement, no target decision | Preserve | unchanged; no failure | B8 sees portable requirement | absent |
| target feature availability/result attached | target query authority appears before C1 | Reject | `IntrinsicForbiddenAuthority` | no B7 candidate | absent |
| canonical helper-eligible semantic operation | exact portable semantic identity and eligibility only | Preserve | unchanged; no failure and no helper selected | B8 sees semantic operation | absent |
| admitted helper-eligible semantic alias | registered semantic equivalence independent of route | Normalize | canonical semantic operation; else `IntrinsicHelperSemanticUnproved` | B8 sees eligibility only | absent |
| selected runtime-helper call/route | helper selection belongs solely to C8 | Reject | `IntrinsicHelperRouteForbidden` | no B7 candidate | absent |
| canonical direct semantic call | exact call ID/callee/signature/bundle/effects | Preserve | unchanged; no failure | B8 sees semantic call | absent |
| admitted direct-call descriptor alias | exact callee/signature/bundle mapping | Normalize | canonical call descriptor; else `IntrinsicCallUnproved` | B8 sees canonical call | absent |
| direct call inconsistent with exact call graph | call/callee facts disagree | Reject | `IntrinsicCallGraphMismatch` | no B7 candidate | absent |
| finite-set indirect call | exact typed callee operand and proven graph set | Preserve | unchanged; no failure | B8 sees semantic indirect call | absent |
| unknown-indirect call | valid typed call and `Unknown(IndirectCalleeUnproved)` | Preserve | unchanged conservatively; no failure | B8 sees typed unknown call | absent |
| canonical variadic semantic call | exact fixed/variadic typed arguments and bundle boundary | Preserve | unchanged; no failure and no ABI placement implied | B8 sees semantic variadic call | absent |
| admitted variadic bundle alias | exact typed semantic argument/order mapping | Normalize | canonical variadic bundle; else `IntrinsicVariadicInvalid` | B8 sees canonical bundle | absent |
| physical `va_list`/register-save/stack placement attached | ABI realization belongs after Canonical | Reject | `IntrinsicVariadicAbiForbidden` | no B7 candidate | absent |
| canonical semantic call bundle | exact closed typed bundle operands/effects | Preserve | unchanged; no failure | B8 sees canonical bundle | absent |
| noncanonical declared effect ordering | exact registry/call semantic effect set | Normalize | canonical ordered/deduplicated effects; else `IntrinsicEffectMismatch` | B8 sees canonical effects | absent |
| effect narrowing unsupported by exact memory effects | `Unknown` or conservative effect cannot prove narrowing | Preserve | retain conservative effects; no failure | B8 sees conservative semantics | absent |
| effect descriptor contradicting registry/type semantics | no lossless registered reconciliation | Reject | `IntrinsicEffectMismatch` | no B7 candidate | absent |
| P07-owned atomic intrinsic alias | exact atomic operation/order/scope semantics assigned to registry | Normalize | canonical registered intrinsic form; else `IntrinsicAtomicInvalid` | B8 sees canonical semantic atomic | absent |
| P05 canonical memory/atomic node | earlier owner already canonical and not P07 registry form | Preserve | unchanged; no failure | B8 sees P05 form | absent |
| canonical opaque `InlineAsm` ordinary node | exact typed generic edges and byte-preserved payload/constraints/clobbers/effects | Preserve | unchanged; no failure | B8 sees one opaque node | absent |
| admitted inline-asm payload-shape alias | exact bytes and generic typed edges; no interpretation | Normalize | canonical opaque container only; else `IntrinsicAsmShapeInvalid` | B8 sees byte-identical payload | absent |
| inline-asm template/constraint/clobber rewrite request | P07 cannot parse, normalize or bind text | Reject | `IntrinsicAsmInterpretationForbidden` | no B7 candidate | absent |
| asm-goto ordinary result and typed successors | exact generic definition and terminator-owned topology | Preserve | unchanged; no failure | B8 sees ordinary result on all successors | absent |
| edge-specific asm result availability/payload | alternate graph is not canonical semantics | Reject | `IntrinsicAsmEdgeAvailabilityForbidden` | no B7 candidate | absent |
| earlier P01-P06 noncanonical form recreation required | P07 cannot repair/recreate earlier stage | Reject | `IntrinsicEarlierFormRequired` | no B7 candidate | absent |
| name/text/pointer/legacy-derived intrinsic or call identity | no stable registry/semantic proof | Reject | `IntrinsicForbiddenIdentity` | no B7 candidate | absent |
| unfamiliar valid intrinsic/call/asm form with no owner | registry has no closed disposition | Reject | `IntrinsicUnknownForm` | no B7 candidate | absent |

Every form has exactly one disposition. `Unknown` call/effect facts preserve
only the explicitly conservative rows; they never authorize narrowing, helper
selection or target inference. Every Reject aborts the complete occurrence.

## Verification and Publication

The exact private P07 output must satisfy:

- cumulative Raw and P01 `TypesLegal`, P02 `ScalarsCanonical`, P03
  `CfgCanonical`, P04 `SsaCanonical`, P05 `MemoryCanonical`, P06
  `AggregatesCanonical` and P07 `IntrinsicsCanonical` postconditions;
- every intrinsic/helper-eligible/call/variadic/asm form has one closed result;
- exact registry IDs/versions/signatures/immediates/effects, stable IDs, typed
  def-use and opaque asm bytes remain exact;
- all P01-P06 CFG/SSA/memory/aggregate semantics remain canonical;
- no helper route, target feature result, ABI/constraint interpretation,
  preparation or allocation fact exists;
- a second P07 occurrence proposes no mutation.

Only the framework freezes/publishes the B7 candidate and establishes
`IntrinsicsCanonical`. Only B8's full same-revision Canonical verifier may mint
`CanonicalBir`.

## Failure and Diagnostics

Stable failures include `IntrinsicWrongInput`, `IntrinsicStaleInput`,
`IntrinsicP06ContractFailure`, `IntrinsicStaleAnalysis`,
`IntrinsicRegistryInvalid`, `IntrinsicForbiddenAuthority`, every matrix
failure, `IntrinsicPreservedFormChanged`, `IntrinsicNonconvergent`,
deterministic resource exhaustion, cancellation, verifier rejection and
transaction/occurrence publication failure.

Diagnostics carry exact checkpoint/analysis/registry keys and stable module/
function/instruction/value/call/intrinsic/rule anchors. Failure rolls back the
complete module wave and publishes no function prefix, call/intrinsic/use
repair, revision, property, candidate analysis/cache entry, B7 candidate or
publication capability.

## Analysis and Invalidation

Any committed call target/bundle, intrinsic identity/signature/immediate/effect,
definition/use, declaration, symbol/visibility or module/body revision change
invalidates call graph, memory effects, provenance, publication/value-flow and
all transitive dependents observing it. Old B6 handles are stale after revision
increment; P07 cannot patch or retarget them.

After commit, exact-B7 call graph/effects are installed only by checked
preservation or recomputation under final keys; otherwise they are evicted.
Candidate-local facts die on rollback. B8 receives the frozen B7 candidate and
does not treat any analysis handle as the Canonical capability.

## Target and ABI Rules

P07 has no target feature result/layout, helper route, ABI/varargs placement,
constraint binding, preparation, home, spill, frame or MIR input. It preserves
portable semantic requirements/eligibility but never decides availability or
realization. Inline-asm payload is interpreted only later at C9.

## Implementation State

Implementation is absent. No checked-in `PassId::IntrinsicCanonicalize`
implementation/registration, registry reconciliation route, module transaction,
analysis integration, verifier hook, build target or runtime proof exists. The
matrix is design authority, not implementation coverage.

## Proof Requirements

- verify metadata/core-first order and substantive input/output matrices;
- test exact B6 call-graph/effects keys, stale/unknown forms and explicit
  effects reuse/recompute decision;
- cover every intrinsic/helper/call/variadic/asm disposition;
- prove opaque asm bytes, stable IDs, atomic rollback, cumulative P01-P07
  postconditions and mutation-derived invalidation;
- prove B8 accepts exactly one stamped B7 candidate and P07 cannot mint
  `CanonicalBir`;
- reconcile implementation/build truth.

## Open Questions

None for v1. New intrinsic/call/asm families require registered ownership.

## Review Checklist

- [x] Exact B6 call graph and memory-effects inputs are required.
- [x] Memory-effects reuse/recompute is exact-key and stale-safe.
- [x] Every intrinsic/helper/call/variadic/asm form has one disposition.
- [x] InlineAsm remains one opaque ordinary-value node.
- [x] Helper/target/ABI/constraint decisions are excluded.
- [x] Atomic rollback, cumulative postconditions and invalidation are explicit.
- [x] B8 receives exactly one stamped B7 candidate; P07 cannot publish Canonical.
- [x] Implementation truth is absent.
