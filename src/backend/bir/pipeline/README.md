# Ordered BIR Pipeline Contract

Status: design contract; implementation has not started.

The root [`BIR README`](../README.md) owns the normative `S00`-`S29` total
order. This document is subordinate to that order and expands orchestration
only for its `S02`-`S09` canonicalization interval. Individual pass documents
own local algorithms, but neither they nor this document may insert a stage,
change a predecessor, weaken an input profile, or reorder a root row. An order
change starts in the root contract and must update both affected adjacent
contracts in the same review.

The pipeline is a canonicalization boundary, not a target backend hidden under
a pass-manager name. It accepts one already typed and Raw-verified semantic
module and publishes one immutable target-independent `CanonicalBir`. Target
preparation, generic pseudo and shared call lowering, target pseudo
legalization, out-of-SSA, and shared BIR allocation are downstream of this
canonicalization pipeline. Those owners publish new exact pseudo/allocated
revisions and never write their facts into `CanonicalBir`. Concrete registers, frame offsets, target opcodes,
instruction selection, prologue/epilogue, and emission remain later MIR/backend
authority.

## 1. Root-order anchor and local stage graph

The root rows consumed and produced by this local runner are:

```text
S00 LIR import -> private frozen ModuleDraft
S01 Raw verification/publication -> RawBir
S02 P01 legalize -> S03 P02 scalar -> S04 P03 cfg -> S05 P04 ssa
  -> S06 P05 memory -> S07 P06 aggregate -> S08 P07 intrinsics
S09 Canonical verification/publication -> CanonicalBir
S10 TargetProfile selection/validation (outside run_bir_pipeline)
```

This excerpt is an anchor, not a second order registry. The complete flow,
including `S11`-`S29`, the `S25 -> S23` allocation retry edge, every verifier
gate, and every target-aware phase is defined only in the root README.

`ModuleDraft` and `RawBir` are defined by
[`core`](../core/README.md), import by
[`lir_to_bir`](../lir_to_bir/README.md), and publication profiles by
[`verify`](../verify/README.md). In particular:

- `ModuleDraft` is unpublished, frozen candidate storage;
- only `verify_and_publish_raw(ModuleDraft&&)` may mint `RawBir`;
- `RawBir` is move-only, fully typed, structurally valid, and verified with the
  complete Raw rule registry;
- a pass never repairs an unresolved reservation, malformed type, textual
  operand, missing terminator, foreign ID, invalid call bundle, or importer
  failure;
- `CanonicalBir` is minted only from the last successful pipeline stage stamp by
  the Canonical publication gate;
- `PreparedInput` is a cumulative verifier profile, not another mutable BIR
  stage and not a container of prepared facts;
- `VerifiedPreparationInput` is a short-lived immutable capability tying a
  `CanonicalBir` view, target context, verification report, and exact revision
  axes/function-revision digest together for the external preparation API. It
  borrows the immutable `CanonicalBir` storage and cannot outlive that owning
  capability;
- target layout publishes an immutable capability bound to that exact complete
  stage stamp and target fingerprint; preparation products additionally bind
  the layout and ordered predecessor-product fingerprints;
- preparation dependency order is exactly `abi -> calls -> variadic -> address
  -> inline_asm -> runtime_helpers`; after atomic cumulative-bundle
  publication, `regalloc/constraints` alone interprets and binds source
  constraint descriptions to ordinary operands/results;
- D1 privately forks from Canonical into the closed pseudo schema, D2 completes
  shared ABI-aware call transport, D3 publishes the first verified `PseudoBir`,
  and D4 fully reverifies target realizability before out-of-SSA;
- shared BIR allocation consumes that later exact pseudo lineage and may
  publish a distinct immutable allocated revision only after complete
  assignment/spill verification;
- `PreparedBir` is the later capability over that allocated revision, not a
  synonym for `VerifiedPreparationInput` or preparation facts;
- `MirReadyBirView` is a read-only view of the same allocated graph.

There is no `src/backend/bir/mir` stage or namespace. Preparation, layout,
allocation, and allocated-publication ownership are documented under their
existing BIR directories; target MIR remains an external consumer of the
verified `MirReadyBirView`.

## 2. Immutable built-in canonical occurrence sequence

Within the root `S02`-`S09` interval, the required canonical occurrence
sequence is exactly:

```text
P01 legalize
P02 scalar
P03 cfg
P04 ssa
P05 memory
P06 aggregate
P07 intrinsics
G01 canonical publication gate
```

The seven named entries are mandatory in the initial implementation. `G01` is
an unconditional publication gate, not a pass occurrence. The separately
invoked prepared-input gate is not part of `run_bir_pipeline`; it consumes the
published `CanonicalBir` only when an external preparation caller supplies a
target context. A profile may select another implementation algorithm only
when it produces the same canonical semantic fingerprint, ordered diagnostics,
postconditions, and failure class. It may not omit or reorder an entry.

The scaffold order has been challenged against its dependencies and is
retained with two strict consequences:

1. `aggregate` and `intrinsics` are **canonicalizers**, not lowering passes.
   They may normalize descriptors but must not expand an aggregate or
   intrinsic into fresh noncanonical scalar, CFG, SSA, or memory forms after
   those owners have run. Such expansion belongs in an explicitly reviewed
   earlier pass or outside Canonical BIR.
2. `memory` may normalize semantic addresses and effects, but it may not select
   address modes, assign storage, or split target operations. It must preserve
   the canonical CFG and SSA profiles it receives.

If either consequence proves impossible, the correct design change is to
split and reorder named passes here. Silently invoking an earlier pass from a
later pass, or running an undocumented cleanup pass, is forbidden.

## 3. Adjacent-stage contract table

| Entry | Accepted input | Required postcondition / next-pass contract | Forbidden authority |
|---|---|---|---|
| `legalize` | exact verified `RawBir`; all IDs, types, operands, terminators, call effects, asm payloads, globals and initializers are valid; documented Raw-only semantic forms may remain | every `legalize`-owned Raw form is eliminated or converted to its typed portable handoff; widths, constants, casts, effect descriptors and opcode families satisfy the legal semantic type universe accepted by `scalar`; forms owned by later entries, including opaque inline-asm template and constraint payload, remain lossless and unchanged except for typed reference repair caused by legalize edits | ABI locations, target register width policy disguised as semantic legality, target opcode choice, malformed-import repair |
| `scalar` | legal semantic types and operations; no unresolved `legalize`-owned Raw scalar, type, constant, or boolean-boundary form; lossless forms owned by `cfg`/`ssa`/`memory`/`aggregate`/`intrinsics` or a root-declared later stage may remain | scalar ops, casts, comparisons, select conditions, integer/floating exceptional behavior, undef/poison policy and helper-eligible semantic operations have unique canonical descriptors; opaque inline-asm template and constraint payloads remain unchanged; output is accepted by CFG mutation and never relies on rendered comparison text | branch fusion, runtime-helper selection, instruction selection, physical flag/register state |
| `cfg` | canonical scalar conditions and valid terminators as sole edge authority | reachable block set, successor/predecessor edge keys, branch/switch/indirect/asm-goto semantics, block order policy and phi/block-argument edge updates are canonical; all edge edits are atomic; output admits dominance and SSA construction | label spelling as a graph key, prepared branch records, machine fallthrough layout, out-of-SSA copies |
| `ssa` | canonical CFG with exact parallel-edge identity and complete def-use | each value definition and use obeys the chosen canonical SSA form; phi/block-argument incoming keys cover exact predecessor edges; dominance holds; trivial aliases are normalized; output admits memory analysis and mutation | physical homes, phi move scheduling, spill slots, MIR parallel copies |
| `memory` | canonical SSA plus typed semantic loads, stores, GEP/ptr-offset, atomics, stack-save/restore and memory-intrinsic descriptors | memory/access/address forms expose target-independent object, offset, alignment, volatility, ordering, scope and effect operands needed for recomputable analyses; output preserves CFG/SSA and admits aggregate descriptor normalization | address modes, frame offsets, alias conclusions stored as truth, ABI by-value placement |
| `aggregate` | canonical scalar, CFG, SSA and memory profiles; resolved record/array/union/complex types | aggregate values, copies, extracts/inserts, layout-independent aggregate paths and by-value semantic boundaries have unique forms; it preserves memory-canonical GEP/address descriptors and emits no earlier-stage noncanonical operation | target layout decomposition, sret/register classification, stack copy sequence, target lane choice |
| `intrinsics` | all preceding canonical profiles and structured intrinsic/inline-asm semantic payloads | intrinsic namespaces, signatures, effects, atomics represented as intrinsics, runtime-helper-eligible operations and opaque semantic inline asm satisfy the final Canonical profile; unsupported semantics fail with diagnostics | helper symbol choice, asm constraint realization, clobber registers, target instructions |
| Canonical publication | successful `P07` candidate with a complete frozen stage stamp | full Canonical verifier passes on that same frozen module revision and ordered function-revision digest, then mints `CanonicalBir` atomically | partial publication or “verified earlier” shortcuts |
| Prepared-input gate | immutable `CanonicalBir`, explicit `TargetContext`, no canonical edit capability | cumulative `PreparedInput` verification succeeds and returns a borrowing `VerifiedPreparationInput` bound to the complete `PipelineStageStamp` plus target fingerprint; layout/preparation/constraints consume it, then D1 may fork a separate pseudo candidate | writing target/pseudo/allocation facts into Canonical BIR, calling this gate's result `PreparedBir`, or treating preparation facts as an instruction graph |

No pass may loosen its predecessor's postconditions. Every later pass either
preserves those profiles or fails its transaction.

### 3.1 Exhaustive first-owner map for Raw-only forms

This table is the pipeline transcription of the accepted import inventory. A
form may be *validated* by an earlier entry, but only its first owner may
normalize or eliminate it. Importer-only forms do not escape `RawBir`.

| Raw/import-boundary form | First owner | Required disposition before the next entry |
|---|---|---|
| old/new duplicate LIR shapes, reservations and forward-reference shells | importer/publication | unified or resolved before `RawBir`; no canonical pass accepts a compatibility alternative or unresolved reservation |
| arbitrary-width SSA integers, exact FP payload encoding, raw scalar opcode/predicate/cast aliases, non-`I1` truth uses | `legalize` | converted to typed portable semantic operations/bridges or rejected; exact values remain lossless |
| original inline-asm template and constraint strings | root stage `S18` | preserved byte-for-byte as opaque payload by `P01`-`P07`; interpreted and bound only at `S18` |
| portable scalar expression, compare, cast, select and helper-eligible semantic operation shapes | `scalar` | one canonical target-independent scalar descriptor; helper identity remains undecided |
| raw switch ordering, indirect target sets, unreachable blocks and noncanonical branch/block shape | `cfg` | exact `EdgeKey`-based canonical CFG with no name- or fallthrough-derived authority |
| resolved but noncanonical phi placement/incoming order, promotable local memory and trivial SSA aliases | `ssa` | chosen SSA form, dominance and exact parallel-edge incoming coverage hold |
| multi-index GEP, semantic address paths, load/store/access forms, atomics, stack-save/restore and memory-intrinsic descriptors | `memory` | one target-independent memory/address/effect representation; no address mode or physical storage decision |
| aggregate insert/extract/copy/path forms and typed source-level by-value or hidden-result semantics | `aggregate` | one aggregate semantic form; source spellings such as `sret` are not ABI placement authority |
| semantic/feature intrinsic identity, final intrinsic effects, structured opaque inline-asm semantic payload and top-level asm dependency registry | `intrinsics` | final Canonical registry/signature/effect form or fail-closed unsupported diagnostic; no helper symbol, constraint allocation or target opcode choice |

Typed calls, globals, relocations, initializers, declarations, call bundles and
call effects are already semantically complete at Raw publication. Entries may
check and preserve their cross-module consistency, but may not recover missing
facts from legacy routes, names, rendered text, or target/preparation state.

## 4. Proposed public C++ API

The pass framework owns `PassId`, `PassKind`, `PipelineOccurrence`,
`PassVerifyOptions`, pass contexts/results, preservation sets and mutation
summaries. This pipeline only owns its immutable occurrence list, runner policy,
audit and stage-level result. The following names are concrete design targets;
they intentionally do not redeclare framework types.

```cpp
namespace c4c::backend::bir {

enum class PipelineProfile : std::uint8_t {
  Debug,
  Release,
  ReproducibilityCheck,
};

class CanonicalPipelinePlanView final {
 public:
  std::span<const PipelineOccurrence> entries() const;
  Hash128 schema_fingerprint() const;
};

const CanonicalPipelinePlanView& canonical_pipeline_v1();

struct PipelineOptions final {
  PipelineProfile profile = PipelineProfile::Release;
  PassVerifyOptions verification;
  std::uint32_t max_function_workers = 1;
  bool retain_reentry_checkpoint_on_failure = true;
  bool collect_audit_events = true;
  DiagnosticOptions diagnostics;
};

// Whole-stage freshness stamp. ModuleRevision covers module-owned tables;
// function_revisions covers every body in canonical FunctionId order.
struct PipelineStageStamp final {
  ModuleEpoch epoch;
  ModuleRevision module_revision;
  FunctionRevisionDigest function_revisions;
};

struct PassStatistics final {
  PassId id;
  std::uint32_t ordinal;
  std::uint32_t iterations;
  std::uint64_t attempted_mutations;
  std::uint64_t committed_mutations;
  std::uint64_t analyzed_functions;
  PipelineStageStamp before;
  PipelineStageStamp after;
};

struct PipelineAudit final {
  Hash128 plan_fingerprint;
  Hash128 options_fingerprint;
  Hash128 input_semantic_hash;
  Hash128 output_semantic_hash;
  PipelineStageStamp input_stamp;
  PipelineStageStamp output_stamp;
  std::vector<PassStatistics> passes;
  std::vector<AuditEvent> events;
};

// Opaque owning capability over a committed pipeline candidate. Its storage
// representation must be supplied by the core/pass-framework checkpoint
// prerequisite; pipeline code must not name or share raw ModuleStorage.
class PipelineCheckpoint final {
 public:
  PipelineCheckpoint(PipelineCheckpoint&&) noexcept;
  PipelineCheckpoint& operator=(PipelineCheckpoint&&) noexcept;
  PipelineCheckpoint(const PipelineCheckpoint&) = delete;
  PipelineCheckpoint& operator=(const PipelineCheckpoint&) = delete;
  ~PipelineCheckpoint();

  ModuleView view() const;
  PipelineStageStamp stamp() const;
  PropertySet established_properties() const;
  std::uint32_t next_ordinal() const;
  Hash128 plan_fingerprint() const;
  Hash128 options_fingerprint() const;
};

struct PipelineSuccess final {
  CanonicalBir module;
  DiagnosticSet diagnostics;
  PipelineAudit audit;
};

enum class PipelineFailureKind : std::uint8_t {
  InvalidPlan,
  InputRevisionMismatch,
  PassRejectedInput,
  PassFailed,
  PassViolatedContract,
  VerificationFailed,
  FixedPointBudgetExceeded,
  NonDeterministicResult,
  Cancelled,
};

struct PipelineFailure final {
  PipelineFailureKind kind;
  std::optional<PassId> pass;
  std::optional<FunctionId> function;
  std::uint32_t iteration;
  DiagnosticSet diagnostics;
  std::optional<PipelineCheckpoint> continuation;
  PipelineAudit partial_audit;
};

Result<PipelineSuccess, PipelineFailure> run_bir_pipeline(
    RawBir&& input, PipelineOptions options = {});

Result<PipelineSuccess, PipelineFailure> resume_bir_pipeline(
    PipelineCheckpoint&& checkpoint, PipelineOptions options = {});

struct TargetContext;
struct PreparationInputFailure;

class VerifiedPreparationInput final {
 public:
  ModuleView module() const;
  PipelineStageStamp stamp() const;
  TargetFingerprint target_fingerprint() const;

 private:
  friend Result<VerifiedPreparationInput, PreparationInputFailure>
  verify_preparation_input(const CanonicalBir&, const TargetContext&,
                           VerifyOptions);
};

Result<VerifiedPreparationInput, PreparationInputFailure>
verify_preparation_input(const CanonicalBir& module,
                         const TargetContext& target,
                         VerifyOptions options = {});

} // namespace c4c::backend::bir
```

`canonical_pipeline_v1()` stores the seven entries explicitly. It is not
assembled by filesystem discovery, registration order, linker initialization,
environment strings, or unordered containers. `ordinal` is stable and checked
against `PassId`; duplicate, missing, or out-of-order mandatory entries make
runner initialization fail before the input is consumed. There is deliberately
no public plan builder and `run_bir_pipeline` accepts no caller-supplied pass
list. A future sequence is a new reviewed, versioned pipeline entry point, not
an options mutation of `canonical_v1`.

`PipelineStageStamp` is a pipeline aggregate of the framework/core revision
axes, not a replacement for invocation-local `RevisionStamp`. Equality checks
all three fields. The ordered function digest is computed over
`(FunctionId, FunctionRevision)` in canonical function order; a
`ModuleRevision` match alone never establishes freshness of function bodies.

## 5. Pass implementation interface

The runner invokes only the closed `FunctionPass` and `ModulePass` interfaces
defined by [`passes/README.md`](../passes/README.md). That document exclusively
owns pass contexts, sessions, results, preservation declarations,
`MutationSummary` validation, budgets and the registry. This file assigns each
occurrence to one registered implementation and supplies its barriers; it does
not wrap those interfaces in a second type system.

Within `S02`-`S09`, the runner sequences transactions, verification and stage
publication.
A pass cannot construct a session, commit, mint a stage token, clear a
diagnostic or retain mutable storage. A false no-change result, undeclared edit
or invalid preservation claim is a framework contract violation and fails the
current pipeline transaction.

## 6. Function and module barriers

Each entry names exactly one framework `PassKind`:

- `Function`: one isolated transaction per function definition. Declarations
  are visited only when the pass contract explicitly includes them.
- `Module`: one exclusive module transaction; no function worker or borrowed
  analysis may be active.

There is no hybrid third pass kind. A module implementation may perform a
read-only analysis/planning wave before acquiring its exclusive edit session,
but it remains one registered `ModulePass` and publishes through one module
transaction. A function implementation may participate in the framework's
atomic function-wave publication, but it remains one registered `FunctionPass`.

The initial scope plan is:

| Pass | Scope | Barrier reason |
|---|---|---|
| `legalize` | `Module` | globals, declarations, definitions and shared type/symbol tables must agree; its typed whole-module plan and edits publish once |
| `scalar` | `Function` | definitions are independent once shared types are legal |
| `cfg` | `Function` | edges never cross function ownership |
| `ssa` | `Function` | dominance and definitions are function-local |
| `memory` | `Module` | global-object references and module memory consistency require one whole-module transaction; derived effect summaries remain analysis results |
| `aggregate` | `Module` | named aggregate/type identities and by-value boundaries must agree across declarations and definitions |
| `intrinsics` | `Module` | intrinsic declarations/signature registry and top-level asm dependencies are module-level |

No function pass may add, remove, rename, or change linkage/type of a function,
global, symbol, named type, intrinsic declaration, or top-level asm object.
Those are cross-function mutations and require a registered module entry.

## 7. Deterministic parallel scheduling

Function-scope work may run concurrently only when
`max_function_workers > 1` **and** core provides the atomic function-wave
proposal/merge primitive required by the pass framework. Until that prerequisite
exists, the normative runner clamps the worker count to one and evaluates each
function invocation serially in canonical `FunctionId` order **inside one
unpublished occurrence candidate**. The complete occurrence publishes once;
failure discards that candidate and retains the pre-occurrence checkpoint. It
must not expose intermediate function commits or simulate rollback after edits
became externally visible. This serial candidate/checkpoint facility is itself
a core/pass-framework implementation prerequisite; if unavailable, runner
initialization fails before consuming `RawBir`. Once the atomic parallel merge
primitive exists, parallelism changes latency, never semantics:

1. Snapshot the complete `PipelineStageStamp` and enumerate eligible
   definitions in canonical `FunctionId` order.
2. Give each worker an immutable module view, one function transaction, a
   function-local diagnostic/statistic/audit buffer, and revision-keyed
   analyses.
3. Prohibit worker access to another function's editor or mutable module
   indexes. Cross-function facts are immutable inputs for the whole wave.
4. Collect proposals without committing them to shared storage.
5. Sort proposals by canonical `FunctionId`; within a proposal, preserve the
   editor journal's stable entity order.
6. Merge diagnostics by `(pass ordinal, iteration, FunctionId, entity order,
   RuleId, source location)`; merge statistics by `(closed StatisticId,
   FunctionId)` with checked arithmetic; merge audit events by their stable
   occurrence/function/entity key. None uses worker completion time.
7. If any proposal fails, discard every proposal in that wave. There is no
   partial function-wave commit.
8. Recheck the module epoch/revision and every proposal's base
   `{FunctionId, FunctionRevision}` against the original stage stamp, commit
   the complete sorted wave, update the ordered function-revision digest, then
   release the next pass barrier. A body-only wave does not bump
   `ModuleRevision`.

Work stealing is permitted internally, but must not influence ID allocation.
New entity IDs come from deterministic transaction-local reservation ranges or
from the sorted merge, not a contended global counter.

`ReproducibilityCheck` runs the same plan with two legal schedules (normally
one worker and configured parallel workers) and compares canonical semantic
hashes, diagnostics and audit mutation identities. A mismatch returns
`NonDeterministicResult` and publishes neither candidate. The runner forks two
private candidates from one framework-owned committed checkpoint; it does not
copy `RawBir`, `CanonicalBir`, or any other move-only stage token. Before the
atomic wave prerequisite exists, reproducibility checks deterministic repeated
serial execution and records that parallel proof is unavailable.

## 8. Transactions, revisions and last-good state

Each pass attempt starts from one immutable, verified last-good stage stamp.

```text
last-good committed checkpoint + complete PipelineStageStamp
  -> framework-owned private occurrence stage fork
  -> one independent transaction per framework invocation
  -> invocation result + editor journal + local verifier gate
  -> occurrence postconditions on the complete private fork
  -> atomic verified fork promotion
  -> new last-good committed checkpoint
```

Rules:

- no mutation is visible before commit;
- an unchanged pass cannot bump either revision axis;
- each changed function invocation bumps only that `FunctionRevision` exactly
  once; a body-only function edit or complete function wave does **not** bump
  `ModuleRevision`;
- a function wave atomically publishes all changed function revisions and a
  newly checked ordered function-revision digest while retaining the same
  module revision;
- a module commit bumps `ModuleRevision` exactly once and bumps affected
  function revisions only when the core mutation contract says their
  observable function view changed;
- IDs created in a rolled-back transaction never resolve in a later revision;
- analyses, views, checkpoints, audit records and preparation capabilities
  retain the complete applicable revision stamp; any whole-stage consumer that
  reads bodies also retains the ordered function-revision digest and rejects a
  mismatch;
- `RawBir` is consumed by the run, but its backing snapshot and complete stage
  stamp remain the first last-good checkpoint until the first successful
  commit;
- `CanonicalBir` is not created until final verification succeeds.

On pass error, verifier error, cancellation, budget exhaustion or merge
conflict, the current invocation transaction is rolled back and the complete
private occurrence fork is discarded. Later passes do not run.
`retain_reentry_checkpoint_on_failure` controls whether the single owning
`PipelineCheckpoint` is moved into `PipelineFailure::continuation`. No
move-only stage object is copied into a success or failure. The checkpoint does
not convert to `CanonicalBir`; only `resume_bir_pipeline(PipelineCheckpoint&&)`
may consume it after the re-entry checks in Section 13.

## 9. Analysis manager contract

Analyses are derived, disposable and revision-bound. They are neither BIR
fields nor hidden channels between passes.

The closed `AnalysisId` registry and `AnalysisManager::require` API are owned
by [`passes/README.md`](../passes/README.md) and
[`analysis/README.md`](../analysis/README.md). The pipeline names no competing
IDs or query signatures; it only enforces invalidation at occurrence barriers.

An analysis key includes `AnalysisId`, module epoch, module revision, optional
function ID/revision, algorithm/schema version, and relevant options. A
module-wide analysis that reads bodies additionally includes the ordered
function-revision digest; module revision alone is insufficient. Analyses may
share prerequisite results, but dependency edges are explicit.

Before an occurrence fork becomes the next checkpoint, the runner compares
its exact `MutationSummary` with `PreservedAnalyses`. It invalidates all
unpreserved results transitively and rebinds a preserved result only through
the analysis manager's checked operation. At a
minimum:

- CFG mutation invalidates reachability, dominance, post-dominance, loop,
  liveness and path-sensitive provenance/effects;
- operand/def-use mutation invalidates def-use, liveness, memory effects and
  dependent provenance;
- call/intrinsic effect mutation invalidates call graph and memory effects;
- type/object/address mutation invalidates memory effects and provenance;
- module symbol/global/call-edge mutation invalidates module call graph and
  every dependent function analysis.

Preservation is checked against actual mutations. An analysis object cannot be
carried across a revision merely because its pointer remains alive. Passes ask
the manager again after every commit barrier.

## 10. Verifier gates

Verification is independent of pass success. Returning `Changed` is not proof
of a valid candidate.

| Policy/profile | Gate behavior |
|---|---|
| Release default | validate editor journal and pass-specific postconditions after every entry; run full Canonical verification at publication |
| `AfterMutatingPass` | additionally run framework `verify_after_edit` (or its requested full fallback) after every changed entry/wave |
| Debug default | `AfterEveryPass`: verify the candidate plus all accumulated `PassProperty` postconditions after every entry, including unchanged entries; expensive preservation and deterministic-order assertions enabled |
| Reproducibility | Debug gates for both schedules plus semantic-hash/audit comparison |

`PipelineProfile::Debug` upgrades a weaker requested verification policy; a
caller cannot disable final Canonical publication verification. A future
sampling mode may reduce internal checks, but must never sample publication.

Pass-specific properties are cumulative: after `cfg`, Raw-verified + legal +
scalar + CFG properties are checked; after `ssa`, those plus SSA; and so on.
They are framework properties, not invented verifier profiles. The verifier's
public profile names remain `Raw`, `Canonical`, and `PreparedInput`. The final
gate freezes one exact candidate stage stamp, runs the complete Canonical
registry, and atomically consumes the successful candidate into
`CanonicalBir`.

The prepared-input gate is separate because it also sees `TargetContext`. It
may reject a canonical semantic feature unsupported by that preparation
implementation, but cannot mutate or weaken Canonical BIR.

## 11. Fixed-point groups and convergence

The initial seven canonicalizers are required to be idempotent after one
successful invocation. The built-in v1 plan therefore has no implicit outer
fixed-point loop. Each pass may use a documented, internally bounded worklist
to reach its own postcondition; iteration count and mutation count are audited.

The API includes explicit fixed-point groups for future reviewed pipelines.
Their contract is:

- a group is a contiguous, statically listed sequence of framework
  `PipelineOccurrence` objects in a newly reviewed pipeline version;
- every iteration runs those entries in the same order;
- before the first invocation, the framework forks one private owning stage
  from the pre-group checkpoint; this is a stage-storage fork, **not** an outer
  editor transaction and not a nested pass invocation;
- the runner invokes every registered pass normally and sequentially against
  that private fork. Each function/module invocation opens at most one editor
  transaction and independently commits or rolls back exactly once according
  to the pass-framework state machine; successful private commits advance the
  correct revision axes on the fork and invalidate its analyses;
- a successful invocation commit becomes the framework-published input for the
  next invocation **within that fork** and has an ordinary before/after
  `RevisionStamp`; it remains unreachable to pipeline callers and is not a
  resumable `PipelineCheckpoint`;
- iteration boundaries are runner bookkeeping only. They are not editor
  savepoints, cannot keep an editor open across invocations, and do not permit
  one pass to invoke another;
- no private-fork revision becomes a public pipeline checkpoint until one full
  no-change iteration proves convergence. The framework then atomically
  promotes the complete verified fork as the next stage checkpoint;
- entry skipping based on previous change counts is forbidden unless encoded
  in a new immutable plan version;
- convergence means every entry reports `Unchanged` and editor journals confirm
  zero mutation during one complete iteration;
- semantic-hash equality alone does not excuse a pass that mutates and restores
  the same state;
- both `max_iterations` and `max_total_mutations` are hard deterministic
  budgets; exhaustion is `FixedPointBudgetExceeded` and rolls back the entire
  currently open invocation, discards the private fork, and retains the exact
  pre-group checkpoint/stamp;
- wall-clock budget is telemetry unless the caller explicitly requests
  cancellation semantics, because timing-based output is not reproducible;
- the pre-group checkpoint remains last-good until convergence; no partially
  completed iteration or nonconverged group can be resumed.

This contract requires framework/core primitives for private owning stage
fork, independent invocation commits on that fork, and atomic verified fork
promotion. If any primitive is unavailable, a plan containing a fixed-point
group fails validation before consuming its input. An implementation must not
approximate the group with a long-lived outer editor, nested editor
transactions, cross-invocation savepoints, or revision rollback.

This rejects the reference compiler's heuristic “three iterations plus
diminishing returns” as a correctness convergence rule. Its explicit repeated
order and dirty-function idea are useful, but output must not depend on timing,
unordered completion, or a change-count percentage.

## 12. Enablement and profile policy

The environment does not edit the pass list. CLI and configuration are parsed
into typed `PipelineOptions`, validated, fingerprinted, and recorded before
execution.

- mandatory canonicalizers cannot be disabled, substituted, skipped or
  repeated through `PipelineOptions`; v1 has no optional occurrence;
- a “fast” algorithm and a “thorough” algorithm behind the same `PassId` must
  produce the same canonical semantic fingerprint, deterministic ordered
  diagnostics, mutation-independent postconditions and failure class;
- debug/release affects checks, telemetry and parallelism defaults, not order;
- optimization level must not cause malformed or noncanonical BIR to bypass a
  pass;
- unknown pass names, duplicate toggles and incompatible budgets are plan
  errors, not ignored strings;
- target feature flags are unavailable to canonical passes except where the
  semantic type universe explicitly requires a target-independent capability.
  Target support decisions belong at `PreparedInput` or later.

## 13. Stage re-entry and resumption

Normal APIs are single-directional. A caller cannot cast a `CanonicalBir` back
to `RawBir`, run `P04` directly, or resume a failed transaction.

Permitted re-entry is only the `resume_bir_pipeline(PipelineCheckpoint&&)` API
in Section 4. A checkpoint may be created only by the runner at a committed
pass barrier and carries one unforgeable, owning stage capability. Resume
consumes that capability exactly once; success, failure or validation rejection
leaves the caller with no reusable token. Before running the successor it
rechecks the complete `PipelineStageStamp` including the ordered
function-revision digest, accumulated framework properties, the applicable
verifier rules, plan/options fingerprints, and that `next_ordinal` is exactly
the successor of the last completed entry. A matching module revision with a
different function digest is stale. Checkpoints are process-local until a
separately versioned serialization format exists. Source edits, target changes,
order changes, algorithm options, verifier-schema changes, or an already
consumed token invalidate re-entry.

`PipelineCheckpoint::view()` creates an ordinary revision-bound immutable
borrow. Resume requires all such borrows to be destroyed before the checkpoint
is moved; a live borrow is a deterministic re-entry failure, never a reason to
clone the owning capability.

Re-running the whole pipeline requires a new `RawBir` from the importer. It is
not stage re-entry. Running only `verify_preparation_input` again with another
target is allowed because `CanonicalBir` is immutable and preparation facts are
external.

## 14. Diagnostics, audit and provenance

Every diagnostic carries:

- stable diagnostic/rule ID and severity;
- pass ID, ordinal and fixed-point iteration;
- module epoch/revision and optional function revision;
- stable entity ID plus structured source provenance when present;
- concise message, related entities and causal diagnostic IDs;
- whether it arose from pass rejection, pass contract checking, analysis,
  verifier, merge, budget or cancellation.

Passes emit diagnostics through their transaction-local sink. They cannot print
directly, use rendered BIR as an error protocol, or make success depend on
diagnostic formatting.

Audit events record the plan/options fingerprints, compiler/schema version,
input/output semantic hashes, complete before/after `PipelineStageStamp`s,
invocation-local framework `RevisionStamp`s, requested analyses and their
revision/digest keys, pass start/finish, mutation categories,
preservation/invalidation decisions, verifier profiles/reports, worker count,
deterministic merge order, rollback, budgets and final publication. Source
provenance remains non-authoritative metadata: it explains where semantics came
from but never substitutes for a typed operand or ID.

Reproducible output requires canonical iteration order for modules, functions,
blocks, instructions, operands, diagnostics, audit events and hash inputs.
Pointer values, allocation addresses, thread IDs, wall time and unordered-map
iteration are excluded from semantic hashes.

## 15. Legacy capability coverage

The legacy tree is evidence for capabilities, not a shape to preserve.

| Legacy capability | New owner / disposition |
|---|---|
| `lir_to_bir.cpp`, `bir_route1.cpp` producer identity, route facade | importer plus core stable IDs; completed before this pipeline; route spellings/facades are not copied |
| route 2/3 memory and provenance records | semantic operands survive in core; `memory` canonicalizes representation; effects/provenance remain revision-keyed analyses |
| route 4/5 publication availability and block-entry records | exact def-use, CFG edge identity and dominance/SSA replace publication side tables |
| route 6 call publication | typed call bundles/effects are Raw input; call ABI placement is external preparation |
| route 7 comparison view | `scalar` owns comparison/select semantics; `cfg` consumes typed conditions; no trailing-compare text search |
| route 8 and select-dependency views | explicit typed dependencies and def-use; recomputable analyses, not authoritative route records |
| `prealloc/legalize.cpp` type/value normalization | target-independent portion moves to `legalize`; its ABI repair, target i1 promotion policy, prepared label tables and branch-fusion records are rejected from Canonical passes |
| `prealloc/control_flow.hpp`, label lookup tables | CFG analysis plus `cfg` using `BlockId` and `EdgeKey`; prepared label interning is unnecessary |
| `prealloc/comparison.*` | scalar canonical forms and comparison analysis; physical flags/branch fusion remain later |
| `prealloc/addressing.hpp`, memory freshness, atomics, object data | memory/aggregate/intrinsic semantic canonicalization plus analyses; target addresses and storage plans remain preparation/MIR |
| `prealloc/liveness.*` | reusable revision-bound BIR analysis when needed; never a canonical pass or stored authority |
| `prealloc/out_of_ssa.cpp` phi materialization, join transfers, parallel-copy bundles | rejected from the canonical interval; root stage `D5` BIR out-of-SSA owns it after target legalization and before `E1` allocation liveness |
| `prealloc/regalloc.cpp`, allocation constraints, spill/reload and move bundles | typed target preparation plus shared BIR allocation own abstract constraints, assignments, and capacity `Spill`/`Reload` in a new allocated revision; MIR owns concrete mapping and machine moves; no allocation fact is written to Canonical BIR |
| stack layout, dynamic stack plan, frame plan, storage plan | semantic stack-save/restore/allocation operations remain BIR; physical slots/frame policy are external |
| call plans, variadic entry plans, inline-asm carriers, runtime-helper facts | `intrinsics` guarantees typed semantic inputs; external typed preparation owns classifications and plans |
| prepared lookups, traversal coordinates and agreement tables | replaced by stable IDs, immutable typed plan handles and revision checks; no duplicate authority |
| prepared printer and legacy notes/completed-phase strings | structured diagnostics/audit; strings are presentation only |

The old `BirPreAlloc::run()` order (`legalize -> stack layout -> liveness ->
out_of_ssa -> regalloc -> publish plans`) is therefore not the new BIR pass
order. It mixed semantic cleanup, analyses, allocation, machine transition and
publication in one mutable object. Only its capability inventory is retained.

## 16. Reference compiler: adopt and reject

Evidence reviewed under `ref/claudes-c-compiler/src/backend` and its directly
used `src/passes` orchestration leads to these decisions:

| Reference behavior | Decision |
|---|---|
| one explicit top-level pass list | adopt; the plan is inspectable and fingerprinted |
| repeated CFG cleanup around transforms | adopt only when represented as distinct ordered entries in a future plan; never hidden recursion |
| per-function dirty sets | adopt as an optimization only when skipping cannot bypass a mandatory postcondition and produces identical audit semantics |
| shared CFG/dominator/loop computation for GVN/LICM/IVSR | adopt the explicit analysis manager and preservation model |
| fixed backward liveness dataflow with dense bitsets | useful analysis evidence, outside canonical mutation authority |
| centralized canonical operand traversal | adopt through core descriptors/def-use and analysis APIs |
| all optimization levels exercising one sequence while compiler matures | adopt the reliability motivation; profiles cannot reorder canonicalization |
| environment substring matching to disable passes | reject; use validated typed options and exact `PassId` |
| hard-coded three iterations and diminishing-return percentage | reject as a correctness criterion; use deterministic convergence and hard budgets |
| whole-module mutable IR passed directly among functions | reject; transactions, revisions and stage tokens are mandatory |
| text assembly peephole passes | reject from BIR; target output work is later |
| combined stack layout/regalloc/codegen state | split: shared BIR allocation owns abstract assignments and ordinary capacity spill/reload in a new revision; target MIR/backend owns concrete registers, frame layout, selection, and encoding |
| target-specific div-by-constant gating inside the semantic pass list | do not copy into canonicalization; target-dependent expansion requires a later target stage or a proven target-independent semantic transform |
| inline asm symbol resolution after inlining | retain the need for explicit symbol dependencies, but Raw import must already provide structured identities; no text reparsing in this pipeline |

The reference is not authority for c4c stage ownership. In particular, its
backend consumes an SSA IR directly into stack layout/regalloc/instruction
selection; that does not justify placing those activities in `src/backend/bir`.

## 17. Coverage obligations before implementation

The canonical pipeline is coverage-complete only if reviews establish all of
the following:

1. every Raw-only form listed by core/import/verify has exactly one owning pass
   and a fail-closed unsupported outcome;
2. every instruction/terminator descriptor declares which passes may mutate it
   and which analyses observe it;
3. every legacy route/prealloc capability is mapped either to a canonical pass,
   a revision-bound analysis, typed preparation, MIR/backend, or explicit
   rejection;
4. every pass document repeats the exact adjacent pre/postconditions above and
   lists its preserved/invalidated analyses;
5. aggregate and intrinsic reviews prove they cannot recreate earlier
   noncanonical forms under the retained order;
6. cross-function declaration/definition, call-effect, global initializer,
   top-level asm and intrinsic-registry consistency has one module barrier;
7. every internal gate has cumulative framework-property checks plus the
   configured verifier operation, and the full Canonical profile alone mints
   an unforgeable publication token;
8. preparation accepts only `VerifiedPreparationInput` and never mutates
   `CanonicalBir`;
9. deterministic single-worker and parallel schedules produce identical
   semantic hashes and ordered diagnostics;
10. failure injection at every analysis, pass, merge and verifier boundary
    proves rollback and last-good behavior.

## 18. Source gaps and open design choices

These are unresolved and must remain visible rather than being guessed by the
implementation:

- Section 3.1 is the frozen first-owner inventory for the Raw forms currently
  admitted by importer Section 11, but the implementation mechanism that
  proves every closed descriptor is represented in that inventory (generated
  table or exhaustive dispatch) remains to be selected;
- each pass still needs an exhaustive postcondition checker and stable
  pass-contract diagnostic codes; these must not be presented as new verifier
  profiles;
- the chosen canonical SSA representation (phi versus block arguments) must be
  frozen before `cfg`/`ssa` APIs are implemented;
- parallel CFG edges need one final `EdgeKey` encoding shared by core, CFG,
  SSA, diagnostics and audit;
- aggregate canonicalization must prove whether layout-independent paths alone
  are sufficient for every array/struct/union/complex and by-value legacy case;
- intrinsic namespace/versioning, unknown-intrinsic policy, asm-goto outputs,
  exception/unwind edges and runtime-helper eligibility remain open;
- module effect-summary reconciliation in `memory` may be analysis-only; if so,
  its scope can be narrowed in a new reviewed plan version;
- the core/pass framework must define the opaque owning candidate/checkpoint,
  private stage fork, independent committed invocations on a fork, serial
  occurrence discard, atomic verified fork promotion and atomic function-wave
  merge primitives before this runner or any fixed-point group can be
  implemented; missing primitives fail plan validation before input
  consumption, and serialization remains intentionally unspecified beyond
  process-local ownership;
- deterministic ID reservation under parallel insertion needs one selected
  algorithm and stress proof;
- cancellation semantics must decide whether a completed pass barrier is
  returned as last-good without ever treating it as success;
- `VerifiedPreparationInput` is fixed as a short-lived borrow of immutable
  `CanonicalBir` storage; the concrete C++ lifetime encoding remains an
  implementation choice but may not copy the graph or outlive the owning
  `CanonicalBir` capability;
- no optimization family such as DCE/GVN/LICM is currently part of this
  canonicalization plan. Adding one requires explicit source intent, ordered
  entries, fixed-point policy, and separate semantic coverage review.

Until those choices are resolved, implementations must fail closed at the
relevant boundary and must not use legacy text, route tables, target state or
prepared side data to synthesize missing semantic authority.

## 19. Design checks

Reviewers should reject a change to this contract if any answer is “yes”:

- Can a pass run on an unverified `RawBir` or repair malformed import state?
- Can configuration change mandatory order?
- Can a later pass emit an earlier pass's noncanonical form?
- Can worker completion order affect IDs, diagnostics or output?
- Can a failed wave partially commit?
- Can a preservation claim keep an analysis across the wrong revision axes or
  ordered function-revision digest?
- Can a last-good snapshot be mistaken for `CanonicalBir`?
- Can a canonical pass choose ABI locations, registers, stack/frame offsets,
  call moves, target opcodes or helper symbols?
- Can preparation write facts back into canonical storage?
- Can MIR appear under `src/backend/bir`?

Expected implementation proof includes plan-validation unit tests, exhaustive
pass dispatch, verifier fault injection, transaction rollback tests,
single/parallel reproducibility tests, fixed-point budget tests, stale-analysis
tests, stage-token construction tests, and legacy/reference coverage audits.

## 20. Research anchors inspected

- `src/backend/bir/core/README.md`
- `src/backend/bir/lir_to_bir/README.md`
- `src/backend/bir/verify/README.md`
- `src/backend/legacy/bir_route*.cpp` and route/view headers
- `src/backend/legacy/prealloc/prealloc.cpp`
- `src/backend/legacy/prealloc/legalize.cpp`
- `src/backend/legacy/prealloc/liveness.cpp`
- `src/backend/legacy/prealloc/out_of_ssa.cpp`
- `src/backend/legacy/prealloc/regalloc.cpp`
- `src/backend/legacy/prealloc/README.md` and its published-plan headers
- `ref/claudes-c-compiler/src/passes/mod.rs` and pass documentation
- `ref/claudes-c-compiler/src/backend/README.md`
- `ref/claudes-c-compiler/src/backend/generation.rs`
- `ref/claudes-c-compiler/src/backend/liveness.rs`
- `ref/claudes-c-compiler/src/backend/regalloc.rs`
- `ref/claudes-c-compiler/src/backend/stack_layout/`
