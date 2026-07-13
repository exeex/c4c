# Canonical BIR Pass Framework Contract

Contract-Status: under-review
Implementation-Status: partial-foundation
Kind: framework
Applies-To: B1 / P01 through B8 orchestration support
Upstream: immutable pipeline checkpoints and configured pass occurrences
Downstream: verified occurrence results returned to the ordered pipeline
Owner-Path: `src/backend/bir/passes/README.md`
Last-Reconciled-Commit: `938c7b43e`

## Purpose

This framework defines closed pass identity, capability-scoped invocation,
private transactions, occurrence barriers, analysis access/invalidation,
verification hooks, deterministic execution control and structured failure for
canonical P01-P07. It provides orchestration support but never chooses the
ordered pipeline, mints `CanonicalBir`, or owns verifier semantics.

## Owns

- closed `PassId`, `PassKind`, `PassProperty`, repeat and mutation-effect
  registries plus static descriptor validation;
- type-erased function/module invocation sessions and exact read/edit authority;
- one private transaction per invocation, commit/rollback and authoritative
  core-derived `MutationSummary`;
- deterministic per-function proposal waves and atomic module occurrence
  barriers;
- analysis-manager access, exact-key checks, preservation validation,
  invalidation and transitive eviction;
- pre/postcondition invocation, verifier-on-commit support, diagnostics,
  cancellation, budgets, instrumentation and statistics;
- production of an unforgeable successful occurrence result for pipeline use.

## Does Not Own

- P01-P07 order, repetition schedule, stage lineage, plan/options fingerprint,
  checkpoint re-entry or the B7-to-B8 capability boundary; the
  [pipeline](../pipeline/README.md) owns those;
- semantic rewrites of legalize, scalar, CFG, SSA, memory, aggregate or
  intrinsics; their individual contracts own them;
- BIR storage, stable IDs, revisions, editors or def-use; [core](../core/README.md)
  owns those;
- analysis algorithms/results; the [analysis framework](../analysis/README.md)
  owns them;
- verifier rules or the Canonical acceptance token; the
  [verifier](../verify/README.md) owns them;
- Raw/Canonical capability minting, target/profile/layout, ABI/helper
  selection, preparation, constraints, allocation, MIR, rendering or emission.

## Inputs

The framework receives one immutable pipeline-owned checkpoint, one statically
validated descriptor/configuration, exact invocation scope and capabilities,
analysis/verifier services, deterministic execution control and no target-
domain facts. It cannot repair an invalid predecessor or infer authority from a
pass name, requested property, diagnostic report or cached analysis.

### Exact framework input matrix

| Input/product | Exact required state | Optional/empty form | Failure / forbidden substitution |
|---|---|---|---|
| pass descriptor | closed stable `PassId`, `PassKind`, requires/establishes/preserves, analyses, repeat and permitted effects | optional analyses may be empty | duplicate/unknown/inconsistent descriptor is `PassRegistryInvalid` |
| immutable checkpoint | exact owning epoch/module/function revisions, digest, stage stamp and cumulative properties | declaration/empty module remains valid if descriptor admits it | stale/foreign/mixed checkpoint is `PassWrongInput` |
| occurrence identity | pipeline-supplied stable occurrence ordinal and exact plan/options fingerprint | none | pass cannot choose/increment/reorder it; mismatch is `PassOccurrenceInvalid` |
| invocation capability | read view plus one kind-correct private editor/session and diagnostic sink | read-only no-op proposal valid | raw storage, foreign editor or wrong kind is `PassCapabilityInvalid` |
| analysis access | manager session bound to the exact checkpoint and descriptor-declared IDs/domains | no required analysis yields empty set | stale/undeclared/target-domain request is `PassAnalysisInvalid` |
| verification access | configured input gate, local/cumulative postconditions and verifier-on-commit callback | a no-op still runs required checks | green report from another revision cannot substitute |
| execution control | deterministic `CancellationToken`, bounded work/memory/iteration policy | zero optional statistics sinks valid | cancellation/resource exhaustion is structured failure |
| target exclusion | no profile/layout/ABI/helper/preparation/constraint/allocation/MIR/renderer input | none | any later-domain authority is `PassForbiddenInput` |

## Outputs

An invocation returns one complete proposal result or one structured failure.
Only the occurrence barrier may atomically replace the pipeline checkpoint with
the complete verified fork. The framework never returns `CanonicalBir`.

### Exact framework output matrix

| Output/product | Exact owner/consumer | Required binding | Failure / forbidden escape |
|---|---|---|---|
| invocation result | occurrence barrier | descriptor/occurrence/input key, Changed/Unchanged and complete proposed effects | no partial entity/function result escapes |
| private committed revision | occurrence fork only | exact parent key, new core revision/stamp and verified candidate | not a public checkpoint/capability |
| authoritative `MutationSummary` | analysis manager and pipeline occurrence | derived from actual core edits, including tombstones and module/function effects | pass declaration cannot override observed effects |
| preservation/invalidation decision | analysis manager | complete old/new keys, registered traits/dependencies and validators | no retargeted old handle or declared-only preservation |
| structured failure | pipeline observation | stable category/rule/entity/key and deterministic ordering | text/counter/report cannot change success |
| complete occurrence fork | ordered pipeline | every invocation succeeded, module barrier passed, exact stage key retained | no function prefix, mixed fork or last-good relabeling |
| occurrence-success capability | ordered pipeline only | move-only result bound to exact occurrence/stage/property request | cannot mint pipeline order or `CanonicalBir` |
| instrumentation/statistics | diagnostics/observers | read-only stable IDs/keys and deterministic counters | no mutation, identity, verifier or scheduling authority |

## Adjacent-Stage Contract

The [pipeline](../pipeline/README.md) supplies the exact P01-P07 occurrence
sequence and consumes only complete framework results. Individual pass
contracts supply semantic pre/postconditions. The analysis manager supplies
immutable exact-key facts. The [Canonical verifier](../verify/README.md) consumes
the final frozen B7 candidate at B8 and alone returns the private publication
token used by the pipeline to mint `CanonicalBir`.

Framework commit, occurrence publication and B8 publication are distinct:

```text
invocation private commit
  -> complete occurrence barrier/checkpoint
  -> pipeline advances exact ordinal/stamp
  -> B8 verifier-private Canonical token
  -> pipeline atomically mints CanonicalBir
```

No earlier arrow can be skipped, collapsed or treated as equivalent.

## Closed Canonical Descriptor Matrix

| Ordinal / ID | Kind | Requires | Establishes | Required analysis contract | Semantic owner |
|---|---|---|---|---|---|
| B1 / P01 `Legalize` | Module | `RawVerified` | `TypesLegal` | none | [legalize](legalize/README.md) |
| B2 / P02 `ScalarCanonicalize` | Function | `TypesLegal` | `ScalarsCanonical` | exact-current `ComparisonSelect` | [scalar](scalar/README.md) |
| B3 / P03 `CfgCanonicalize` | Function | `ScalarsCanonical` | `CfgCanonical` | exact-current pre-planning `Cfg`; recompute afterward | [CFG](cfg/README.md) |
| B4 / P04 `SsaCanonicalize` | Function | `CfgCanonical` | `SsaCanonical` | exact-current `Cfg`, `Dominance`, `PublicationValueFlow` | [SSA](ssa/README.md) |
| B5 / P05 `MemoryCanonicalize` | Module | `SsaCanonical` | `MemoryCanonical` | exact-current `MemoryEffects`, `Provenance` closure | [memory](memory/README.md) |
| B6 / P06 `AggregateCanonicalize` | Module | `MemoryCanonical` | `AggregatesCanonical` | none required | [aggregate](aggregate/README.md) |
| B7 / P07 `IntrinsicCanonicalize` | Module | `AggregatesCanonical` | `IntrinsicsCanonical` | exact-current `CallGraph`, `MemoryEffects` | [intrinsics](intrinsics/README.md) |

These identities, kinds and property names are immutable. Descriptors express
requirements; they do not schedule themselves. B8 is not a `PassId`.

## Ordered Behavior

1. Validate the closed registry and pipeline-supplied descriptor/occurrence/
   checkpoint/property/plan keys before exposing a view.
2. Resolve declared analyses through the exact-key manager; freeze immutable
   input views and reject stale/target-domain facts.
3. Fork one private occurrence candidate. Create exactly one kind-correct
   transaction per invocation and inventory work in canonical stable-ID order.
4. Run proposals with deterministic cancellation/resource checkpoints.
   Function waves may execute concurrently but publish results in canonical
   function order.
5. Validate `PassResult`, actual mutation effects, local postconditions and
   verifier-on-commit for every private invocation.
6. On any failure, discard every candidate-local edit/fact and retain the
   pipeline's prior checkpoint/cache unchanged.
7. When all invocations succeed, derive the complete occurrence summary,
   validate preservation/invalidation and transitive eviction, then atomically
   return one complete occurrence fork to the pipeline.

## Invariants

- One invocation observes one exact immutable input and owns at most one
  private transaction. A pass object retains no view/editor/handle across runs.
- Core derives revisions and mutation facts; a pass cannot self-assert Changed,
  Unchanged, a property or preservation.
- Every revision increment stales old handles. Checked preservation installs a
  new immutable result under the new key and never retargets the old handle.
- Function concurrency is deterministic and isolated. Module passes and
  occurrence barriers see the entire exact wave.
- Diagnostics, statistics, names, rendered text, pointer values and worker
  timing are observations only.
- The framework cannot reorder P01-P07, skip B8, grant target authority or
  manufacture a stage capability outside the pipeline/verifier boundary.

## Transaction, Failure, and Rollback

Every invocation follows:

```text
validate -> freeze -> fork -> plan -> edit -> derive summary
  -> postconditions -> verifier-on-commit -> commit private revision
```

Failure at any point rolls back the complete invocation; occurrence failure
discards the complete wave. No edit, function prefix, revision, property,
analysis/cache entry, diagnostic report, stage token or reusable candidate
escapes. A true no-op retains its revision but still proves descriptor
postconditions and repeat contract.

Stable framework failures cover registry/configuration, wrong/stale input,
capability/analysis mismatch, mutation-contract violation, nonconvergence,
verification, cancellation/resource exhaustion, commit conflict and occurrence
publication failure.

## Analysis Access and Invalidation

The framework exposes only descriptor-declared analyses through a session bound
to the exact input checkpoint. Candidate-local scratch facts die on rollback.
After successful edits it submits the core-derived `MutationSummary` to the
analysis manager, intersects declared preservation with descriptor permission,
checks dependencies/traits/validators, evicts rejected results and transitive
dependents, and installs only results stamped with the exact new key.

`PreservedAnalyses::all()` is valid only for an empty summary or independent
proof for every live result. A verifier report, equal count or unchanged pointer
is not preservation evidence.

## Verification and Publication Support

The framework invokes configured input/local/cumulative/verifier-on-commit
checks but does not define their rules. An invocation may request its property
only after its complete postcondition succeeds; the framework records the
property on the exact occurrence lineage when the pipeline accepts the result.

The framework may transfer a private successful B7 occurrence capability to
the pipeline. It cannot call that object `CanonicalBir`. B8 verification and
publication are separate pipeline/verifier authorities.

## Target and ABI Rules

Pass framework code and canonical passes may depend only on core, analysis,
verification and structured diagnostics interfaces. They cannot depend on
legacy compatibility views, preparation products, target backends, MIR,
calling placement, frame/allocation/spill state or renderer output.

Raw and Canonical BIR may preserve source-semantic typed sizes, alignments, and
address spaces owned by their accepted rows. They contain no semantic
`target_profile`, rendered `data_layout`, target triple, pointer-width/address-
space layout selection, or other C1/C2 target context. C1 independently selects
the exact `TargetProfile`, and C2 derives target layout. A canonical pass cannot
inspect or reconstruct those facts, calling-convention placement policy, legal
machine opcode sets, or backend feature switches.

## Re-entry, Repetition, and Orchestration Support

The framework validates `Once`, `Idempotent` and explicitly bounded
`FixedPointMember` contracts, but the pipeline alone decides whether/where a
configured occurrence appears. Initial canonical P01-P07 are each one ordered
occurrence and their documents require idempotence; no hidden recursion or
earlier-pass callback is allowed.

Framework checkpoint helpers may consume/fork/discard owning storage, but the
pipeline alone defines last-good retention, checkpoint serialization,
resumption strictly after a completed stamp and B8 admission.

## Implementation State

Implementation is partial foundation only:

- checked in and build-included: deterministic `CancellationToken`/work budget
  in `execution_control.*`;
- checked in and build-included under pipeline storage: revision digest/stamp
  identity plus internal consume-Raw, fork, view and discard foundations;
- absent: closed pass descriptors/registry, pass sessions/edit transactions,
  analysis manager integration, complete occurrence publication, property/
  postcondition/verifier orchestration, instrumentation and B7 capability path.

No P01-P07 semantic pass implementation is present. Foundation code does not
constitute a callable canonical pipeline or `CanonicalBir` publication route.

## Proof Requirements

- validate descriptor IDs/kinds/properties/analysis contracts and reject every
  registry mismatch;
- prove one transaction per invocation, atomic occurrence rollback,
  deterministic parallel waves and core-derived revisions/summaries;
- test stale/foreign analysis rejection, mutation-specific preservation and
  transitive invalidation;
- prove framework cannot choose order/re-entry, define verifier semantics,
  consume target facts or mint `CanonicalBir`;
- reconcile every implementation claim with storage, build inclusion and
  callable reachability.

## Open Questions

Concrete C++ APIs for the absent registry/session/occurrence integration remain
implementation work; they cannot weaken this authority split.

## Review Checklist

- [x] Metadata, core ownership and substantive matrices are present.
- [x] P01-P07 identities/kinds/properties/analyses are exact.
- [x] Transactions, occurrence barriers and rollback are distinct and atomic.
- [x] Analysis access/invalidation never retargets stale handles.
- [x] Pipeline order/capability and verifier semantics remain external owners.
- [x] Accepted Raw/Canonical target-context paragraph is retained.
- [x] Partial-foundation implementation truth is explicit.
