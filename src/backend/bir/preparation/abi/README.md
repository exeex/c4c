# Immutable ABI Requirement Plan Contract

Contract-Status: under-review
Implementation-Status: absent
Kind: product
Phase-ID: C3
Upstream: exact verifier-bound Canonical/C1 input plus matching C2 `VerifiedTargetLayout` and exact-current `PublicationValueFlow`
Downstream: one immutable exact-key `AbiPlan` consumed first by C4 call preparation
Owner-Path: `src/backend/bir/preparation/abi/README.md`
Last-Reconciled-Commit: `069bd3093`

## Purpose

C3 is the sole preparation owner for target-aware semantic ABI classification
requirements. It consumes the exact Canonical/target/layout capabilities and
exact-current publication/value-flow facts, classifies every function
parameter/result and call-visible typed boundary under one registered ABI, and
atomically publishes one immutable `AbiPlan`. The plan states requirements
only: it does not choose a concrete register or stack location, move a value,
lower a call, assign a home, or create frame state.

## Owns

- stable ABI boundary/requirement IDs and the closed C3 classification schema;
- exhaustive parameter, result, by-value, hidden-result, aggregate/HFA/HVA and
  variadic-boundary requirement classification;
- register-eligible versus stack-required, legal class/group alternatives,
  semantic width/alignment/splitting and abstract preservation requirements;
- the complete immutable `AbiPlanKey`/fingerprint, private plan verifier,
  structured failure, atomic publication and transitive invalidation; and
- the sole exact-key `AbiPlan` capability accepted by C4.

## Does Not Own

- C1 target selection/profile/fingerprint, verifier Canonical binding or C2
  target-layout tables/eligibility;
- core function/call/value identity, def-use or the computation/publication of
  `PublicationValueFlow`;
- a selected pseudo slot, concrete register name/unit, outgoing stack offset,
  frame object/offset, value home, move, spill or assignment;
- call transport, call preservation/clobber scheduling, tail-call decisions,
  variadic save-area/traversal planning, address/helper/constraint planning,
  pseudo lowering, MIR or emission; or
- mutation of Raw/Canonical BIR, analysis handles, layout or any predecessor
  product.

## Inputs

C3 receives one frozen exact Canonical/target/layout tuple and one ordered
exact-current `PublicationValueFlow` handle for every defined function. The
analysis remains `CanonicalSemantic` with target key `None`; C3 does not retag
or cache it under the target-aware plan key.

### Exact C3 input matrix

| Input / product | Exact required state | Optional / empty form | Failure / forbidden substitution |
|---|---|---|---|
| preparation input | verifier-issued borrowing `VerifiedPreparationInput` for this exact Canonical owner and target fingerprint | empty module remains valid | copied/report/foreign/stale binding is `AbiPlanBindingInvalid` |
| Canonical view/stamp | immutable B8 `CanonicalBir` with exact epoch/module/function digest, plan/options lineage, ordinal 7 and all P01-P07 properties | declaration-only/empty module valid | Raw/candidate/view substitute, mixed or stale stamp is `AbiPlanCanonicalInvalid` |
| validated target | exact immutable C1 `TargetProfile` and complete `TargetFingerprint` named by the binding | none | architecture/triple equality or partial/reconstructed profile is `AbiPlanTargetInvalid` |
| verified layout | exact C2 `VerifiedTargetLayout`, complete `TargetLayoutKey` and fingerprint under the same target and Canonical stamp | known-empty optional class/table is valid | compatible count/table, stale key or foreign mapping schema is `AbiPlanLayoutInvalid` |
| value-flow descriptor | `AnalysisId::PublicationValueFlow`, schema 1, function scope, `CanonicalSemantic` | none | unknown/duplicate/schema/domain mismatch is `AbiPlanAnalysisInvalid` |
| value-flow handle bundle | exact-current ordered handle per defined `FunctionId`, each keyed by epoch/module/function revision, exact CFG/dominance dependencies, canonical empty options, target `None`, preparation empty | declarations have no body handle; empty module bundle is known empty | missing/extra/foreign/stale handle or dependency is `AbiPlanStaleAnalysis` |
| ABI registry | exact backend ABI/calling-convention/classification-rule registry version named by target/layout schemas | no extension entries | unsupported/duplicate/mismatched rule is `AbiPlanRegistryInvalid` |
| ABI schema | exact `AbiPlanSchemaFingerprint` covering every requirement family and verifier rule | none | missing/newer/older/mixed schema is `AbiPlanSchemaInvalid` |
| ABI options | canonical normalized `AbiPlanOptionsFingerprint`; v1 has no free semantic option | canonical empty options only | route/worker/cache/environment choice is `AbiPlanOptionsInvalid` |
| execution control | deterministic cancellation/resource/fixed-point bound | observer absent is valid | unbounded/nondeterministic policy is `AbiPlanResourceInvalid` |

## Outputs

Success publishes exactly one immutable module-wide `AbiPlan`. Its tables are
embedded read-only views and never independent capabilities. Failure publishes
no plan, entry subset, classification, fingerprint, cache record or C4 fact.

### Exact C3 output matrix

| Embedded output / result | Exact consumer | Required binding | Optional / error form |
|---|---|---|---|
| `AbiPlan` capability | C4 call preparation and declared later consumers | complete `AbiPlanKey`, verifier token and full module coverage | no partial/equal-looking plan; failure returns none |
| function ABI summary | C4 call-signature matching | stable `FunctionId`, convention rule, fixed/variadic boundary and ordered requirement IDs | declaration summary valid; no functions known empty |
| parameter requirements | C4 ordered call-input planning | stable function/parameter/value IDs, semantic type, class/group alternatives, width/alignment and register-eligible/stack-required disposition | zero parameters known empty; unproved/malformed type fails |
| result requirements | C4 return-recovery planning | stable function/result identity, semantic type and exact return disposition | void is explicit `Absent`; unsupported form fails |
| by-value requirements | C4 outgoing object requirements | stable boundary/object/type IDs, semantic size/alignment, copy/lifetime and stack-required contract | no byval is known empty; no offset/object placement selected |
| hidden-result requirements | C4 hidden-carrier planning | exact sret boundary, result type/object identity and legal abstract carrier requirement | no sret is `Absent`; hidden concrete register forbidden |
| aggregate/HFA/HVA requirements | C4 call-visible decomposition | stable typed field/lane paths, ordered class/group alternatives, split count and all-or-stack rule | nonaggregate is `Absent`; unclassifiable layout fails |
| variadic boundary requirements | C4/C5 fixed-extra boundary handoff | exact fixed parameter count, ellipsis presence, convention and typed promotion obligations | nonvariadic is `Absent`; no save-area/offset/traversal plan |
| abstract preservation requirements | C4 preservation/clobber planning | convention-owned preserved/clobbered class sets only, never selected values/homes | no special requirement known empty |
| structured `AbiPlanFailure` | caller/diagnostics | stable rule/boundary/entity/key/cause and deterministic order | report grants no plan, cache or retry authority |

## Adjacent-Stage Contract

The [external C1 boundary](../../../../target_profile/README.md) and [C2 target
layout](../../target_layout/README.md) supply exact target/layout identity; the
[shared verifier](../../verify/README.md) supplies `VerifiedPreparationInput`.
At C3, the analysis manager requests [PublicationValueFlow](../../analysis/publication/README.md)
for the exact immutable Canonical revision after all B1-B8 mutations. Its
complete function/dependency keys must match that revision; an old B3/P04
handle is stale even when semantic facts would compare equal.

[C4 call preparation](../calls/README.md) accepts only the complete same-key
`AbiPlan` fingerprint together with the unchanged Canonical/target/layout
capabilities. C4 may derive per-call transport requirements but cannot repair,
weaken or reconstruct C3 classification. C3 produces no call move, selected
slot, stack offset or instruction.

## Stable Identity and Exact Product Key

| Key / identity axis | Exact content | Stale / forbidden shortcut |
|---|---|---|
| Canonical identity | complete B8 `PipelineStageStamp` and immutable owner | module revision alone, view or semantic hash |
| C1 target identity | complete `TargetFingerprint` and validated profile schema | architecture/triple/ABI-only equality |
| C2 layout identity | complete `TargetLayoutKey` and `TargetLayoutFingerprint` | table counts, compatible capacity or mapping version |
| analysis identity | ordered `(FunctionId, complete PublicationValueFlow AnalysisKey/fingerprint)` for every definition | missing declaration distinction, stale dependency or recomputed-new-key substitution |
| ABI rule identity | backend ABI, calling-convention registry and classification-rule versions | free-form convention name or backend-local default |
| plan schema/options | `AbiPlanSchemaFingerprint` and normalized semantic options | route, environment, worker or cache identity |
| `AbiRequirementId` | stable function/boundary/role/index identity plus requirement-kind ordinal | name, pointer, vector position without owning stable IDs |
| `AbiPlanFingerprint` | deterministic digest of the full key and every verified output row in stable-ID order | subset hash, diagnostic report or structurally equal plan |

`AbiPlanKey` is the ordered tuple of all axes above. Every C4-C9 reference
retains it or the complete fingerprint. No key axis may be omitted because a
current value is empty or zero.

## Exhaustive ABI Classification Matrix

`Classify` produces a requirement row without mutating its input. `Absent`
records a legal non-applicable form. `Reject` is whole-plan failure. C3 may
select a legal class/group alternative or `StackRequired`, but never a slot or
offset.

| Semantic boundary form | Exact facts consumed | Disposition | Exact requirement / stable failure | C4 visibility |
|---|---|---|---|---|
| empty module | exact empty Canonical/function/analysis inventories | Absent | one valid empty `AbiPlan` | C4 sees known-empty module plan |
| function declaration | stable `FunctionId`, signature, convention and target/layout key | Classify | complete signature requirements without body handle | every matching call may reference declaration summary |
| zero-parameter definition | exact empty parameter order and function key | Absent | known-empty parameter table | C4 sees no inputs |
| integer/pointer parameter | parameter/value ID, type/width and value-flow entry fact | Classify | legal `General` class/group alternatives or `StackRequired` | ordered input requirement |
| floating parameter | type/format and exact target/layout float ABI eligibility | Classify | legal `Float32/64/128` alternative or `StackRequired` | typed float input requirement |
| vector parameter | element/shape/width and exact enabled vector class/group | Classify | legal `Vector64/128/256` alternative or `StackRequired` | typed vector input requirement |
| direct aggregate parameter | semantic aggregate type/layout and ordered field paths | Classify | registered whole/group alternative with exact width/alignment | one aggregate requirement |
| split aggregate parameter | semantic layout, complete ordered part/lane paths and ABI rule | Classify | exact all-parts class/group sequence or `StackRequired` | complete split requirement, never partial |
| by-value parameter/object | stable type/object boundary, semantic size/alignment and copy/lifetime role | Classify | `ByValueStackRequired` plus abstract object requirements | C4 owns outgoing object planning, not placement |
| hidden sret parameter | exact result type/object identity and registered convention rule | Classify | one hidden-carrier requirement plus memory-result relation | C4 plans transport; no concrete hidden register |
| void result | exact void semantic type | Absent | explicit no-result requirement | C4 expects no recovery |
| scalar integer/pointer result | result identity/type and return value-flow fact | Classify | legal `General` alternative or `StackRequired/MemoryResult` | typed return requirement |
| floating/vector result | result type and exact class/group eligibility | Classify | legal float/vector alternative or `MemoryResult` | typed return requirement |
| direct aggregate result | semantic aggregate layout and registered return rule | Classify | whole/group result alternative with exact width/alignment | one complete return requirement |
| split aggregate result | ordered field/lane paths and complete return rule | Classify | exact all-parts sequence or `MemoryResult` | complete split return, never prefix |
| HFA/HVA parameter or result | homogeneous member type/count 1-4 and exact AAPCS64 layout legal bases | Classify | ordered homogeneous group requirement or all-stack/memory disposition | C4 sees one closed homogeneous requirement |
| fixed portion of variadic signature | exact fixed-count boundary and ordinary typed parameters | Classify | ordinary requirements tagged `FixedVariadic` | C4/C5 preserve fixed/extra boundary |
| variadic ellipsis / extra-argument domain | ellipsis identity, convention and registered default-promotion obligations | Classify | typed promotion/classification obligations only | C5 plans entry/save/traversal; no save area here |
| abstract convention preservation | convention rule and C2 eligible class sets | Classify | preserved/clobbered class requirements only | C4 decides call-specific preservation requirements |
| unsupported calling convention | stable convention ID absent from exact registry | Reject | `AbiPlanConventionUnsupported` | no plan |
| missing/malformed semantic type or value-flow fact | exact entity plus missing/`Unknown` required fact | Reject | `AbiPlanSemanticInputInvalid` or `AbiPlanAnalysisUnproved` | no plan; never guess or downgrade |
| class/group absent with legal memory fallback | exact C2 `Absent`/zero-capacity fact and ABI rule permitting memory | Classify | explicit `StackRequired`/`MemoryResult` | no inferred register class |
| class/group absent without legal fallback | exact type/rule and missing required C2 eligibility | Reject | `AbiPlanLayoutUnsupported` | no plan |

Every parameter/result/boundary has exactly one row. Unknown forms are not
treated as stack-required unless the registered ABI rule explicitly provides
that semantic fallback.

## Ordered Behavior

1. Validate the exact Canonical/C1/C2 capabilities, ABI registry/schema/options
   and deterministic execution policy.
2. Request `PublicationValueFlow` after B8 for each defined function; require
   exact-current complete keys/dependencies and freeze all input handles.
3. Inventory functions, signatures, parameters, results, call-visible
   aggregate paths and variadic boundaries in stable Canonical order.
4. Classify every boundary through exactly one registered rule and exhaustive
   matrix row; stage private requirement tables only.
5. Verify complete coverage, stable identities, type/layout consistency,
   allowed class/group membership, all-or-nothing splits, fallback legality and
   full key/fingerprint coverage.
6. Recheck every capability/analysis key, then atomically publish one immutable
   `AbiPlan`; otherwise discard all private rows and publish none.

## Invariants

- C3 is requirements-only. Register eligibility is not a selected register;
  stack requirement is not an offset, frame object or emitted store.
- Every semantic identity is a stable Canonical ID/path/role. Names, text,
  pointers, renderer coordinates and legacy lookup maps are forbidden.
- Split aggregates and HFA/HVA forms publish completely or not at all.
- Analysis facts remain target-independent disposable views and never become
  target/layout or plan publication authority.
- Canonical, `VerifiedPreparationInput`, layout and analysis handles remain
  immutable and unchanged.

## Verification, Failure, and Invalidation

The private C3 verifier checks the complete candidate against the frozen exact
key: input capability equality, analysis freshness/dependencies, total boundary
coverage, stable-ID uniqueness, semantic type/layout facts, registered
convention, class/group eligibility, split/homogeneous closure, fallback
legality and deterministic ordering.

Any stale/missing/mixed key, stale analysis, unknown required fact, malformed
signature/type, unsupported convention, illegal class/group/split, incomplete
coverage, cancellation or resource failure discards the whole candidate.
Failure publishes no plan, entry subset, requirement, fingerprint, cache entry
or C4 fact; every input remains unchanged.

Any Canonical stamp, target fingerprint/profile schema, layout key/fingerprint,
PublicationValueFlow result/dependency key, ABI registry/schema/options or
observed semantic identity change invalidates the complete plan and every
C4-C9 successor. Old handles never retarget; checked reclassification creates
a complete new-key plan.

## Target and ABI Rules

C3 may use C2 eligibility and target ABI rules to classify typed requirements.
It cannot use renderer names, concrete target units, backend-local defaults,
Raw/Canonical `data_layout` text, selected homes or allocator/frame state.
Concrete ABI locations and call transport remain later authorities.

## Implementation State

Implementation is absent. This directory contains only this README and has no
build edge. No checked-in `AbiPlan`, key/fingerprint/requirement IDs,
classification registry, exact-current analysis request route, verifier,
transaction, cache, C4 handoff or focused runtime proof exists.

Legacy call-return ABI classification, target register profiles, formal
publication/value-location records, call plans and special-carrier helpers are
migration evidence only. They do not consume the exact C1/C2/Canonical/
analysis tuple, publish this immutable key or satisfy C3.

## Proof Requirements

- prove exact Canonical/C1/C2 and exact-current PublicationValueFlow keys,
  timing, target-none analysis domain and stale rejection;
- prove every parameter/result/byval/sret/aggregate/HFA/HVA/variadic form has
  one explicit classification, absence, fallback or stable failure;
- prove requirements-only output, atomic failure, transitive invalidation and
  no input/BIR mutation;
- prove C4 accepts only the complete same-key `AbiPlan` without reconstructing
  classification or selecting concrete locations; and
- reconcile implementation claims with directory contents, build inclusion and
  callable reachability.

## Open Questions

None. New conventions, classification families or fallbacks require a
versioned ABI registry/schema update and coordinated C2/C3/C4 review.

## Review Checklist

- [x] Metadata and core-first ownership are explicit.
- [x] Exact input/output matrices and product key are complete.
- [x] PublicationValueFlow is requested exact-current at C3 and stays target-less.
- [x] Classification families and optional/error forms are exhaustive.
- [x] No concrete location, call transport, lowering, home or frame state appears.
- [x] Failure/invalidation are atomic and C4 handoff is exact.
- [x] Absent implementation truth is explicit.
