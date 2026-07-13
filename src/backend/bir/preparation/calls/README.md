# Immutable Call Requirement Plan Contract

Contract-Status: under-review
Implementation-Status: absent
Kind: product
Phase-ID: C4
Upstream: exact verifier-bound Canonical/C1 input, matching C2 `VerifiedTargetLayout`, exact C3 `AbiPlan`, and exact-current `CallGraph`
Downstream: one immutable exact-key `CallPlan` consumed first by C5 variadic preparation and later by D2 call lowering
Owner-Path: `src/backend/bir/preparation/calls/README.md`
Last-Reconciled-Commit: `954166281`

## Purpose

C4 is the sole preparation owner for target-aware per-call semantic
requirements. It binds every Canonical call or helper-eligible semantic
operation to the exact C1-C3 products and exact-current call semantics, then
atomically publishes one immutable `CallPlan`. The plan describes typed inputs,
outputs, preservation, clobber, return, tail and abstract outgoing-object
requirements only. It performs no call transport or lowering, selects no
helper, target opcode, ABI location or value home, and creates no frame state.

## Owns

- stable call-boundary/requirement IDs and the closed C4 call-form schema;
- exhaustive direct, finite-indirect, unknown-indirect, variadic, intrinsic and
  helper-eligible semantic dispositions;
- per-call typed input/output, simultaneous-transfer, byval/sret, abstract
  preservation/clobber, return-recovery and tail-shape requirements;
- complete `CallPlanKey`/fingerprint, private verifier, structured failure,
  atomic publication and transitive invalidation; and
- the sole exact-key `CallPlan` capability accepted first by C5 and later by
  D2.

## Does Not Own

- C1 target selection, verifier binding, C2 layout, C3 ABI classification or
  computation/publication of `CallGraph`;
- creation or mutation of calls, callees, operands, results, effects, stable
  IDs, def-use, CFG or Canonical storage;
- a parallel move sequence, concrete or abstract ABI slot, register/unit,
  outgoing stack offset, store, call instruction, result move, save/restore,
  home, spill, frame object/offset or stack adjustment;
- runtime-helper eligibility/interface/symbol selection, variadic entry/save-
  area/traversal planning, pseudo lowering, target legalization, MIR or
  emission; or
- reinterpretation of inline assembly as a call edge or use of names, text,
  pointers, renderer coordinates or legacy routes as semantic authority.

## Inputs

C4 receives one frozen exact Canonical/target/layout/ABI tuple and one
exact-current module-scope `CallGraph` handle. The analysis remains
`CanonicalSemantic`, with target-layout key `None`, empty preparation digest,
empty dependencies and canonical empty options; C4 does not retag it under the
target-aware call-plan key.

### Exact C4 input matrix

| Input / product | Exact required state | Optional / empty form | Failure / forbidden substitution |
|---|---|---|---|
| preparation input | verifier-issued borrowing `VerifiedPreparationInput` for this exact Canonical owner and target fingerprint | empty module remains valid | copied report, foreign/stale binding or reconstructed view is `CallPlanBindingInvalid` |
| Canonical view/stamp | immutable B8 `CanonicalBir` with exact epoch/module/function digest, plan/options lineage, ordinal 7 and all P01-P07 properties | declaration-only or no-call module valid | Raw/candidate/view substitute, mixed revision or stale stamp is `CallPlanCanonicalInvalid` |
| validated target | exact immutable C1 `TargetProfile` and complete `TargetFingerprint` named by the binding | none | architecture/triple/ABI-only similarity or default target is `CallPlanTargetInvalid` |
| verified layout | exact C2 `VerifiedTargetLayout`, complete key and fingerprint under the same Canonical/target binding | known-empty optional class/table valid when ABI fallback permits | compatible table/count, stale key or reconstructed mapping is `CallPlanLayoutInvalid` |
| ABI plan | exact verified C3 `AbiPlan`, complete `AbiPlanKey` and fingerprint for the same Canonical/C1/C2 tuple and ABI registry | empty module plan is valid | reclassification, subset/equal-looking/stale/foreign plan is `CallPlanAbiInvalid` |
| call-graph descriptor | `AnalysisId::CallGraph`, schema 1, module scope, `CanonicalSemantic` | none | unknown/duplicate/schema/domain mismatch is `CallPlanAnalysisInvalid` |
| call-graph handle | exact-current B8 handle keyed by epoch/module revision and ordered complete `(FunctionId, FunctionRevision)` digest, empty dependencies/options, target-layout `None`, preparation empty | no calls is known empty; external/indirect uncertainty remains typed `Unknown(reason)` | missing/foreign/stale handle, partial body digest or implicit recompute is `CallPlanStaleAnalysis` |
| call-rule registry | exact versioned per-call/tail/preservation/clobber rule registry selected by the C1/C3 ABI identity | no extension entries | backend default, environment route, duplicate or mismatched rule is `CallPlanRegistryInvalid` |
| call schema | exact `CallPlanSchemaFingerprint` covering every call form, requirement family and verifier rule | none | missing/newer/older/mixed schema is `CallPlanSchemaInvalid` |
| call options | canonical normalized `CallPlanOptionsFingerprint`; v1 has no free semantic option | canonical empty options only | route/worker/cache policy or tail/helper preference is `CallPlanOptionsInvalid` |
| execution control | deterministic cancellation/resource/fixed-point bound | observer absent is valid | unbounded or nondeterministic policy is `CallPlanResourceInvalid` |

## Outputs

Success publishes exactly one immutable module-wide `CallPlan`. Every table is
an embedded read-only view, not an independently transferable capability.
Failure publishes no plan, entry, requirement, fingerprint, cache record or C5
or D2 fact.

### Exact C4 output matrix

| Embedded output / result | Exact consumer | Required binding | Optional / error form |
|---|---|---|---|
| `CallPlan` capability | C5, C8, D2 and declared later consumers | complete `CallPlanKey`, verifier token and total Canonical call/helper-eligible coverage | valid empty plan when no relevant sites; no partial/equal-looking plan |
| call-site summary | C5 and D2 call identity matching | stable caller `FunctionId`, call `InstId`, form, convention, effect and ordered requirement IDs | no ordinary calls is known empty; malformed/foreign site fails |
| typed input requirements | C5 extra-argument extension and D2 input transport | exact operand/value/type/ordinal and matching C3 requirement IDs | zero inputs known empty; no selected ABI slot or move |
| typed output requirements | D2 result formation | exact result/value/type/ordinal and matching C3 requirement IDs | zero outputs/void explicit `Absent`; incomplete multi-result fails |
| simultaneous-transfer requirements | D2 transport scheduling | closed ordered pieces, equality/overlap constraints and all-or-nothing group identity | no transfer known empty; requirements are not a move sequence or scratch choice |
| outgoing-object requirements | D2 abstract call-object creation and later E4 placement | stable byval/sret/argument object ID, semantic size/alignment/lifetime/access and matching C3 boundary | no object known empty; no stack offset, frame placement or store selected |
| hidden-carrier requirements | C5/D2 hidden input/result handling | exact sret/byval relation, typed object/value identity and matching C3 requirement | no hidden carrier explicit `Absent`; no register/location chosen |
| preservation requirements | D2 call-site preservation planning | exact call ID, ordinary live-across semantic requirement domain and ABI-preserved class obligations | no preservation known empty; no save/restore/value home selected |
| clobber requirements | D2 explicit call clobber formation | exact call effect plus ABI caller-clobbered abstract class/unit-set requirements | no extra clobber known empty; no target opcode or concrete register spelling |
| return requirements | D2 return/recovery planning | returning/tail/noreturn status, ordered ordinary/hidden results and recovery dependencies | void/noreturn/tail forms explicit; no result move selected |
| tail disposition | D2 closed call shape | exact ordinary, optional-tail, required-tail eligibility proof or stable rejection rule | non-tail is explicit `OrdinaryReturn`; required tail without proof fails |
| variadic-call boundary | C5 extension | fixed-count boundary, exact extra operand IDs/types and C3 promotion/classification obligations | nonvariadic explicit `Absent`; no save area/traversal/transport selected |
| deferred helper-call requirement | C8 helper selection and D1/D2 lineage | helper-eligible Canonical operation ID, typed operands/results/effects and exact C3-compatible boundary template | ineligible operation explicit `Absent`; no helper family/interface/symbol selected |
| structured `CallPlanFailure` | caller/diagnostics | stable call/operation/rule/entity/key/cause and deterministic order | report grants no plan, cache, repair or retry authority |

## Adjacent-Stage Contract

The [C3 ABI plan](../abi/README.md) supplies immutable signature and boundary
classification. At C4, the manager requests [CallGraph](../../analysis/call_graph/README.md)
for the exact immutable Canonical revision after all B1-B8 mutations. The
complete module and observed-body digest must match B8; a B6/P07-era handle is
stale even if a preservation validator could prove a new-key result equal.

[C5 variadic preparation](../variadic/README.md) accepts only this complete
same-key `CallPlan` fingerprint together with unchanged C1-C3 capabilities. It
extends fixed/extra boundaries without repairing call forms or transporting
arguments. [C8 helper preparation](../runtime_helpers/README.md) alone selects
an eligible helper interface for a deferred helper requirement.

[D2 shared call lowering](../../passes/call_lowering/README.md) later accepts
the exact C3/C4 fingerprints through the verified cumulative bundle. D2 alone
creates `AbiPreserve`, `AbiArgMove`, `AbiArgStore`, `AbiCall`,
`AbiResultMove` and `AbiRestore` transport. It may not reconstruct C4, change a
tail disposition or substitute a structurally equal plan; C4 creates none of
those nodes and selects no slot, stack displacement, frame offset, machine
opcode or encoding.

## Stable Identity and Exact Product Key

| Key / identity axis | Exact content | Stale / forbidden shortcut |
|---|---|---|
| Canonical identity | complete B8 `PipelineStageStamp`, immutable owner, epoch/module revision and ordered function revisions | module revision, stable IDs or semantic hash alone |
| C1 target identity | complete `TargetFingerprint` and validated profile schema | architecture/triple/backend ABI similarity |
| C2 layout identity | complete `TargetLayoutKey` and `TargetLayoutFingerprint` | compatible capacity/table/mapping version |
| C3 ABI identity | complete `AbiPlanKey` and verified `AbiPlanFingerprint` | copied entries, subset hash or reclassification |
| analysis identity | complete exact-current CallGraph `AnalysisKey` and result fingerprint | B6/P07 handle, partial body digest or implicit new-key substitution |
| call rule identity | calling-convention/per-call/tail/preservation/clobber registry versions | environment or backend-local default |
| plan schema/options | `CallPlanSchemaFingerprint` and normalized semantic options | route, worker, cache or diagnostic configuration |
| `CallRequirementId` | stable caller/call-or-operation/boundary/role/ordinal identity plus requirement-kind ordinal | callee name, pointer, text or vector position without owning IDs |
| `CallPlanFingerprint` | deterministic digest of the full key and every verified output row in stable-ID order | subset/report/equal-looking plan fingerprint |

`CallPlanKey` is the ordered tuple of every axis above. Empty/zero inventories
retain their axes. Every C5-C9 and D2 reference carries the complete key or
fingerprint; an old handle never retargets.

## Exhaustive Call-Form Matrix

`Plan` publishes requirements without mutating input. `Absent` is a legal
non-applicable form, `Defer` is an explicitly owned later decision, and
`Reject` aborts the whole plan. Unknown call-graph facts never authorize a
guessed callee or weakened ABI boundary.

| Canonical call / operation form | Exact facts consumed | Disposition | Exact requirement / stable failure | Later authority |
|---|---|---|---|---|
| empty or no-call module | exact empty call graph and helper-eligible inventory | Absent | one valid empty `CallPlan` | C5 sees known-empty plan |
| direct internal call | exact call site, callee `FunctionId`, signature/effects and matching C3 summary | Plan | complete typed boundary and direct callee identity | D2 transports exactly this call |
| direct declaration/external call | exact site, declaration/symbol/linkage/visibility and typed signature | Plan | external direct boundary plus conservative declared/unknown effects | D2 emits call; no symbol guess |
| proven finite-set indirect call | exact callee operand and complete ordered proven callee set | Plan | one common compatible typed boundary plus retained finite set/proof | D2 emits indirect call; no target chosen |
| unknown-indirect call with registered ABI fallback | exact `Unknown(IndirectCalleeUnproved)`, typed signature and rule permitting conservative boundary | Plan | conservative complete input/output/effect/clobber requirements | D2 remains indirect |
| unknown-indirect call without legal ABI fallback | exact unknown reason/signature and absent rule | Reject | `CallPlanIndirectUnproved` | no plan; never guess a finite set |
| zero-input call | exact empty operand order and matching callee/signature | Absent | known-empty typed input table | no argument transport |
| void or zero-output returning call | exact void signature and returning control effect | Absent | explicit no-result/ordinary-return requirement | D2 emits no recovery requirement |
| single-result call | exact result ID/type/ordinal and C3 result requirement | Plan | one typed output and return-recovery requirement | D2 owns result transport |
| multi-result call | exact complete ordered result IDs/types and all matching C3 requirements | Plan | closed all-results requirement group | D2 cannot accept a prefix |
| by-value argument call | exact operand/object/type plus C3 byval size/alignment/copy/lifetime requirement | Plan | abstract outgoing-object and typed input requirement | D2 creates abstract object/store; E4 places it |
| hidden-sret result call | exact result/object identity plus C3 hidden-result relation | Plan | hidden-carrier and memory-result recovery requirements | D2 creates transport; no carrier selected here |
| ordinary non-tail call | exact returning semantics and absent tail request | Plan | `OrdinaryReturn` plus return/preservation requirements | D2 emits returning shape |
| optional tail candidate proved | exact call/caller/callee boundary, control shape and registered complete proof | Plan | `TailEligible` closed tail requirements | D2 may use only recorded tail shape |
| optional tail candidate rejected | exact stable failed proof with ordinary call semantically legal | Plan | `OrdinaryReturn` plus stable non-tail reason | D2 cannot re-prove or upgrade |
| required tail call proved | exact required-tail marker and complete eligibility proof | Plan | `TailRequired` closed no-return-to-caller shape | D2 must preserve tail form |
| required tail call unproved | exact required-tail marker and failed/unknown proof | Reject | `CallPlanRequiredTailUnproved` | no ordinary-call downgrade |
| nonvariadic call | exact nonvariadic signature | Absent | no variadic boundary | C5 records non-applicability |
| variadic call with no extras | exact fixed-count boundary and empty extra operand suffix | Plan | fixed boundary plus known-empty extra set | C5 owns entry/save/traversal extension |
| variadic call with extras | exact fixed/extra split, ordered typed extra operands and C3 promotion obligations | Plan | complete extra-argument requirement group | C5 extends; D2 later transports |
| call-like intrinsic retained by P07 | exact intrinsic call site/identity/signature/effects and C3-compatible boundary | Plan | intrinsic identity plus complete ordinary call requirements | D2 treats only admitted call form |
| non-call intrinsic | exact call graph non-edge classification and intrinsic semantic identity | Absent | explicit no-call-plan entry | C4 does not fabricate a call |
| helper-eligible semantic operation | exact stable operation descriptor/operands/results/effects and call-graph non-call status | Defer | typed `DeferredHelperCallRequirement` with no interface/symbol | C8 alone selects; D1 forms `GenericCall` |
| inline assembly | exact call-graph non-edge observation | Absent | explicit non-call status | inline-assembly owners retain authority |
| missing/malformed call type, signature, bundle, operand or result | exact stable site/entity and invalid semantic fact | Reject | `CallPlanSemanticInputInvalid` | no plan or downgrade to unknown |
| ABI/signature/call-graph disagreement | exact C3 boundary and exact analysis site that do not agree | Reject | `CallPlanAbiAnalysisMismatch` | neither predecessor is repaired |
| unsupported convention/effect/tail/helper template | exact stable rule and unsupported form | Reject | `CallPlanFormUnsupported` | no backend fallback or helper selection |

Every relevant Canonical call and helper-eligible operation has exactly one
matrix disposition. Parallel call sites remain separate by `InstId`; equal
caller/callee pairs never collapse.

## Ordered Behavior

1. Validate and freeze the exact Canonical/C1/C2/C3 capabilities, rule/schema/
   options identities and deterministic execution policy.
2. Request `CallGraph` after B8; require its complete exact-current module/body
   key, empty dependencies/options and target/preparation exclusions.
3. Inventory every ordinary call, call-like/non-call intrinsic and helper-
   eligible semantic operation in stable Canonical order.
4. Join each site to exact graph facts and exact C3 signature/boundary rows;
   classify it through exactly one exhaustive matrix disposition.
5. Stage typed input/output, simultaneous-group, object, hidden-carrier,
   preservation/clobber, return/tail, variadic and deferred-helper requirements.
6. Verify total coverage, stable identity uniqueness, type/signature/effect
   agreement, finite-set compatibility, fallback legality, all-or-nothing
   groups and complete key/fingerprint coverage.
7. Recheck every capability and analysis key, then atomically publish one
   immutable `CallPlan`; otherwise discard all private rows and publish none.

## Invariants

- C4 is requirements-only. A simultaneous transfer group is not a move list;
  an outgoing-object requirement is not a stack offset or frame object.
- C3 classifications are referenced exactly and never recreated, weakened or
  translated into concrete locations.
- Unknown indirect calls remain unknown; helper-eligible operations remain
  interface-free until C8; inline assembly remains a non-call.
- Canonical, preparation binding, layout, ABI plan and analysis handles remain
  immutable and unchanged.
- Names, spellings, pointers, dense indices, renderer text and legacy route
  records never establish call identity, freshness or support.

## Verification, Failure, and Invalidation

The private C4 verifier checks the candidate against the frozen exact key:
capability equality, CallGraph freshness, total call/operation coverage,
stable-ID uniqueness, signature/type/effect agreement, exact C3 references,
finite indirect compatibility, unknown fallback legality, complete inputs and
outputs, group closure, object/hidden-carrier consistency, preservation/
clobber/return/tail rules, deferred-helper non-selection and deterministic
ordering.

Any stale/missing/mixed key, stale analysis, malformed call, incomplete
operand/result/group coverage, ABI/analysis mismatch, unsupported convention,
illegal indirect/tail/variadic/intrinsic/helper form, cancellation or resource
failure discards the whole candidate. Failure publishes no plan, entry,
requirement, fingerprint, cache record, C5 fact, helper fact or D2 fact; every
input remains unchanged.

Any Canonical stamp/body revision, target/profile schema, layout key, C3 plan
key/fingerprint, CallGraph result key/fingerprint, call-rule/schema/options or
observed call/operation identity change invalidates the complete plan and all
C5-C9 successors. Old handles never retarget or implicitly recompute; a
checked complete new-key plan is required even when facts compare equal.

## Target, Helper, and Lowering Rules

C4 may use C2 eligibility and exact C3 classifications only to state per-call
semantic requirements. It cannot select a class alternative's slot, concrete
register/unit, helper family/interface/symbol, target opcode, stack offset,
home, scratch, spill, frame action, instruction sequence or encoding. C8 owns
helper selection, D1 owns generic call formation, D2 owns shared ABI transport,
D4 owns target legalization and E4 owns final frame placement/actions.

## Implementation State

Implementation is absent. This directory contains only this README and has no
build edge. No checked-in `CallPlan`, key/fingerprint/requirement IDs, call-
form registry, exact-current CallGraph request route, verifier, transaction,
cache, C5/C8 handoff, D2 consumer path or focused runtime proof exists.

Legacy `call_plans.*`, preallocation call moves, formal/publication routes,
decoded homes, call-return ABI helpers and target-private call code are
migration evidence only. They do not consume the exact Canonical/C1/C2/C3/
analysis tuple, publish this immutable key or satisfy C4.

## Proof Requirements

- prove exact Canonical/C1/C2/C3 and exact-current CallGraph keys, B8 timing,
  target/preparation-none domain and stale rejection;
- prove direct/finite-indirect/unknown/variadic/intrinsic/helper-eligible plus
  empty, optional, tail, byval/sret and malformed forms are exhaustive;
- prove complete typed input/output, preservation/clobber/return requirements,
  requirements-only output, atomic failure and transitive invalidation;
- prove C5 accepts only the complete same-key product, C8 alone selects helpers
  and D2 alone creates call transport/lowering; and
- reconcile implementation claims with directory contents, build inclusion
  and callable reachability.

## Open Questions

None. New call forms, indirect proof families, tail rules or helper templates
require versioned registry/schema updates and coordinated C3-C5/C8/D2 review.

## Review Checklist

- [x] Metadata and core-first ownership are explicit.
- [x] Exact input/output matrices and complete product key are closed.
- [x] CallGraph is requested exact-current at C4 and stays target-less.
- [x] Every direct/indirect/unknown/variadic/intrinsic/helper form is explicit.
- [x] Typed input/output, preservation/clobber/return requirements are complete.
- [x] No transport, lowering, helper/opcode/location/home/frame choice appears.
- [x] Failure/invalidation are atomic and C5/C8/D2 boundaries are exact.
- [x] Absent implementation truth is explicit.
