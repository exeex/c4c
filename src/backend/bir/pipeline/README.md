# Ordered Canonical BIR Pipeline Contract

Contract-Status: under-review
Implementation-Status: partial-foundation
Kind: pipeline
Phase-ID: B1 through B8
Upstream: one move-only verified target-independent unallocated `RawBir`
Downstream: one verifier-gated immutable target-independent unallocated `CanonicalBir`
Owner-Path: `src/backend/bir/pipeline/README.md`
Last-Reconciled-Commit: `938c7b43e`

## Purpose

This pipeline is the sole configured order/capability owner for canonical BIR.
It consumes one verified `RawBir`, executes exactly P01-P07 with framework
support, preserves deterministic checkpoint lineage/re-entry, then submits the
same frozen B7 candidate to B8. Only a completely green B8 verifier token lets
the pipeline atomically mint one `CanonicalBir`.

## Owns

- the immutable built-in P01-P07 occurrence order and unconditional B8 gate;
- canonical-v1 plan/options fingerprints, occurrence ordinals, cumulative
  property/stage stamps and exact module/function revision digest lineage;
- move-only `RawBir` consumption, last-good internal checkpoints, private
  occurrence forks and atomic checkpoint replacement;
- deterministic function-wave/module barriers, skip/repeat validation and
  failure/rollback orchestration;
- checkpoint serialization/resumption strictly after a completed occurrence;
- the B7-candidate-to-B8 capability boundary and atomic construction of
  `CanonicalBir` only from the verifier-private green token.

## Does Not Own

- pass descriptors, invocation transactions, analysis invalidation or
  execution-control mechanics; the [pass framework](../passes/README.md) owns
  those support functions;
- local P01-P07 semantic rewrites; each pass document owns them;
- analysis fact algorithms or verifier rule semantics;
- core IDs/revisions/storage/editor behavior;
- target selection/layout, ABI/helper preparation, pseudo lowering,
  constraints, allocation, MIR, rendering or emission;
- any alternate caller-supplied pass order or a way to relabel a checkpoint as
  `RawBir`/`CanonicalBir`.

## Inputs

The public runner consumes one move-only exact `RawBir`, one closed
canonical-v1 options product and deterministic execution control. The Raw
capability already proves full A2 verification and contains no unresolved
draft/import state, target context, preparation, allocation or MIR facts.

### Exact pipeline input matrix

| Input/product | Exact required state | Optional/empty form | Failure / forbidden substitution |
|---|---|---|---|
| owning input | one move-only verified `RawBir` with exact epoch/module/function revisions and Raw stamp | empty module valid | draft/view/copy/stale/foreign input is `PipelineWrongInput` |
| canonical plan | closed build-versioned canonical-v1 P01-P07+B8 plan fingerprint | no caller extension points | reordered/missing/extra occurrence is `PipelinePlanInvalid` |
| semantic options | normalized options fingerprint covering every configured pass/analysis choice | canonical defaults valid | environment/cache/worker timing cannot affect it |
| execution control | deterministic cancellation/resource/iteration policy | optional observer sinks empty | invalid/unbounded policy is `PipelineOptionsInvalid` |
| framework registry | exact validated P01-P07 descriptors/kinds/properties/analysis declarations | none | registry/plan disagreement fails before Raw consumption |
| verifier registry | exact Raw plus cumulative Canonical profile/version | none | missing/mismatched registry is `PipelineVerifierInvalid` |
| re-entry input | internal move-only checkpoint carrying exact lineage/stamp/fingerprints when resuming | absent for fresh run | copied/mixed/stale checkpoint is `PipelineCheckpointInvalid` |
| target exclusion | no target/profile/layout/ABI/helper/preparation/constraint/allocation/MIR input | none | later-domain input is `PipelineForbiddenInput` |

## Outputs

Success returns exactly one move-only `CanonicalBir`; failure returns one
structured failure plus only the explicitly permitted internal last-good
checkpoint. No stage view, report, stamp or semantic hash is a public capability.

### Exact pipeline output matrix

| Output/product | Exact consumer | Required binding | Failure / forbidden escape |
|---|---|---|---|
| immutable `CanonicalBir` | [C1 external target-profile boundary](../../../target_profile/README.md) and read-only observers | same B7 owning revision, full canonical stamp/digest/lineage and B8 verifier token | only successful B8 may create it |
| final stage stamp | `CanonicalBir` owner and preparation input gate | exact epoch/module revision/function digest, plan/options fingerprints, ordinal 7 and all properties | copied/equal/reconstructed stamp is not capability |
| structured pipeline failure | caller/diagnostics | stable phase/occurrence/rule/entity/key and deterministic order | no report changes success or grants resume |
| internal last-good checkpoint | `resume_bir_pipeline` only | complete prior occurrence stamp and owning storage | cannot convert directly to `CanonicalBir` or skip next ordinal |
| discard outcome | pipeline cleanup | failed private fork/cache candidates destroyed, predecessor retained | no partial function/module/revision/property escapes |
| observer events/statistics | read-only diagnostics | stable keys/ordinals and deterministic sequence | no scheduling, semantic or publication authority |

## Adjacent-Stage Contract

[A2 Raw publication](../verify/README.md) alone supplies `RawBir`. The
[framework](../passes/README.md) executes each configured occurrence but cannot
change order. Each pass supplies its semantic postcondition. The read-only
[Canonical verifier](../verify/README.md) consumes the exact frozen B7 candidate
and returns a private token only after the complete same-revision profile is
green. The pipeline consumes that token and candidate atomically to create
`CanonicalBir`. The [C1 external target-profile boundary](../../../target_profile/README.md)
receives only the published immutable result.

Authority is intentionally separated:

| Authority | Sole owner | Cannot do |
|---|---|---|
| invocation transaction, occurrence support, analysis invalidation | pass framework | choose P01-P07 order, define Canonical rules, mint `CanonicalBir` |
| exact order, stamps, checkpoints, re-entry and capability transitions | pipeline | define pass semantics or declare verifier success |
| Raw/Canonical cumulative rules and private publication token | verifier | schedule passes, mutate candidates or create pipeline checkpoints |
| final `CanonicalBir` construction from green token + same candidate | pipeline B8 boundary | accept an earlier report, stale candidate or partial wave |

## Exact Built-In Occurrence Sequence

| Ordinal | Occurrence / gate | Exact accepted input property | Exact established property / output | Mandatory analysis timing |
|---|---|---|---|---|
| B1 / P01 | `Legalize` | `RawVerified` | `TypesLegal` | none |
| B2 / P02 | `ScalarCanonicalize` | `TypesLegal` | `ScalarsCanonical` | `ComparisonSelect` immediately before P02 |
| B3 / P03 | `CfgCanonicalize` | `ScalarsCanonical` | `CfgCanonical` | `Cfg` before planning and recomputed from resulting terminators |
| B4 / P04 | `SsaCanonicalize` | `CfgCanonical` | `SsaCanonical` | exact-current `Cfg`, `Dominance`, `PublicationValueFlow` |
| B5 / P05 | `MemoryCanonicalize` | `SsaCanonical` | `MemoryCanonical` | exact-current `MemoryEffects`, `Provenance` closure |
| B6 / P06 | `AggregateCanonicalize` | `MemoryCanonical` | `AggregatesCanonical` | recompute/invalidate observed facts by mutation summary |
| B7 / P07 | `IntrinsicCanonicalize` | `AggregatesCanonical` | `IntrinsicsCanonical`; frozen B7 candidate | exact-current `CallGraph`; exact-B6 `MemoryEffects` rebind/recompute |
| B8 | unconditional Canonical verification/publication gate, not a pass | frozen ordinal-7 candidate with every P01-P07 property | one `CanonicalBir` only after full verifier token | analyses are not publication capability |

No occurrence may be omitted, reordered, recursively invoke an earlier pass or
hide an extra cleanup/fixed point. B8 is unconditional and cannot be disabled.

## Stage Stamp and Fingerprint Matrix

| Axis | Exact content | Update/validation rule | Forbidden shortcut |
|---|---|---|---|
| storage identity | owning private checkpoint/candidate capability | move-only consumption at each boundary | borrowed view, pointer or semantic hash |
| revision key | `ModuleEpoch`, `ModuleRevision`, ordered `(FunctionId, FunctionRevision)` digest | core-derived after committed effects; frozen for verification | module revision alone or mixed digest |
| plan identity | build-versioned canonical-v1 sequence fingerprint | fixed before Raw consumption and identical through B8 | caller-supplied order or free-form names |
| options identity | normalized pass/analysis semantic options fingerprint | fixed for lineage; resume must match exactly | environment/cache/worker count |
| occurrence lineage | completed ordinal/`PassId`, parent stamp and occurrence fingerprint | append exactly once after complete barrier | copied stamp, skipped ordinal or similar reconstruction |
| cumulative properties | `RawVerified` then exact P01-P07 properties | established only after corresponding postcondition/barrier | report or descriptor self-assertion |
| verifier identity | Canonical registry/profile version and private green token | generated only for the same frozen B7 key | earlier/cached diagnostic-only report |

## Ordered Behavior

1. Validate Raw capability, closed plan/options, framework/verifier registries
   and target exclusion before consuming input.
2. Create the initial internal last-good Raw checkpoint with exact stamp and
   canonical-v1 lineage.
3. For each ordinal P01-P07, validate required cumulative property/analysis
   timing, fork one private occurrence, invoke framework support and retain the
   prior checkpoint until the whole occurrence barrier succeeds.
4. On success, atomically replace last-good with the exact verified fork,
   append the ordinal/property/fingerprint and invalidate/install analyses by
   framework result. On failure, discard the fork and retain last-good.
5. After P07, freeze the exact B7 owner/stamp and prohibit edits/analysis
   publication races.
6. Invoke B8 on that same candidate. Require all framework postconditions and
   the complete Canonical verifier profile under the unchanged key.
7. On a private green token, atomically consume candidate+token and return one
   `CanonicalBir`; otherwise discard the B7 candidate and publish none.

## Invariants

- Exactly P01-P07 then B8 executes. Descriptors/properties do not create order.
- Each occurrence is atomic at module-wave scope even when its function
  proposals run concurrently.
- Last-good is an internal recovery checkpoint, never success or Canonical.
- `RawBir`, checkpoints, candidates, verifier tokens and `CanonicalBir` are
  move-only capability states; views/stamps/reports cannot forge transitions.
- Every checkpoint/candidate key and analysis handle is exact-current. Old
  handles never retarget across revision increments.
- Canonical BIR is target-independent and unallocated. No target/profile/
  layout/ABI/helper/preparation/constraint/home/spill/frame/MIR fact appears.

## Failure, Rollback, and Re-entry

Failure before an occurrence barrier publishes no edit/function prefix/
revision/property/cache entry. The private fork is discarded and last-good
remains unchanged. Failure at B8 publishes no `CanonicalBir`, property, cache
entry or reusable verifier token; an earlier checkpoint cannot be relabeled.

An internal checkpoint records owning storage, exact completed ordinal,
revision digest, plan/options fingerprints, occurrence lineage and cumulative
properties. Serialization/reload validates all axes. Resume starts strictly
after the completed ordinal, repeats no successful occurrence, skips none, and
always runs B8. A checkpoint is consumed once; inspection uses a borrow only.

## Verification and Canonical Publication

B8 accepts only one private frozen B7 candidate whose stamp proves exact
canonical-v1 lineage, plan/options fingerprints, ordinal 7,
`IntrinsicsCanonical`, epoch/module revision and ordered function-revision
digest. The candidate passed to the verifier is the same owner frozen by P07.

The verifier runs Raw plus every P01-P07 obligation and rejects target/
preparation/allocation/MIR facts. Any diagnostic, cancellation, resource
failure or key/stamp change discards the candidate. Only the completely green
private token permits the pipeline to mint exactly one immutable
target-independent unallocated `CanonicalBir` carrying the verified stamp.

## Target and ABI Rules

Canonical options cannot include a target triple/profile/layout, calling
placement, helper route, constraints, homes, spills, frame or MIR state. Source-
semantic typed sizes, alignments and address spaces remain semantic BIR facts;
[C1](../../../target_profile/README.md) selects and validates target context
independently after publication.

## Implementation State

Implementation is partial foundation only:

- checked in and build-included: revision/digest/stamp identity, internal
  move-only `PipelineCheckpoint`, consume-verified-Raw, private fork/view,
  distinct-storage check and discard; deterministic cancellation/work budget is
  provided by framework storage;
- absent: public `run_bir_pipeline`/resume APIs, canonical plan/options
  fingerprint implementation, complete P01-P07 runner/barriers, property and
  analysis orchestration, checkpoint serialization, B7 freeze capability, B8
  verifier-token integration and `CanonicalBir` minting.

No P01-P07 semantic pass implementation exists. Existing foundations cannot
produce a Canonical pipeline success.

## Proof Requirements

- prove the immutable exact P01-P07+B8 order and descriptor agreement;
- test stamp/digest/plan/options/lineage mismatch, move-only capability use,
  atomic occurrence rollback and last-good non-success semantics;
- test checkpoint serialization/re-entry strictly after completed ordinal and
  unconditional B8;
- prove framework/pipeline/verifier authority separation and same-owner B7/B8
  verification;
- prove only one full green gate mints target-independent unallocated
  `CanonicalBir`;
- reconcile implementation claims with storage, build inclusion and callable
  reachability.

## Open Questions

Concrete APIs for the absent runner/resume/serialization/B8 integration remain
implementation work; no API may weaken the capability or authority split.

## Review Checklist

- [x] Metadata, core-first ownership and substantive matrices are present.
- [x] P01-P07 order, analyses, properties and unconditional B8 are exact.
- [x] Stamps/fingerprints/capability transitions and re-entry are complete.
- [x] Failure retains only internal last-good and publishes no mixed state.
- [x] Framework, pipeline and verifier authorities are distinct.
- [x] Only same-revision full B8 can mint one target-independent `CanonicalBir`.
- [x] Partial-foundation implementation truth is explicit.
