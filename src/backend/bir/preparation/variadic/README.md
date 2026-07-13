# Immutable Variadic Requirement Plan Contract

Contract-Status: under-review
Implementation-Status: absent
Kind: product
Phase-ID: C5
Upstream: exact verifier-bound Canonical/C1 input, matching C2 `VerifiedTargetLayout`, exact C3 `AbiPlan`, and exact C4 `CallPlan`
Downstream: one immutable exact-key `VariadicPlan` consumed first by C6 address preparation and later by D2 call lowering
Owner-Path: `src/backend/bir/preparation/variadic/README.md`
Last-Reconciled-Commit: `1468f5a62`

## Purpose

C5 is the sole preparation owner for target-aware variadic entry, call,
promotion, save-area and `va_list` traversal requirements. It binds every
variadic definition, call boundary and `va_start`/`va_copy`/`va_arg`/`va_end`
semantic operation to the exact C1-C4 capabilities, then atomically publishes
one immutable `VariadicPlan`. The plan states semantic requirements only: it
does not place a save area, choose a register, transport a value, emit a helper
or instruction, mutate a list object, or create frame state.

## Owns

- stable variadic function/call/operation/requirement IDs and one closed C5
  schema;
- exact fixed/named versus extra/unnamed boundaries and default-promotion
  obligations inherited from C3/C4;
- variadic-entry, abstract register-save-area and overflow-domain semantic
  requirements;
- `va_list` state, initialization, copy, typed traversal, exhaustion/fallback,
  alignment and lifetime requirements;
- the complete `VariadicPlanKey`/fingerprint, private verifier, structured
  failure, atomic publication and transitive invalidation; and
- the sole exact-key `VariadicPlan` capability accepted first by C6 and later
  by D2.

## Does Not Own

- C1 target selection, verifier binding, C2 layout/capacity, C3 ABI
  classification, C4 per-call requirements or any predecessor reconstruction;
- function/call/operation/value/object identities, signatures, types, def-use,
  CFG, Canonical storage or language-level insertion of default promotions;
- a physical or abstract register selection, concrete save-area object,
  base/offset/size placement, GP/FP cursor offset, overflow stack address,
  frame object/offset, stack adjustment, home, spill or scratch;
- argument/register saves, loads/stores, call transport, prologue/epilogue,
  helper selection/lowering, pseudo/target instruction selection, MIR or
  emission; or
- mutation of a `va_list` object, predecessor product or input handle, and use
  of names, rendered types, text, pointers or legacy routes as authority.

## Inputs

C5 receives one frozen exact Canonical/target/layout/ABI/call tuple. C3 owns
semantic classification and promotion obligations; C4 owns exact call-site
fixed/extra boundaries. C5 references those verified rows and never
reclassifies a value or reconstructs a call plan.

### Exact C5 input matrix

| Input / product | Exact required state | Optional / empty form | Failure / forbidden substitution |
|---|---|---|---|
| preparation input | verifier-issued borrowing `VerifiedPreparationInput` for this exact Canonical owner and target fingerprint | empty module remains valid | copied report, foreign/stale binding or reconstructed view is `VariadicPlanBindingInvalid` |
| Canonical view/stamp | immutable B8 `CanonicalBir` with exact epoch/module/function digest, plan/options lineage, ordinal 7 and all P01-P07 properties | no variadic definitions/calls/operations is valid | Raw/candidate/view substitute, mixed revision or stale stamp is `VariadicPlanCanonicalInvalid` |
| validated target | exact immutable C1 `TargetProfile` and complete `TargetFingerprint` named by the binding | none | architecture/triple/ABI similarity or default target is `VariadicPlanTargetInvalid` |
| verified layout | exact C2 `VerifiedTargetLayout`, complete key/fingerprint, eligible class/group domains, capacities, aliases and ABI eligibility under the same binding | optional class may be `Absent`; capacity may be known zero | compatible count/table, inferred register file, stale key or reconstructed mapping is `VariadicPlanLayoutInvalid` |
| ABI plan | exact verified C3 `AbiPlan`, complete key/fingerprint, function/parameter/aggregate/variadic classification and promotion obligations | empty/nonvariadic rows remain explicit | reclassification, subset/equal-looking/stale/foreign plan is `VariadicPlanAbiInvalid` |
| call plan | exact verified C4 `CallPlan`, complete key/fingerprint, call identity and fixed/extra typed boundaries | no calls is known empty | recreated boundary, subset/equal-looking/stale/foreign plan is `VariadicPlanCallInvalid` |
| variadic rule registry | exact ABI-selected entry/save-area/list-layout/promotion/traversal rule versions consistent with C1-C4 | no extension entries | backend default, environment route, duplicate/mixed rule is `VariadicPlanRegistryInvalid` |
| variadic schema | exact `VariadicPlanSchemaFingerprint` covering every requirement family and verifier rule | none | missing/newer/older/mixed schema is `VariadicPlanSchemaInvalid` |
| variadic options | canonical normalized `VariadicPlanOptionsFingerprint`; v1 has no free semantic option | canonical empty options only | route/worker/cache/helper or placement preference is `VariadicPlanOptionsInvalid` |
| execution control | deterministic cancellation/resource/fixed-point bound | observer absent is valid | unbounded or nondeterministic policy is `VariadicPlanResourceInvalid` |

## Outputs

Success publishes exactly one immutable module-wide `VariadicPlan`. All tables
are embedded read-only views and never independent capabilities. Failure
publishes no plan, entry, requirement, fingerprint, cache record, C6 fact or
D2 fact.

### Exact C5 output matrix

| Embedded output / result | Exact consumer | Required binding | Optional / error form |
|---|---|---|---|
| `VariadicPlan` capability | C6, D2 and declared later consumers | complete `VariadicPlanKey`, verifier token and total relevant-definition/call/operation coverage | valid empty plan when no variadic semantics; no partial/equal-looking plan |
| variadic-function entry summary | C6 lineage and later D2 entry formation | stable `FunctionId`, convention, fixed parameter count, named/unnamed boundary and ordered requirement IDs | declaration has no entry body; nonvariadic explicit `Absent` |
| fixed/named boundary requirements | C6 and D2 | exact fixed parameter IDs/types and matching C3 `FixedVariadic` requirements | zero fixed parameters known empty; mismatch fails |
| extra/unnamed call requirements | D2 call transport | exact C4 call/operand IDs, typed extra suffix and matching C3 promotion/classification IDs | variadic call with zero extras known empty; nonvariadic explicit `Absent` |
| default-promotion requirements | verifier/D2 lineage | original semantic type, already-promoted call type, registered promotion rule and matching C3/C4 obligation | no promotion required explicit `Identity`; missing/wrong promotion fails |
| register-save-area semantic requirements | D2 entry formation and later E4 placement | stable abstract region ID, eligible class/group domains, semantic capacity/alignment/lifetime/access and exhaustion rule | no eligible register domain known empty; no register, byte offset or placement selected |
| overflow-domain requirements | D2 and C6 address lineage | stable abstract overflow-domain ID, semantic alignment/progression and legal class-exhaustion fallback | no overflow use known empty; no concrete stack address/offset selected |
| `va_list` state schema | D2 semantic-operation formation | exact ABI rule, typed list object domain, abstract region/cursor/status fields and state transitions | ABI with no supported list form fails; no storage layout/offset emitted |
| `va_start` requirements | D2 semantic operation | stable operation/list-object/function IDs, uninitialized-state precondition and exact named/unnamed boundary token | no operation known empty; nonvariadic/mismatched boundary fails |
| `va_copy` requirements | D2 semantic operation | stable source/destination list IDs, source live-state and independent destination lifetime relation | no operation known empty; uninitialized/aliased-illegal source fails |
| `va_arg` traversal requirements | D2 semantic operation | stable operation/list/result IDs, requested semantic type/layout, C3 class, ordered region tests, alignment/progression and state successor | no operation known empty; unsupported type/path/exhaustion fails |
| `va_end` requirements | D2 semantic operation | stable operation/list ID, live-state precondition and terminal lifetime transition | no operation known empty; double/end-before-start fails |
| traversal/lifetime graph | verifier and D2 ordering | stable list-state IDs, Canonical control/value relations, initialization/copy/use/end transitions and complete use coverage | no list objects known empty; ambiguous/unclosed state fails |
| structured `VariadicPlanFailure` | caller/diagnostics | stable function/call/operation/object/rule/key/cause and deterministic order | report grants no plan, cache, repair or retry authority |

## Adjacent-Stage Contract

The [C3 ABI plan](../abi/README.md) supplies exact variadic classification and
promotion obligations. The [C4 call plan](../calls/README.md) supplies each
call's exact fixed/extra typed boundary. C5 consumes their complete same-key
fingerprints with the unchanged C1/C2 capabilities; it cannot recreate a
boundary, insert a missing promotion or accept a structurally equal product.

[C6 address preparation](../address/README.md) accepts only this complete
same-key `VariadicPlan` fingerprint with exact ordered C3/C4 predecessors. It
may plan address-materialization requirements involving abstract variadic
objects, but cannot place a save area, repair traversal or mutate a `va_list`.

[D2 shared call lowering](../../passes/call_lowering/README.md) later consumes
the exact C3-C5 fingerprints through the verified cumulative bundle and alone
creates variadic transport and semantic-operation pseudos. E4 alone chooses
frame bases/offsets and materializes frame actions. C5 creates no BIR node,
register save, load/store, stack adjustment, helper call or frame object.

## Stable Identity and Exact Product Key

| Key / identity axis | Exact content | Stale / forbidden shortcut |
|---|---|---|
| Canonical identity | complete B8 `PipelineStageStamp`, immutable owner, epoch/module revision and ordered function revisions | module revision, stable ID set or semantic hash alone |
| C1 target identity | complete `TargetFingerprint` and validated profile schema | architecture/triple/backend ABI similarity |
| C2 layout identity | complete `TargetLayoutKey` and `TargetLayoutFingerprint` | compatible capacities, class tables or mapping version |
| C3 ABI identity | complete `AbiPlanKey` and verified `AbiPlanFingerprint` | copied classifications, subset hash or recomputation |
| C4 call identity | complete `CallPlanKey` and verified `CallPlanFingerprint` | recreated fixed/extra rows or structurally equal call plan |
| variadic rule identity | ABI-selected entry/save-area/promotion/list/traversal registry versions | environment, backend-local default or rendered layout |
| plan schema/options | `VariadicPlanSchemaFingerprint` and normalized semantic options | route, worker, cache, helper or placement configuration |
| `VariadicRequirementId` | stable function/call/operation/list-object/boundary/role/ordinal plus requirement-kind ordinal | name, pointer, text, offset or dense position without owning IDs |
| `VariadicPlanFingerprint` | deterministic digest of the full key and every verified output row in stable-ID order | subset/report/equal-looking plan fingerprint |

`VariadicPlanKey` is the ordered tuple of every axis above. Empty/zero
inventories retain their axes. C6-C9 and D2 carry the complete key or
fingerprint; old handles never retarget.

## Exhaustive Variadic Form Matrix

`Plan` publishes requirements without mutation. `Absent` is a legal
non-applicable form and `Reject` aborts the complete plan. An absent/zero C2
class becomes an overflow path only when the exact variadic ABI rule permits
it; unknown facts never authorize a guessed promotion or traversal.

| Variadic definition/call/operation form | Exact facts consumed | Disposition | Exact requirement / stable failure | Later authority |
|---|---|---|---|---|
| empty or no-variadic module | exact empty definition/call/operation inventory | Absent | one valid empty `VariadicPlan` | C6 sees known-empty plan |
| nonvariadic function | exact signature and C3 nonvariadic boundary | Absent | explicit no-entry requirement | no variadic entry formation |
| nonvariadic call | exact C4 nonvariadic call row | Absent | explicit no-extra-argument requirement | no variadic transport |
| variadic declaration without body | stable declaration/signature/convention and fixed boundary | Plan | callable fixed/extra boundary with no entry-body requirement | calls may reference declaration |
| variadic definition with zero fixed parameters | exact signature and empty fixed order | Plan | zero named boundary plus complete entry requirements | D2 forms entry semantics later |
| variadic definition with fixed parameters | exact ordered fixed IDs/types plus C3 classifications | Plan | complete named/unnamed boundary and entry summary | D2 consumes without reclassification |
| variadic call with zero extras | exact C4 fixed boundary and empty extra suffix | Plan | known-empty extra table plus retained boundary | D2 transports fixed inputs only |
| variadic call with extras | exact C4 ordered extra IDs/types and C3 promotion/classification obligations | Plan | complete unnamed argument group | D2 transports exact group |
| already-promoted scalar/pointer extra | original/call types and registered identity promotion | Plan | `IdentityPromotion` requirement | no rewrite in C5 |
| narrow integer/bool/enum extra | original type/range and registered default integer-promotion rule | Plan | exact promoted integer type obligation matching Canonical/C4 | language/importer supplied value; C5 only proves |
| floating extra requiring default promotion | original float type and registered floating-promotion rule | Plan | exact promoted floating type obligation matching Canonical/C4 | no conversion emitted here |
| unpromoted or wrongly promoted extra | exact original/call type disagreement with required rule | Reject | `VariadicPlanPromotionMismatch` | no implicit conversion or downgrade |
| aggregate/vector/HFA/HVA extra admitted by ABI | exact semantic layout, C3 class and variadic ABI rule | Plan | complete typed group plus alignment/fallback requirement | D2 owns transport |
| aggregate/vector extra unsupported by ABI | exact type/layout/class and absent legal rule | Reject | `VariadicPlanTypeUnsupported` | no helper or memory guess |
| no register-save domain | exact C2 absent/zero eligible classes and legal overflow-only ABI rule | Absent | known-empty save-area requirements plus overflow domain | no inferred register bank |
| register-save semantic domain | exact C2 eligible class/group capacities/aliases and ABI entry rule | Plan | abstract region/class capacity, alignment, lifetime and access requirements | D2 forms saves; E4 places region |
| class capacity exhausted with legal overflow | exact ordered consumption/capacity and registered fallback | Plan | deterministic switch to abstract overflow domain | no concrete cursor/stack address |
| capacity exhausted without legal overflow | exact consumption/capacity and absent fallback | Reject | `VariadicPlanCapacityUnsupported` | no over-capacity slot or helper |
| valid `va_start` | operation/list/function IDs and exact named/unnamed boundary, including the last named parameter when present | Plan | initialized abstract list-state transition | D2 emits semantic operation |
| `va_start` in nonvariadic or wrong-boundary function | exact invalid function/boundary relation | Reject | `VariadicPlanStartInvalid` | no repaired boundary |
| valid `va_copy` | distinct destination, live initialized source and ABI copy rule | Plan | independent copied-state/lifetime transition | D2 emits semantic operation |
| `va_copy` from uninitialized/ended or illegal alias | exact invalid source/destination state relation | Reject | `VariadicPlanCopyInvalid` | no guessed state |
| scalar integer/pointer `va_arg` | live list state, requested type/layout, C3 class and region/fallback rule | Plan | ordered class/overflow test, alignment, progression and typed result requirement | D2 emits traversal operation |
| floating/vector `va_arg` | live state, requested format/shape, C3 class and variadic rule | Plan | exact class/fallback traversal requirement | no register selected here |
| aggregate/HFA/HVA `va_arg` | live state, semantic layout, complete field/lane paths and C3 group | Plan | all-or-nothing group/overflow traversal plus alignment requirement | no lane register or address chosen |
| over-aligned `va_arg` with legal ABI rule | live state, semantic alignment and registered rounding/fallback domain | Plan | abstract alignment/progression requirement | C6/D2 later materialize address/operation |
| `va_arg` before start, after end or with incompatible type | exact invalid state/type relation | Reject | `VariadicPlanArgInvalid` | no default value or state repair |
| valid `va_end` | live list state and stable operation/list IDs | Plan | terminal lifetime transition | D2 emits semantic operation |
| `va_end` before start, after end or repeated end | exact invalid lifetime state | Reject | `VariadicPlanEndInvalid` | no ignored operation |
| branch/merge traversal with one provable state | complete Canonical control/value relation and identical compatible incoming states | Plan | one deterministic merged abstract state | no hidden side table or mutation |
| ambiguous/unclosed list lifetime or unsupported ABI traversal | exact conflicting/unknown states or absent registered rule | Reject | `VariadicPlanTraversalUnproved` | no heuristic cursor, helper or leak acceptance |

Every variadic definition, call and semantic operation has exactly one matrix
disposition. A copied list has an independent state identity; traversal state
is semantic plan data, not mutable storage or a concrete cursor offset.

## Ordered Behavior

1. Validate and freeze the exact Canonical/C1/C2/C3/C4 capabilities, registry/
   schema/options identities and deterministic execution policy.
2. Inventory all variadic definitions/declarations, call boundaries and
   `va_start`/`va_copy`/`va_arg`/`va_end` operations in stable Canonical order.
3. Join every definition/call to exact C3/C4 rows; prove fixed/extra boundaries
   and every default-promotion obligation without creating conversions.
4. Derive abstract entry, save-domain, overflow-domain and list-state schemas
   from exact C2 capacities and registered ABI rules, selecting no location.
5. Traverse semantic list operations to stage initialization/copy/argument/end
   transitions, alignment, exhaustion/fallback and lifetime requirements.
6. Verify total coverage, stable identity uniqueness, predecessor agreement,
   promotion correctness, group closure, capacity/fallback legality, state-
   transition closure and complete key/fingerprint coverage.
7. Recheck every input key, then atomically publish one immutable
   `VariadicPlan`; otherwise discard all private rows and publish none.

## Invariants

- C5 is requirements-only. An abstract save region is not a frame object; a
  cursor/progression rule is not a byte offset or mutation.
- C3 classifications and C4 call boundaries are referenced exactly and never
  recreated, weakened or converted into concrete locations.
- Default promotions are verified typed semantic facts already present at the
  Canonical boundary; C5 inserts no cast and rewrites no call operand.
- Canonical, preparation binding, layout, ABI plan and call plan remain
  immutable and unchanged.
- Names, rendered types, pointers, offsets, dense indices and legacy
  publication/traversal routes never establish identity, freshness or support.

## Verification, Failure, and Invalidation

The private C5 verifier checks the candidate against the frozen exact key:
capability equality, total definition/call/operation coverage, stable-ID
uniqueness, exact C3/C4 references, fixed/extra and promotion agreement,
save/overflow capacity and alias consistency, aggregate/group closure,
`va_list` type/state-transition/lifetime correctness, exhaustion/fallback
legality, target/helper/location exclusion and deterministic ordering.

Any stale/missing/mixed key, malformed variadic signature/operation, incomplete
boundary or promotion, illegal class/capacity/alignment/fallback, unproved list
state, unsupported traversal, cancellation or resource failure discards the
whole candidate. Failure publishes no plan, entry, requirement, fingerprint,
cache record, C6 fact or D2 fact; every input remains unchanged.

Any Canonical stamp/function revision, target/profile schema, layout key,
`AbiPlan` or `CallPlan` key/fingerprint, variadic registry/schema/options or
observed definition/call/operation/list identity change invalidates the
complete plan and all C6-C9 successors. Old handles never retarget; checked
planning under the complete new key is required even when facts compare equal.

## Target, Transport, and Frame Rules

C5 may use C2 capacities and exact C3/C4 requirements to describe semantic
save/overflow/list traversal domains. It cannot choose an abstract slot,
physical register/unit, helper, target opcode, save sequence, stack address,
byte offset, home, spill, scratch, frame object/action or instruction. D2 owns
ABI transport, D4 target legalization, C6 address strategies and E4 final frame
placement/action materialization.

## Implementation State

Implementation is absent. This directory contains only this README and has no
build edge. No checked-in `VariadicPlan`, key/fingerprint/requirement IDs,
variadic registry, planner/verifier transaction, state/lifetime graph, cache,
C6 handoff, D2 consumer path or focused runtime proof exists.

Existing LIR import and legacy variadic entry/emission, aggregate/HFA `va_arg`,
call-plan, decoded-home and target-specific helpers are migration evidence
only. They do not consume the exact Canonical/C1/C2/C3/C4 tuple, publish this
immutable key or satisfy C5.

## Proof Requirements

- prove exact Canonical/C1/C2/C3/C4, schema/options and registry keys plus stale
  rejection;
- prove nonvariadic, fixed/extra, promotions, entry/save/overflow and every
  `va_start`/`va_copy`/`va_arg`/`va_end` traversal/lifetime form has one exact
  disposition or stable failure;
- prove requirements-only output, atomic failure, transitive invalidation and
  no input, call, list-object or BIR mutation;
- prove C6 accepts only the complete same-key product and D2/E4 retain sole
  transport/frame authority; and
- reconcile implementation claims with directory contents, build inclusion
  and callable reachability.

## Open Questions

None. New promotion, list-state, traversal, class-exhaustion or ABI families
require versioned registry/schema updates and coordinated C3-C6/D2 review.

## Review Checklist

- [x] Metadata and core-first ownership are explicit.
- [x] Exact input/output matrices and complete product key are closed.
- [x] Nonvariadic, fixed/extra, promotions and entry/save domains are explicit.
- [x] `va_start`/`va_copy`/`va_arg`/`va_end` state/lifetime forms are exhaustive.
- [x] No placement, transport, register/helper/opcode/home/frame choice appears.
- [x] Failure/invalidation are atomic and the C6/D2 boundaries are exact.
- [x] Absent implementation truth is explicit.
