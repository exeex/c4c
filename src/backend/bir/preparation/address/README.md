# Immutable Address Requirement Plan Contract

Contract-Status: under-review
Implementation-Status: absent
Kind: product
Phase-ID: C6
Upstream: exact verifier-bound Canonical/C1 input, matching C2 `VerifiedTargetLayout`, exact C3 `AbiPlan`, exact C4 `CallPlan`, exact C5 `VariadicPlan`, and exact-current `Provenance`
Downstream: one immutable exact-key `AddressPlan` consumed first by C7 inline-assembly target preparation and later by D1/D4/E4/F1
Owner-Path: `src/backend/bir/preparation/address/README.md`
Last-Reconciled-Commit: `5e7909f3e`

## Purpose

C6 is the sole preparation owner for target-aware semantic address-
materialization and relocation requirements. It binds every Canonical address,
object, symbol, path and relocation identity to the exact C1-C5 capabilities
and exact-current provenance, then atomically publishes one immutable
`AddressPlan`. The plan states typed semantic requirements and legal domains
only. It selects no address mode, target opcode or sequence, concrete offset,
frame placement, assignment, encoding or mutation.

## Owns

- stable address/materialization/relocation requirement IDs and one closed C6
  address-form schema;
- exhaustive global, local, TLS, string, function/label/block, relocation,
  GEP/path, dynamic-object and generic address-value dispositions;
- semantic base/index/path/scale/displacement-domain, relocation and bounded
  later-expansion requirements without selecting their realization;
- exact provenance consumption, Known/Absent/Unknown handling and stale
  rejection at the earliest target-aware address consumer;
- the complete `AddressPlanKey`/fingerprint, private verifier, structured
  failure, atomic publication and transitive invalidation; and
- the sole exact-key `AddressPlan` capability accepted first by C7 and later
  retained through D4/E4/F1.

## Does Not Own

- C1 target selection, verifier binding, C2 layout, C3 ABI classification, C4
  call planning, C5 variadic planning or computation/publication of
  `Provenance` and its dependencies;
- address/object/symbol/value/path identity, semantic type/layout, def-use,
  CFG, Canonical storage, linkage or relocation meaning declared upstream;
- a selected target address mode, opcode, instruction/expansion sequence,
  relocation encoding, concrete base/index register, scale encoding,
  displacement or object/frame offset;
- register assignment, home, scratch, spill, frame object/action, stack
  adjustment, helper selection/lowering, MIR record or emission; or
- inference from names, spelling, rendered types/layout, host pointers,
  numeric pointer values, source text, positions or legacy route tables.

## Inputs

C6 receives one frozen exact Canonical/target/layout/C3-C5 tuple and one
ordered exact-current `Provenance` handle for every defined function whose
address semantics it observes. The analysis remains `CanonicalSemantic` with
target key `None`, empty preparation and canonical empty options; C6 never
retags provenance under the target-aware address-plan key.

### Exact C6 input matrix

| Input / product | Exact required state | Optional / empty form | Failure / forbidden substitution |
|---|---|---|---|
| preparation input | verifier-issued borrowing `VerifiedPreparationInput` for this exact Canonical owner and target fingerprint | empty module remains valid | copied report, foreign/stale binding or reconstructed view is `AddressPlanBindingInvalid` |
| Canonical view/stamp | immutable B8 `CanonicalBir` with exact epoch/module/function digest, plan/options lineage, ordinal 7 and all P01-P07 properties | no address identities is valid | Raw/candidate/view substitute, mixed revision or stale stamp is `AddressPlanCanonicalInvalid` |
| validated target | exact immutable C1 `TargetProfile` and complete `TargetFingerprint` named by the binding | none | architecture/triple similarity, environment default or partial target is `AddressPlanTargetInvalid` |
| verified layout | exact C2 `VerifiedTargetLayout`, complete key/fingerprint and target address-width/eligibility domains under the same binding | optional capability/domain may be `Absent` | parsed `data_layout`, compatible table/count, stale key or reconstructed mapping is `AddressPlanLayoutInvalid` |
| ABI plan | exact verified C3 `AbiPlan`, complete key/fingerprint and address-carried byval/sret/aggregate requirements | no such boundaries known empty | reclassification, subset/equal-looking/stale/foreign plan is `AddressPlanAbiInvalid` |
| call plan | exact verified C4 `CallPlan`, complete key/fingerprint and abstract outgoing/hidden object requirements | no calls known empty | recreated object/call boundary, subset/equal-looking/stale/foreign plan is `AddressPlanCallInvalid` |
| variadic plan | exact verified C5 `VariadicPlan`, complete key/fingerprint and abstract save/overflow/list traversal requirements | no variadic objects known empty | recreated traversal/object domain, subset/equal-looking/stale/foreign plan is `AddressPlanVariadicInvalid` |
| provenance descriptor | `AnalysisId::Provenance`, schema 1, function scope, `CanonicalSemantic`, admitted at exact B8 | none | unknown/duplicate/schema/domain/stage mismatch is `AddressPlanAnalysisInvalid` |
| provenance handle bundle | exact-current ordered handle per observed defined `FunctionId`, keyed by epoch/module/function revision and exact CFG, dominance, publication/value-flow and memory-effects dependencies; empty options, target `None`, preparation empty | declaration has no body handle; empty bundle known empty | missing/extra/foreign/stale handle, dependency refresh or B4 handle substitution is `AddressPlanStaleAnalysis` |
| address/relocation registry | exact target-selected semantic address-domain, relocation-family and later-expansion requirement rule versions | no extension entries | backend callback/default, text matcher, duplicate/mixed rule is `AddressPlanRegistryInvalid` |
| address schema | exact `AddressPlanSchemaFingerprint` covering every requirement family and verifier rule | none | missing/newer/older/mixed schema is `AddressPlanSchemaInvalid` |
| address options | canonical normalized `AddressPlanOptionsFingerprint`; v1 has no free semantic option | canonical empty options only | route/worker/cache/address-mode preference is `AddressPlanOptionsInvalid` |
| execution control | deterministic cancellation/resource/fixed-point bound | observer absent is valid | unbounded or nondeterministic policy is `AddressPlanResourceInvalid` |

The shared [Provenance contract](../../analysis/provenance/README.md) admits a
fresh exact B8 read-only query while retaining B4 as P05's earliest mutation-
consumer checkpoint. C6 requires the B8 result and its same-revision dependency
closure; it cannot reuse the B4 handle after P05-P07 mutation even when facts
would compare equal.

## Outputs

Success publishes exactly one immutable module-wide `AddressPlan`. Every table
is an embedded read-only view, never an independent capability. Failure
publishes no plan, entry, requirement, fingerprint, cache record, C7 fact or
later-stage fact.

### Exact C6 output matrix

| Embedded output / result | Exact consumer | Required binding | Optional / error form |
|---|---|---|---|
| `AddressPlan` capability | C7, D1/D4, E4/F1 and declared later consumers | complete `AddressPlanKey`, verifier token and total relevant address/object/relocation coverage | valid empty plan with exact inputs; no partial/equal-looking plan |
| address identity summary | C7 lineage and D1 | stable instruction/value/object/symbol/address-space/path IDs, semantic type and ordered requirement IDs | no address identities known empty; foreign/malformed identity fails |
| semantic base requirements | D1/D4 | exact stable base value/object/symbol origin, address space and pointer-width domain | null/absolute base explicit; Unknown permitted only by registered generic-pointer rule |
| semantic index requirements | D1/D4 | exact stable index value/type/signedness-free width and source path role | no index explicit `Absent`; no register or extension opcode selected |
| semantic path requirements | D1/D4 and verifier | exact ordered field/index/source-byte-offset path components tied to provenance facts | no derived path `Absent`; unknown path never fabricated |
| scale-domain requirements | D1/D4 | finite legal semantic element/stride domain and source type relation | scale one/absent explicit; no scale encoding or chosen address mode |
| displacement-domain requirements | D1/D4/E4 | semantic constant/addend domain, width/alignment and in-domain versus later-expansion requirement | no addend known zero; no concrete target/frame displacement selected |
| symbol/relocation requirements | D1/D4/F1 lineage | stable `SymbolId`, symbol class/linkage/visibility, address space, addend and registered relocation-family alternatives | no relocation explicit `Absent`; unresolved external remains typed, never name-resolved |
| local/dynamic object requirements | D1 and E4 | stable `ObjectId`/lifetime/scope, semantic size/alignment/access and static/dynamic class | no local object known empty; no frame object/base/offset/adjustment selected |
| TLS requirements | D1/D4/E4 | stable TLS symbol/model semantic requirement, address space and registered relocation-domain alternatives | non-TLS `Absent`; no sequence, segment register or offset selected |
| string/constant-address requirements | D1/D4/F1 lineage | stable constant/object/symbol identity, semantic bytes/type/alignment and relocation-domain alternatives | no string/constant known empty; no pool placement/label spelling |
| function/label/block requirements | D1/D4 | stable function/symbol/label/block identity, address space, visibility and relocation-domain alternatives | no such address known empty; no emitted label/address chosen |
| materialization/expansion requirements | D1/D4 | typed closed semantic operation family, legal domain alternatives and whether a bounded later D4 expansion is required | directly realizable requirement explicit; no opcode, sequence, scratch or address mode selected |
| structured `AddressPlanFailure` | caller/diagnostics | stable value/object/symbol/path/relocation/rule/key/cause and deterministic order | report grants no plan, cache, repair or retry authority |

## Adjacent-Stage Contract

The exact C3 [ABI](../abi/README.md), C4 [call](../calls/README.md) and C5
[variadic](../variadic/README.md) products supply predecessor object/address
requirements. C6 references their complete same-key fingerprints and never
recreates classification, call-object or traversal facts.

[C7 inline-assembly target preparation](../inline_asm/README.md) accepts only
the complete `AddressPlan` fingerprint with unchanged C1-C5 capabilities. It
uses C6 only as predecessor lineage and cannot repair address coverage or
attach a selected realization to an inline-assembly operand.

[D4 target legalization](../../passes/target/README.md) alone performs any
required bounded one-to-many pseudo expansion before allocation. [E4 allocated
publication](../../allocated/README.md) alone fixes object bases, offsets,
displacements and frame actions after assignment. F1 is apply-only over the
exact `MirReadyBirView`, frame plan and registered mapping: it emits one record
per already realizable node and cannot select an address, expand a node, place
an object or repair C6.

## Stable Identity and Exact Product Key

| Key / identity axis | Exact content | Stale / forbidden shortcut |
|---|---|---|
| Canonical identity | complete B8 `PipelineStageStamp`, immutable owner, epoch/module revision and ordered function revisions | module revision, stable ID set or semantic hash alone |
| C1 target identity | complete `TargetFingerprint` and validated profile schema | architecture/triple/address width similarity |
| C2 layout identity | complete `TargetLayoutKey` and `TargetLayoutFingerprint` | compatible capacity/table/mapping version |
| C3 ABI identity | complete `AbiPlanKey` and verified `AbiPlanFingerprint` | copied boundary rows, subset hash or reconstruction |
| C4 call identity | complete `CallPlanKey` and verified `CallPlanFingerprint` | recreated outgoing/hidden object requirements |
| C5 variadic identity | complete `VariadicPlanKey` and verified `VariadicPlanFingerprint` | recreated save/overflow/list traversal requirements |
| analysis identity | ordered `(FunctionId, complete exact-B8 Provenance AnalysisKey/fingerprint)` for every observed definition | B4 handle, missing dependency, partial bundle or implicit new-key substitution |
| address-rule identity | target-selected semantic address/relocation/expansion registry versions | backend callback, environment, name or renderer rule |
| plan schema/options | `AddressPlanSchemaFingerprint` and normalized semantic options | route, worker, cache or selected-mode configuration |
| `AddressRequirementId` | stable function/instruction/value/object/symbol/path/role/ordinal plus requirement-kind ordinal | pointer, name, text, raw numeric address or dense position |
| `AddressPlanFingerprint` | deterministic digest of the full key and every verified output row in stable-ID order | subset/report/equal-looking plan fingerprint |

`AddressPlanKey` is the ordered tuple of every axis above. Empty/zero
inventories retain their axes. C7-C9 and later stages carry the complete key or
fingerprint; old handles never retarget.

## Exhaustive Address-Form Matrix

`Plan` publishes semantic requirements without mutation. `Absent` is a legal
non-applicable form, `Defer` names an exact later authority, and `Reject`
aborts the complete plan. `Unknown` provenance permits only a registered
generic pointer-base requirement; it never authorizes object, symbol, path or
relocation inference.

| Canonical address/materialization form | Exact facts consumed | Disposition | Exact requirement / stable failure | Later authority |
|---|---|---|---|---|
| empty or no-address module | exact empty address/object/relocation/provenance inventories | Absent | one valid empty exact-key plan | C7 sees known-empty plan |
| typed null address | exact null origin, pointer type and address space | Plan | typed null/base-zero semantic requirement | D1 forms generic address node |
| defined global object | stable `ObjectId`/`SymbolId`, definition/linkage/type/alignment and Known origin | Plan | global-object plus relocation-domain requirements | D4 legalizes; F1 applies mapping |
| external global declaration | stable symbol/declaration/linkage/visibility/type and exact unresolved-external class | Plan | external-symbol relocation requirement, never a guessed address | linker resolves after encoded mapping |
| function address | stable `FunctionId`/`SymbolId`, signature/linkage/visibility and Known symbol origin | Plan | function-symbol address/relocation domain | no helper or code address chosen |
| thread-local object | stable TLS symbol/object, semantic TLS model requirement and address space | Plan | typed TLS relocation/materialization alternatives | D4 expands; E4 places thread/frame interactions |
| string or constant object | stable constant/object/symbol ID, exact bytes/type/alignment and origin | Plan | constant-address and relocation-domain requirement | no pool offset or label spelling |
| semantic label address | stable label/symbol identity, scope/visibility and Known origin | Plan | label relocation-domain requirement | no emitted label/address selected |
| block address | stable function/block identity and exact declared block-address semantic form | Plan | block-address requirement with exact function ownership | D4 handles target form; no CFG inference |
| static local object | stable object/allocation/lifetime ID, semantic size/alignment/scope and Known origin | Plan | local-object address and access requirements | E4 alone places frame object |
| dynamic stack object | stable dynamic allocation/lifetime ID, typed size/alignment and Known origin | Plan | dynamic-object/base/adjustment semantic requirements | D1/D4 form nodes; E4 fixes actions/placement |
| C4 byval/sret/outgoing object | exact predecessor object requirement and matching Provenance origin/status | Plan | retained call-object address requirement | D2 forms transport; E4 places object |
| C5 variadic save/overflow/list object | exact predecessor region/list requirement and matching semantic origin/status | Plan | retained abstract variadic address/access requirement | D2 forms operations; E4 places regions |
| exact object-derived address | Known object origin, address space and unchanged semantic base | Plan | object-base requirement | no register/home chosen |
| exact symbol-derived address | Known symbol origin, symbol class and address space | Plan | symbol-base/relocation requirement | no name lookup |
| constant-field GEP | Known base, exact field path/type and semantic source offset carrier | Plan | ordered field-path plus constant semantic addend domain | no target displacement selected |
| constant-index GEP | Known base, typed element layout and exact constant index path | Plan | index/path/stride semantic requirement | no scale encoding selected |
| variable-index GEP | Known base, stable index value/type and element stride domain | Plan | base/index/stride eligibility requirements | D4 selects no hidden scratch in C6 |
| nested/mixed GEP | Known base and complete ordered field/index/byte path | Plan | closed all-components semantic path group | no flattened text/offset inference |
| semantic byte-offset carrier | registered typed carrier, Known base and exact source-semantic addend | Plan | base plus semantic addend domain | target legality checked without choosing mode |
| address-space cast admitted by target rule | exact source/destination address spaces, type and registered semantic conversion | Plan | conversion/materialization requirement | D4 owns any expansion |
| address-space cast unsupported | exact spaces/type and absent legal registry rule | Reject | `AddressPlanAddressSpaceUnsupported` | no truncation/helper guess |
| phi/join with identical Known origin | exact destination and every parallel `EdgeKey` Known-compatible origin/path | Plan | retained common semantic base plus join requirement | no predecessor-position inference |
| phi/join with ambiguous origin but legal generic pointer base | exact `Unknown(PhiJoinAmbiguous)` and registered generic-base rule | Plan | opaque typed pointer-base requirement only | no object/symbol/path fact invented |
| generic pointer with Unknown origin and legal base rule | exact stable Unknown reason, pointer type/address space and rule | Plan | generic pointer-base requirement | D1 retains ordinary value identity |
| Unknown origin where object/symbol/path proof is required | exact Unknown reason and requirement needing stronger fact | Reject | `AddressPlanProvenanceUnproved` | no name/text/pointer fallback |
| call/result origin Known by registered semantic rule | exact call/result IDs, Known origin and matching C4 facts | Plan | corresponding base/object/symbol requirement | no callee-name inference |
| opaque call/result origin with legal generic base rule | exact `Unknown(CallOriginUnproved)` and typed pointer result | Plan | generic pointer-base requirement only | no returned-object claim |
| in-domain semantic constant/addend | exact value/width/alignment and finite target legality domain | Plan | direct-domain semantic requirement | F1 still applies only after D4/E4 closure |
| out-of-domain addend with registered bounded expansion | exact semantic addend and expansion-family requirement rule | Defer | `RequiresD4Expansion` without opcode/sequence/scratch | D4 owns explicit expansion |
| out-of-domain addend without legal expansion | exact value/domain and absent rule | Reject | `AddressPlanDisplacementUnsupported` | no truncation or hidden sequence |
| absolute/section-relative relocation admitted | exact stable symbol/object/addend/visibility and registered family | Plan | relocation-family alternatives and semantic constraints | D4/F1 realize registered mapping |
| PC-relative/GOT/PLT relocation admitted | exact symbol class/linkage/visibility/addend and registered family | Plan | typed family-domain requirement | no selected opcode/sequence/encoding |
| TLS relocation admitted | exact TLS symbol/model/address space and registered family | Plan | typed TLS family-domain requirement | D4 expands; F1 applies exact mapping |
| unresolved relocation family or illegal addend | exact semantic relocation and absent/mismatched registry rule | Reject | `AddressPlanRelocationUnsupported` | no backend callback or spelling fallback |
| malformed type/path/object/symbol/relocation/provenance fact | exact stable entity and malformed core or dependency fact | Reject | `AddressPlanSemanticInputInvalid` | malformed never degrades to Unknown |
| name/text/host-pointer/numeric-address-only evidence | non-authoritative compatibility or renderer input | Reject | `AddressPlanForbiddenInference` | no semantic identity synthesized |

Every relevant Canonical address, object and relocation identity has exactly
one matrix disposition. Parallel phi edges remain distinct; stable semantic
path offsets are not target displacements or frame placements.

## Ordered Behavior

1. Validate and freeze the exact Canonical/C1/C2/C3/C4/C5 capabilities,
   registry/schema/options identities and deterministic execution policy.
2. Request `Provenance` after B8 for each observed defined function, requiring
   exact-current function and CFG/dominance/value-flow/effects dependency keys;
   reject every B4, mixed-revision or implicitly refreshed handle.
3. Inventory every address value, global/local/TLS/string/function/label/block
   object, GEP/path, dynamic object and relocation in stable Canonical order.
4. Join each identity to exact predecessor and provenance facts; select exactly
   one form disposition without names, text, host pointers or numeric guesses.
5. Stage semantic base/index/path/domain/relocation/object and bounded later-
   expansion requirements, selecting no realization.
6. Verify total coverage, stable identity uniqueness, predecessor agreement,
   Provenance Known/Absent/Unknown legality, type/address-space/path closure,
   relocation/domain rules and complete key/fingerprint coverage.
7. Recheck every input and analysis key, then atomically publish one immutable
   `AddressPlan`; otherwise discard all private rows and publish none.

## Invariants

- C6 is requirements-only. A legal scale/displacement domain is not an address
  mode; `RequiresD4Expansion` is not an opcode or instruction sequence.
- Source-semantic path/byte-offset facts are stable Canonical meaning, never a
  selected target displacement or frame offset.
- C3-C5 predecessor facts are referenced exactly and never recreated,
  weakened or translated into concrete locations.
- Unknown provenance remains explicitly unknown; it can authorize only a
  registered generic pointer-base form, never inferred object/symbol/path data.
- Canonical, preparation binding, layout, predecessor products, provenance and
  dependency handles remain immutable and unchanged.

## Verification, Failure, and Invalidation

The private C6 verifier checks the candidate against the frozen exact key:
capability equality, exact-B8 provenance/dependency freshness, total identity
coverage, stable-ID uniqueness, C3-C5 reference agreement, type/address-space/
path consistency, Known/Absent/Unknown legality, object/lifetime/alignment,
relocation/domain/expansion rules, target-mode/location exclusion and
deterministic ordering.

Any stale/missing/mixed key, stale dependency, malformed semantic fact,
unproved required provenance, unsupported address space/relocation/domain,
incomplete coverage, cancellation or resource failure also discards the whole
candidate. Failure publishes no plan, entry, requirement, fingerprint, cache,
C7 fact or later-stage fact; every input remains unchanged.

Any Canonical stamp/function revision, target/profile schema, layout key,
C3/C4/C5 key/fingerprint, Provenance result/dependency key, address registry/
schema/options or observed address/object/symbol/path/relocation identity change
invalidates the complete plan and all C7-C9 successors. Old handles never
retarget or implicitly recompute; a complete verified new-key plan is required
even when facts compare equal.

## Target, Legalization, Frame, and MIR Rules

C6 may use C2 target domains and exact semantic provenance to state legal
requirements and alternatives. It cannot select an address mode, target
opcode/sequence, concrete register, addend encoding, relocation encoding,
object/base/frame offset, home, scratch, spill, helper or MIR record. D1 forms
generic pseudos, D4 owns bounded target expansion, E4 owns placement and frame
actions, and F1 only applies one verified mapping per realizable node.

## Implementation State

Implementation is absent. This directory contains only this README and has no
build edge. No checked-in `AddressPlan`, key/fingerprint/requirement IDs,
address/relocation registry, exact-current provenance request route, planner/
verifier transaction, cache, C7 handoff or D4/E4/F1 consumer path exists.

Existing LIR import, addressing, GEP, local/global/static/dynamic object,
relocation, decoded-storage and target-specific helpers are migration evidence
only. They do not consume the exact Canonical/C1-C5/provenance tuple, publish
this immutable key or satisfy C6.

## Proof Requirements

- prove exact Canonical/C1-C5 and exact-current B8 Provenance/dependency keys,
  target/preparation-none domain and stale rejection;
- prove global/local/TLS/string/function/label/block/relocation/GEP/generic
  address and materialization forms plus Known/Absent/Unknown/malformed cases;
- prove requirements-only output, forbidden inference, atomic failure,
  transitive invalidation and no input/BIR mutation;
- prove C7 accepts only the complete same-key product and D4/E4/F1 retain sole
  expansion/placement/apply authority; and
- reconcile implementation claims with directory contents, build inclusion
  and callable reachability.

## Open Questions

None. New address, relocation or provenance-carrier families require versioned
registry/schema updates and coordinated Provenance/C6-C7/D4/E4/F1 review.

## Review Checklist

- [x] Metadata and core-first ownership are explicit.
- [x] Exact input/output matrices and complete product key are closed.
- [x] Address/object/relocation families and optional/error forms are explicit.
- [x] Name/text/pointer inference and concrete realization choices are forbidden.
- [x] Failure/invalidation and C7/D4/E4/F1 authority boundaries are explicit.
- [x] Absent implementation truth is explicit.
- [x] Shared Provenance contract admits the required exact-current B8 query.
