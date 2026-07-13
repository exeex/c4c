# Canonical BIR Pass Framework Contract

Status: design contract; no implementation is claimed.

This document defines how a BIR pass is identified, scheduled, given authority,
committed, verified, invalidated, observed, and failed. It deliberately does not
define the semantic rewrite performed by any one pass. The documents below this
directory own those local transformations. The root
[`BIR README`](../README.md) owns their normative `B1`-`B7` order;
[`pipeline/README.md`](../pipeline/README.md) expands orchestration within that
fixed interval.

The central rule is:

> A pass invocation may inspect one revision in a private occurrence fork,
> perform one capability-scoped transaction, and either commit one verified
> revision into that fork or leave it unchanged. Only the occurrence barrier may
> atomically publish the completed fork as the next pipeline checkpoint.

A pass never owns raw storage, never publishes an unchecked snapshot, and never
communicates by leaving mutable side tables attached to BIR.

## 1. Scope and ownership

### 1.1 This framework owns

- closed pass identity and kind registries;
- the type-erased function-pass and module-pass invocation contracts;
- pass registration and static contract validation;
- transaction creation, commit, rollback, and revision accounting;
- revision-bound analysis access and exact invalidation enforcement;
- pass preconditions, postconditions, preservation declarations, and failure;
- verifier placement around edits and stage publication;
- module barriers and deterministic per-function parallel scheduling;
- instrumentation, statistics, cancellation, and resource budgets;
- convergence enforcement for idempotent and fixed-point pass groups;
- transfer of an unforgeable stage token at a pipeline publication boundary.

### 1.2 This framework does not own

- pass order, repetitions, or pipeline phase composition; those belong to the
  root [`BIR README`](../README.md), with local orchestration detail in
  [`pipeline/README.md`](../pipeline/README.md);
- the semantic rules of legalize, scalar, CFG, SSA, memory, aggregate, or
  intrinsic rewriting; those belong to their pass documents;
- BIR entity layout, stable IDs, def-use maintenance, revisions, views, or
  editor primitives; those belong to [`core/README.md`](../core/README.md);
- CFG, dominance, liveness, provenance, publication, comparison, call-graph,
  or memory-effect computation; those belong to
  [`analysis/README.md`](../analysis/README.md);
- verifier rule semantics; those belong to
  [`verify/README.md`](../verify/README.md);
- ABI classification, call placement, frame layout, register allocation,
  spills, target opcodes, emission, or rendering. Root stages `C2`-`F3` use
  their own reviewed target-aware preparation, pseudo-pass, out-of-SSA,
  allocation, spill/reload, publication, and MIR contracts; they do not gain
  authority by registering as canonical `P01`-`P07` passes here.

This is the target-independent canonical pass framework for root stages
`B1`-`B8`. A later target-aware phase may reuse implementation-neutral
utilities only when its own contract defines the capability, revision,
transaction, verifier, and publication boundary. It may not receive this
framework's `PassProperty`, `CanonicalBir` publication authority, analysis
cache, or canonical occurrence registry by implication.

The target-aware pass contracts indexed below are separate owners rather than
canonical `P01`-`P07` registrations: [D1 generic pseudo lowering](pseudo_lowering/README.md),
[D2 shared ABI-aware call lowering](call_lowering/README.md),
[D4 target pseudo legalization](target/README.md), and
[D5 out-of-SSA](out_of_ssa/README.md).

### 1.3 Allowed dependency direction

Pass framework code and individual canonical passes may depend only on:

```text
bir/core        stable IDs, immutable views, editor capabilities, revisions
bir/analysis    registered revision-bound analyses through AnalysisManager
bir/verify      profile/rule API through the framework verifier gate
bir/diagnostics structured diagnostics and deterministic presentation keys
```

They must not include, link against, query, or recover facts from:

```text
backend/legacy/**
legacy compatibility views, Route1-Route8 records, or spelling adapters
bir/preparation/** or any PreparedBir/plan structure
MIR, target backends, target instruction descriptions, or emitters
ABI placement, frame, stack-slot, register-allocation, spill, or move state
printer/renderer output, debug dumps, textual instruction spelling, or notes
```

Raw and Canonical BIR may preserve source-semantic typed sizes, alignments, and
address spaces owned by their accepted rows. They contain no semantic
`target_profile`, rendered `data_layout`, target triple, pointer-width/address-
space layout selection, or other C1/C2 target context. C1 independently selects
the exact `TargetProfile`, and C2 derives target layout. A canonical pass cannot
inspect or reconstruct those facts, calling-convention placement policy, legal
machine opcode sets, or backend feature switches.

## 2. Closed identities and static descriptors

The initial registry is closed so serialized diagnostics, tests, statistics,
and pipeline contracts cannot depend on free-form names.

```cpp
enum class PassId : std::uint16_t {
  Legalize = 0x0101,
  ScalarCanonicalize = 0x0102,
  CfgCanonicalize = 0x0103,
  SsaCanonicalize = 0x0104,
  MemoryCanonicalize = 0x0105,
  AggregateCanonicalize = 0x0106,
  IntrinsicCanonicalize = 0x0107,
};

enum class PassKind : std::uint8_t {
  Function,
  Module,
};

enum class PassProperty : std::uint16_t {
  RawVerified,
  TypesLegal,
  ScalarsCanonical,
  CfgCanonical,
  SsaCanonical,
  MemoryCanonical,
  AggregatesCanonical,
  IntrinsicsCanonical,
  CanonicalVerified,
};

enum class RepeatContract : std::uint8_t {
  Once,
  Idempotent,
  FixedPointMember,
};

struct PassDescriptor {
  PassId id;
  PassKind kind;
  std::string_view stable_name; // presentation only; identity is PassId
  PropertySet requires;
  PropertySet establishes;
  PropertySet preserves;
  AnalysisIdSet required_analyses;
  AnalysisIdSet optional_analyses;
  PassIdSet opportunity_dependencies;
  RepeatContract repeat;
  MutationEffectSet permitted_effects;
  bool accepts_declarations;
};
```

`PassId`, `PassKind`, `PassProperty`, mutation effects, and analysis IDs are
closed enums or closed generated registries. Unknown numeric IDs fail registry
loading. New passes receive unused IDs; existing IDs are never renumbered or
reinterpreted. A stable name is useful in logs but cannot select a pass or
grant authority.

These declarations are the single source of truth for pass identity and
implementation kind. The pipeline may refer to `PassId` and may assign an
execution scope to an occurrence, but it must not redeclare either enum or give
the same ID another numeric value/name. `PassKind` describes the implementation
interface (`FunctionPass` or `ModulePass`); a pipeline operation such as
"parallel function proposals followed by one module-wide publication" is an
execution scope, not a third pass implementation kind. Such an occurrence must
still identify the concrete function and/or module implementation(s) it invokes
and give each invocation exactly one transaction.

`requires`, `establishes`, and `preserves` describe semantic forms, not order.
The pipeline must supply a sequence whose accumulated properties satisfy each
descriptor. `opportunity_dependencies` means that another pass can create new
work for this pass; it is used only when validating a fixed-point schedule or a
safe skip decision. It cannot insert, reorder, or invoke another pass.

Registration rejects:

- duplicate IDs or names;
- a descriptor whose kind disagrees with its implementation;
- unknown properties or analyses;
- required analyses unavailable for the pass kind;
- impossible precondition/postcondition combinations;
- a mutation effect not representable by core `MutationSummary`;
- an `Idempotent` pass without its required second-run proof policy;
- a `FixedPointMember` that is not assigned a bounded group by the pipeline;
- any dependency cycle interpreted as order rather than an explicitly bounded
  fixed-point group.

## 3. Pass interface and invocation capabilities

Pass objects are stateless after registration. Invocation-local state belongs
to the supplied session. A pass object may contain immutable configuration
validated at pipeline construction, but it may not retain module views, entity
references, analysis results, editors, contexts, or diagnostics between calls.

```cpp
class FunctionPassSession;
class ModulePassSession;
struct PassResult;
struct PassFailure;

class FunctionPass {
public:
  virtual ~FunctionPass() = default;
  virtual PassId id() const noexcept = 0;
  virtual Result<PassResult, PassFailure>
  run(FunctionPassSession&) const = 0;
};

class ModulePass {
public:
  virtual ~ModulePass() = default;
  virtual PassId id() const noexcept = 0;
  virtual Result<PassResult, PassFailure>
  run(ModulePassSession&) const = 0;
};

using AnyPass = std::variant<
    std::unique_ptr<const FunctionPass>,
    std::unique_ptr<const ModulePass>>;
```

The concrete implementation may use function pointers plus opaque immutable
configuration instead of virtual dispatch, but it must preserve the same
authority and lifetime boundary.

### 3.1 Context and read authority

```cpp
enum class VerificationPolicy : std::uint8_t;

struct ResourceBudget {
  std::uint64_t max_work_units;
  std::uint64_t max_new_entities;
  std::uint64_t max_diagnostics;
  std::chrono::steady_clock::duration telemetry_time_limit;
};

class CancellationToken {
public:
  bool is_cancelled() const noexcept;
  BirResult<void> checkpoint(std::uint64_t work_units = 1) const;
};

class PassContext {
public:
  PassId pass_id() const noexcept;
  std::uint32_t pipeline_ordinal() const noexcept;
  std::uint32_t iteration() const noexcept;
  VerificationPolicy verification_policy() const noexcept;
  const ResourceBudget& budget() const noexcept;
  const CancellationToken& cancellation() const noexcept;
  DiagnosticSink& diagnostics() noexcept;
  StatisticSink& statistics() noexcept;
};
```

`PassContext` contains execution policy, not semantic backend authority. It has
no target object, ABI object, renderer, filesystem handle, environment-variable
lookup, random generator, wall-clock-dependent decision API, or mutable global
registry. Time may be measured by instrumentation but may not change a rewrite.
Deterministic resource failure is charged only in closed work units, entity
creation, diagnostics, recursion/worklist depth, or other host-independent
units. A wall-clock limit may request external cancellation and report
telemetry, but it cannot be used as proof of convergence and reproducibility
mode must not promise the same cancellation point across hosts.

### 3.2 Function session

```cpp
class FunctionEdit {
public:
  FunctionView view() const;
  BirResult<BuildResult> insert_before(InstId, InstSpec);
  BirResult<BuildResult> insert_after(InstId, InstSpec);
  BirResult<BuildResult> append(BlockId, InstSpec);
  BirResult<BuildResult> replace_inst(InstId, InstSpec, ResultMapping);
  BirResult<void> erase_inst(InstId);
  BirResult<void> move_before(InstId, InstId anchor);
  BirResult<void> move_to_end(InstId, BlockId);
  BirResult<void> set_operand(OperandSlot, Operand);
  BirResult<void> replace_all_uses_with(ValueId, Operand);
  BirResult<void> set_terminator(BlockId, TerminatorSpec);
  BirResult<BlockId> split_block(BlockId, InstId before);
  BirResult<void> redirect_edge(EdgeKey, BlockId, PhiTransferPlan);
  BirResult<void> erase_block(BlockId, BlockErasePlan);
  BirResult<LocalId> create_local(LocalSpec);
  BirResult<void> erase_local(LocalId);
};

class FunctionPassSession {
public:
  PassContext& context() noexcept;
  ModuleSymbolView module_symbols() const;
  FunctionView input() const;
  FunctionEdit& edit();
  FunctionAnalysisManager& analyses() noexcept;
};
```

`FunctionEdit` is a non-owning, invocation-scoped facade over the one underlying
`FunctionEditor`. It deliberately exposes neither `commit()` nor `rollback()`.
The executor owns those operations, which prevents a pass from publishing twice
or observing a partially committed revision. `input()` always identifies the
published pre-pass revision. Once `edit()` is first requested, candidate reads
must use `edit().view()`; the original input remains immutable.

A function pass cannot add, remove, or mutate module types, constants, symbols,
globals, function declarations, or other functions. It may refer to them through
`ModuleSymbolView` by stable ID.

### 3.3 Module session

```cpp
class ModuleEdit {
public:
  ModuleView view() const;
  BirResult<TypeId> intern_type(TypeSpec);
  BirResult<ConstantId> intern_constant(ConstantSpec);
  BirResult<GlobalId> add_global(GlobalSpec);
  BirResult<FunctionId> add_function(FunctionDeclSpec);
  BirResult<void> replace_global_initializer(GlobalId, GlobalInitializer);
  BirResult<void> change_symbol(SymbolId, SymbolEditSpec);
  BirResult<void> erase_global(GlobalId);
  BirResult<void> erase_function(FunctionId);
};

class ModulePassSession {
public:
  PassContext& context() noexcept;
  ModuleView input() const;
  ModuleEdit& edit();
  ModuleAnalysisManager& analyses() noexcept;
};
```

Like `FunctionEdit`, `ModuleEdit` cannot commit. A module pass runs only at an
exclusive module barrier. If a future module pass also edits function bodies,
core must first define a single `ModuleEditor` transaction that owns those body
edits and their revision effects; the framework must not emulate this with a
sequence of independently visible function commits.

## 4. Pass result and failure model

The pass reports intent; the executor reports the authoritative mutation and
publication result.

```cpp
enum class ChangeIntent : std::uint8_t {
  NoChange,
  Changed,
};

enum class PassFailureCode : std::uint16_t {
  UnsupportedInput,
  PreconditionFailed,
  EditRejected,
  AnalysisFailed,
  StaleAnalysis,
  ContractViolation,
  VerificationFailed,
  ResourceLimit,
  Cancelled,
  NonConvergent,
  InternalError,
};

struct PassFailure {
  PassFailureCode code;
  PassId pass;
  std::optional<FunctionId> function;
  std::optional<EntityId> entity;
  std::vector<PassDiagnostic> diagnostics;
  std::optional<BirError> core_error;
  std::optional<VerificationReport> verification;
};

struct PassResult {
  ChangeIntent intent;
  PreservedAnalyses preserved;
};

struct RevisionStamp {
  ModuleEpoch epoch;
  ModuleRevision module_revision;
  std::optional<FunctionId> function;
  std::optional<FunctionRevision> function_revision;
};

enum class PassStatisticId : std::uint16_t {
  InstructionsInserted,
  InstructionsErased,
  OperandsRewritten,
  BlocksInserted,
  BlocksErased,
  EdgesRedirected,
  AnalysesComputed,
  AnalysesReused,
  VerifierRulesRun,
  WorkUnits,
};

struct PassStatisticCounter {
  PassStatisticId id;
  std::uint64_t value;
};

struct PassInvocationStatistics {
  std::uint32_t pipeline_ordinal;
  std::uint32_t iteration;
  std::optional<FunctionId> function;
  std::vector<PassStatisticCounter> counters; // sorted by PassStatisticId
};

struct CommittedPassResult {
  PassId pass;
  ChangeIntent change;
  std::optional<MutationSummary> mutation;
  RevisionStamp before;
  RevisionStamp after;
  PreservedAnalyses effective_preservation;
  PassInvocationStatistics statistics;
  std::vector<PassDiagnostic> diagnostics;
};
```

`PassResult::Changed` is not proof that storage changed, and a pass cannot
construct `MutationSummary`. At commit, core derives the exact summary from the
candidate transaction. The executor enforces:

- `NoChange` plus an empty actual summary: rollback the unused transaction (if
  one was opened), retain the same fork revision, and report `NoChange`;
- `Changed` plus a non-empty summary: verify, atomically commit, increment the
  appropriate private-fork revision exactly once, and report `Changed`;
- `Changed` plus an empty summary: a contract violation, because counters and
  fixed-point termination must not be driven by a claim;
- `NoChange` plus a non-empty summary: a contract violation and rollback;
- thrown exceptions, uncaught allocation failures, cancellation, budget
  exhaustion, analysis failure, or diagnostics containing a fatal error:
  rollback and commit no invocation revision.

Expected semantic failures use `Result<PassResult, PassFailure>` at the runner
boundary. A free-form string, `false`, null, assertion, or process exit is not a
valid recoverable failure protocol. Debug assertions may guard impossible
implementation states, but CI verification must still diagnose malformed
candidate state without publishing it.

## 5. One transaction per pass invocation

For one function or module invocation, the executor performs exactly this state
machine:

```text
invocation input revision inside the private occurrence fork
  -> check descriptor and preconditions
  -> open at most one editor transaction
  -> invoke pass with scoped facade
  -> compare intent with derived mutation
  -> check permitted effects and preservation
  -> verify candidate according to policy
  -> commit exactly once OR rollback exactly once
  -> invalidate/rebind analyses
  -> emit deterministic result and instrumentation events
```

No nested pass execution is allowed while the transaction is open. A pass may
call helper algorithms, but cannot ask the manager to run another registered
pass. It cannot commit an intermediate rewrite to make a later rewrite easier.
If an algorithm needs internal checkpoints, those are rollback-only child
checkpoints inside one unpublished candidate, not visible BIR revisions.

All editor operations maintain core's cached exact def-use relation. Structural
IR mutation through pointers, `const_cast`, raw containers, storage handles,
friend access, analysis objects, or instrumentation is forbidden. A helper that
needs mutation must receive the same `FunctionEdit`/`ModuleEdit` capability.

A successful function commit increments that `FunctionRevision` once, no matter
how many instructions changed; it does not manufacture a `ModuleRevision`
change for a body-only edit. A successful module commit increments
`ModuleRevision` once and increments affected function revisions only according
to the core contract. `RevisionStamp` must therefore carry both revision axes
instead of flattening them into one counter. Attachment-only mutation remains a
real classified summary; whether semantic analyses survive it is decided by
analysis traits.

Rollback in this section means rollback of the currently open, unpublished
editor candidate. It never means decrementing a revision, resurrecting erased
storage, or undoing an already committed pass. A pipeline that offers a
`last_good` failure result must retain an immutable owned snapshot before the
attempt (or keep the entire attempt unpublished) using a core-supported
ownership API. A raw storage pointer, a borrowed `ModuleView`, or a claim that a
committed revision can later be discarded is not a last-good snapshot.

## 6. Analyses, preservation, and exact invalidation

Analyses are immutable products of a published revision. The manager is the
only way a pass obtains one.

```cpp
enum class AnalysisId : std::uint16_t {
  Cfg,
  Dominance,
  Liveness,
  MemoryEffects,
  Provenance,
  CallGraph,
  Comparison,
  Publication,
};

struct AnalysisStamp {
  ModuleEpoch epoch;
  ModuleRevision module_revision;
  std::optional<FunctionId> function;
  std::optional<FunctionRevision> function_revision;
  FunctionRevisionDigest observed_functions;
};

class PreservedAnalyses {
public:
  static PreservedAnalyses none();
  static PreservedAnalyses all();
  void preserve(AnalysisId);
  bool contains(AnalysisId) const;
};

template <AnalysisResult R>
BirResult<AnalysisHandle<R>>
FunctionAnalysisManager::get(FunctionView);
```

An `AnalysisHandle` checks its stamp whenever dereferenced. A function analysis
must match `{ModuleEpoch, FunctionId, FunctionRevision}` plus every module fact
its traits declare. A module analysis that reads function bodies, such as a call
graph, must match the ordered function-revision digest. A mismatch returns
`StaleAnalysis`; it never silently recomputes through the old handle and never
allows a pass to opt out of checking.

Each registered analysis supplies closed traits:

```cpp
struct AnalysisTraits {
  AnalysisId id;
  PassKind scope;
  MutationEffectSet invalidated_by;
  AnalysisIdSet dependencies;
  bool can_rebind_after(const MutationSummary&) const;
  BirResult<void> validate_preservation(
      const AnalysisBase&, ModuleView before,
      CandidateModuleView after, const MutationSummary&) const;
};
```

After a successful candidate verification and before making cached results
observable at the new revision, the executor computes effective preservation:

1. Start from the pass declaration in `PassResult` and require it to be a subset
   of the descriptor's statically allowed preservation set.
2. Reject preservation whose dependencies are not also
   preserved/revalidated. A pass need not request an analysis merely to state
   that its mutation class preserves it; cached instances are still checked by
   manager-owned traits and never handed to the pass for validation.
3. Compare the actual `MutationSummary` with `invalidated_by`.
4. If effects overlap, reject the claim unless the analysis implements and
   passes exact `validate_preservation` for this mutation.
5. In debug/CI preservation-audit mode, independently recompute the result and
   compare it with the preserved result using the analysis's semantic equality.
6. Invalidate all other cached results before the new revision is exposed.

An analysis object stamped with the old revision is always stale. “Preserved”
means the manager may install a checked, manager-owned rebinding of the immutable
result under the new exact stamp; it does not mean the old handle becomes valid.
If an analysis cannot prove safe rebinding, it is recomputed lazily.

`PreservedAnalyses::all()` is accepted only for a truly empty summary or when
every live cached analysis independently validates preservation. Declaring CFG
preserved because no blocks were added is insufficient if terminators, edge
slots, asm-goto targets, indirect target sets, or phi edge keys changed.

Analyses are read-only and cannot be used as mutable communication between
passes. Dense indices belong to one result and must never become BIR identity.

## 7. Preconditions and postconditions

The executor checks descriptor properties before invoking a pass. A missing
property is `PreconditionFailed`, not a request to run the pass that normally
establishes it. Order repair belongs to pipeline construction.

Each individual pass document must enumerate:

- accepted input stage and exact raw/canonical forms;
- required `PassProperty` values and analyses;
- mutation effects it may produce;
- semantic postconditions and the verifier rules that enforce them;
- properties preserved and established;
- whether it runs on declarations and unreachable blocks;
- idempotence or fixed-point behavior;
- its immediate predecessor/successor assumptions without claiming order;
- mapped legacy capability and deliberately rejected legacy behavior.

A property is not established merely because the pass returns success. It is
published only after the pass-specific postcondition checker and the configured
verifier gate succeed. Postcondition checkers are read-only and deterministic.

## 8. Verification and publication

The framework uses the verifier API; it does not duplicate verifier rules.

### 8.1 Verification policy

```cpp
enum class VerificationPolicy : std::uint8_t {
  PublicationOnly,
  AfterMutatingPass,
  AfterEveryPass,
  PreservationAudit,
};

struct PassVerifyOptions {
  VerificationPolicy policy;
  VerifyOptions verifier;
  bool run_pass_postconditions = true;
  bool rerun_idempotent_pass = false;
};
```

- Production/release may use `PublicationOnly` plus incremental
  `verify_after_edit` on every commit, but it may not disable the mandatory full
  verification that creates a stage token.
- Debug and normal tests use `AfterEveryPass`; the framework verifies the
  published input before invocation and the candidate after mutation.
- CI/reviewer proof uses `PreservationAudit` and may rerun idempotent passes.
- Fuzzing uses full before/after verification with bounded diagnostics and
  resource limits.

Even in the least expensive policy, editor commit invokes stage-appropriate
`verify_after_edit` over the actual mutation closure. Incremental verification
is an optimization, not a weaker rule set. If the verifier cannot prove the
closure complete, it requests full verification.

The framework never invents a parallel verifier-rule registry. Every verifier
failure retains the exact closed `VerifyRule` value and `VerificationReport`
owned by [`verify/README.md`](../verify/README.md); `VerificationRejected` is
only the pass-layer category. Pass-specific postcondition diagnostics use a
separate closed pass-contract code space and must not impersonate a
`VerifyRule`. Verification profile names are likewise those from the verifier
contract (`Raw`, `Canonical`, and `PreparedInput`); intermediate
`PassProperty` values are pipeline proofs, not new public verifier profiles.

### 8.2 Stage publication token

Intermediate invocation commits update only the framework-owned private
occurrence fork; after every invocation succeeds, one occurrence barrier may
atomically publish that fork as the next committed pipeline checkpoint. Neither
kind of commit manufactures `CanonicalBir`. Only the pipeline's explicit
publication gate may consume the final verified stage object and produce the
next token:

```cpp
class CanonicalBir; // move-only, verifier-gated stage proof

Result<CanonicalBir, PublicationFailure>
verify_and_publish_canonical(PassStage&& completed,
                             ExpectedPipelineFingerprint,
                             VerifyOptions = {});
```

Publication requires all pipeline-declared properties, the expected ordered
pass fingerprint, no active editor or borrowed candidate capability, and a full
`VerifyProfile::Canonical` report. A pass cannot construct, copy, downcast to,
or return `CanonicalBir`. A green `verify_after_edit` report cannot be traded
for a stage token.

The exact internal type carrying verified Raw semantics between the consumed
`RawBir` token and final `CanonicalBir` remains a core/pipeline API design gap;
it must not be represented by a public enum field that a pass can set.

## 9. Module barriers and parallel execution

One function-pass occurrence runs against a framework-owned private fork of the
committed pre-occurrence checkpoint:

1. Before consuming pipeline input, require the private-fork and atomic
   occurrence-publication capabilities described below.
2. Fork the committed checkpoint, then freeze its module-owned type, constant,
   symbol, global, and function tables.
3. Snapshot module epoch/revision and functions in stable module order; skip
   declarations only if the descriptor says so.
4. Give each function invocation exactly one exclusive editor transaction and
   one function analysis manager. A successful invocation commits only into the
   private occurrence fork, never into the externally committed stage.
5. After every invocation succeeds, discard all borrowed views/ranges, validate
   the complete occurrence, and atomically publish the fork once.
6. If any invocation fails, roll back its still-open transaction, discard the
   entire private fork, and retain the exact pre-occurrence checkpoint.

There is never more than one editor for a function. In serial mode, invocations
run in canonical `FunctionId` order inside the same private fork. Each still has
one transaction and may commit its verified candidate to that fork, but no such
commit is an occurrence publication. If a later invocation fails, the runner
discards the fork; it does not roll back an already committed revision. Any
analysis needing whole-module function bodies uses the pre-occurrence snapshot
and is invalidated as a module-wide unit if any observed function changes.

Serial function-scope execution has its own hard prerequisite: core/framework
must provide an owning private occurrence fork plus one atomic occurrence
publication operation. If either capability is unavailable, runner/plan
initialization fails before consuming `RawBir`; serial execution must not fall
back to publishing each function directly to the committed stage.

Parallel execution has an additional, separate prerequisite: core/framework
must provide a checked multi-function proposal/merge primitive with
deterministic ID allocation. Workers prepare disjoint proposals from the same
frozen snapshot without committing to shared fork storage; the runner merges
them in stable `FunctionId` order and then uses the same atomic occurrence
publication operation. Until this additional primitive exists, a capable runner
may use the serial private-fork path. A function pass cannot observe worker
index or completion order.

Neither path may emulate occurrence atomicity by publishing functions
independently and later claiming that committed revisions were rolled back or
hidden.

A module pass is a stop-the-world barrier: all workers have joined, all function
editors and analysis borrows are destroyed, instrumentation callbacks are idle,
and the module editor is exclusive. After its commit, a new frozen phase and
new analysis managers are created.

### 9.1 Determinism

- Scheduling order may vary; semantic output and observable reports may not.
- Entity iteration uses core stable order, never hash-container order.
- Diagnostics sort by pipeline ordinal, module function ordinal, block ordinal,
  instruction ordinal, operand/result position, rule/code, then insertion
  sequence local to that entity.
- Statistics merge by closed statistic ID, pass ID, and function order.
- Concurrent failure is deterministic: all already-running invocations reach a
  cancellation checkpoint and roll back; the selected primary failure is the
  lowest stable diagnostic key, with later failures attached as notes.
- No pass may use worker index, pointer address, completion order, wall time, or
  an unordered traversal to choose a rewrite.

## 10. Repetition, idempotence, and convergence

Pass order and repetition are pipeline decisions, but execution semantics are
fixed here.

### 10.1 Once

A `Once` pass is invoked at most once per pipeline occurrence. If the pipeline
lists the same `PassId` twice, those are two explicit occurrences with distinct
ordinals, not an implicit loop.

### 10.2 Idempotent

An `Idempotent` pass promises that rerunning it immediately on its successful
output returns `NoChange`, emits no new semantic diagnostics, and preserves the
same canonical semantic fingerprint. Debug/CI may enforce this with a second
transaction that must roll back empty. Failure is `ContractViolation`.

### 10.3 Fixed-point groups

A `FixedPointMember` runs only inside a pipeline-declared group with:

- explicit member order;
- maximum iteration count;
- maximum total work/new-entity budget;
- a progress fingerprint or monotone measure;
- opportunity dependencies used only for proven safe skips;
- a mandatory final no-change iteration, unless a monotone terminal predicate
  proves convergence;
- `NonConvergent` failure on oscillation, repeated fingerprint, or budget end.

Raw change counts and a “less than N percent” timing heuristic are not semantic
proof of convergence. They may be optional optimization policy only if stopping
early still satisfies all declared postconditions. A pass cannot report fake
changes to keep a loop alive.

## 11. Registration and pipeline validation

```cpp
class PassRegistry {
public:
  BirResult<void> register_function(
      PassDescriptor, std::unique_ptr<const FunctionPass>);
  BirResult<void> register_module(
      PassDescriptor, std::unique_ptr<const ModulePass>);
  BirResult<PassDescriptor> descriptor(PassId) const;
  BirResult<const AnyPass&> implementation(PassId) const;
  void freeze();
};

struct PipelineOccurrence {
  PassId id;
  std::uint32_t ordinal;
  std::optional<FixedPointGroupId> group;
  ResourceBudget budget;
};
```

The registry is assembled and frozen before any module is accepted. Runtime
plugin loading and environment-selected replacement implementations are outside
this contract. A pipeline occurrence references a registered ID; it does not
carry an arbitrary callback.

Pipeline validation checks property flow, kind/barrier placement, fixed-point
contracts, required verifier profiles, analysis availability, and publication
fingerprint. This framework validates the supplied sequence but never invents
one. The current documented order remains solely in the root
[`BIR README`](../README.md); the pipeline document transcribes the canonical
interval into a checked descriptor without becoming an independent order
source.

Disabling a pass is legal only when the resulting pipeline still establishes
every property required at publication. A debug option cannot bypass a
correctness pass and still label the output `CanonicalBir`.

## 12. Diagnostics, instrumentation, and statistics

### 12.1 Structured diagnostics

```cpp
enum class PassDiagnosticCode : std::uint16_t {
  MissingProperty,
  UnsupportedForm,
  EditRejected,
  UndeclaredMutation,
  ChangeIntentMismatch,
  PreservationRejected,
  StaleAnalysis,
  VerificationRejected,
  Cancelled,
  ResourceLimit,
  NonConvergent,
  InternalFailure,
};

struct PassDiagnostic {
  PassDiagnosticCode code;
  DiagnosticSeverity severity;
  PassId pass;
  std::optional<FunctionId> function;
  std::optional<FunctionEntityId> entity;
  std::optional<OriginId> origin;
  std::optional<VerifyRule> verifier_rule; // only for an attached verifier report
  std::string detail; // presentation only
};
```

Codes and entity IDs are authoritative; `detail` is not parsed by another
pass. Source origins may improve presentation but cannot substitute for stable
BIR identity. Diagnostics are buffered per invocation and become observable
only through the deterministic merge. Rolled-back attempts may report why they
failed, but cannot leave semantic notes attached to IR.

### 12.2 Instrumentation

```cpp
class PassInstrumentation {
public:
  virtual void pipeline_started(PipelineInfo) = 0;
  virtual void pass_started(PassRunInfo) = 0;
  virtual void analysis_requested(AnalysisEvent) = 0;
  virtual void pass_finished(const CommittedPassResult&) = 0;
  virtual void pass_failed(const PassFailure&) = 0;
  virtual void pipeline_finished(PipelineResultInfo) = 0;
};
```

Instrumentation receives immutable value snapshots after the relevant state
transition. It cannot obtain editors, mutable analysis caches, storage pointers,
or invoke a pass recursively. Callback failure is isolated as instrumentation
failure; it cannot turn an invalid candidate valid. Timing and tracing must not
affect scheduling decisions that change semantics.

### 12.3 Statistics

`PassInvocationStatistics` is the framework-owned record for exactly one
function or module invocation. A pipeline may deterministically aggregate or
convert these records into pipeline-level summaries, but the framework neither
depends on nor returns a pipeline-owned `PassStatistics` type.

Statistics use the closed `PassStatisticId` values
`InstructionsInserted`,
`InstructionsErased`, `OperandsRewritten`, `BlocksInserted`, `BlocksErased`,
`EdgesRedirected`, `AnalysesComputed`, `AnalysesReused`, `VerifierRulesRun`, and
`WorkUnits`. Core-derived mutation counts are authoritative; a pass may add
algorithm-specific counters but cannot override them. Counters are unsigned,
overflow-checked, and merged deterministically. They are observation, not a
convergence proof unless a pass contract defines a checked monotone measure.

## 13. Cancellation and resource limits

Passes must call `checkpoint()` in potentially unbounded graph walks,
worklists, recursion replacement loops, and fixed-point iterations. Editor
operations also charge new entities and mutation work against the invocation
budget. Cancellation or exhaustion:

- stops additional work at the next checkpoint;
- prevents commit even if the candidate happens to verify;
- rolls back the entire invocation;
- invalidates candidate-only handles;
- returns structured `Cancelled` or `ResourceLimit` failure;
- does not change published revisions or analysis caches.

Cancellation does not expose partial occurrence progress as a committed
pipeline stage. Serial mode rolls back the current open invocation transaction
and discards the private occurrence fork. Parallel mode cancels and rolls back
every still-unpublished proposal and likewise discards the fork. Revisions
already committed inside a discarded private fork are not rolled back; their
owning fork simply never becomes the next committed checkpoint. Revisions
published to a committed checkpoint can never be called proposals, discarded,
or rolled back. Publishing a partially transformed occurrence is never allowed.

## 14. Legacy behavior disposition

Legacy behavior is evidence of required compiler capability, not an API to
preserve.

| Legacy mechanism | New disposition | Pass-framework rule |
|---|---|---|
| `Route1ProducerIndex` | def-use/core traversal plus a revision-bound producer analysis if a derived classification remains necessary | never a mutable pass side table; stable `ValueId`/`InstId` replace spelling identity |
| `Route2SelectChainValueIndex` | comparison/select dependency analysis | revision-stamped analysis; pass requests it by `AnalysisId` |
| `Route3MemoryAccessIndex` | core memory schema plus provenance/memory-effects analysis | no target storage or selected address mode in canonical passes |
| `Route4PublicationAvailabilityIndex` | dominance, CFG, SSA availability, and verifier rules | availability is checked/recomputed; it is not publication authority |
| `Route5EdgeJoinSourceIndex` | edge-keyed phi/SSA analysis or SSA rewrite | parallel edges use exact `EdgeKey`; no predecessor-name matching |
| `Route6CallUseSourceIndex` | typed call operands and def-use; any semantic query is analysis | ABI registers, call moves, and home selection are rejected |
| `Route7ComparisonConditionIndex` | comparison analysis and canonical comparison pass behavior | branch-condition materialization remains semantic; target condition codes do not enter BIR |
| `Route8ReturnChainIndex` | exact def-use/return-use analysis and later semantic canonicalization if needed | no return register or move authority |
| route facades and reference validators | `AnalysisManager`, stable IDs, and verifier rules | no facade may make stale records look current |
| `query.cpp` same-block scans | focused revision-bound analyses or direct immutable traversal | never query renderer text or raw names |
| `bir_validate.cpp::validate` | structured Raw/Canonical verifier profiles | boolean/free-form failure is replaced by stable rule diagnostics |
| `BirPreAlloc::completed_phases` strings | pipeline properties and stage tokens | a string cannot prove a pass ran or a stage is valid |
| `BirPreAlloc::run()` sequencing | split across canonical pipeline, preparation, and MIR pipeline owners | canonical pass manager does not absorb stack/liveness/out-of-SSA/regalloc order |
| legacy `legalize_module` direct aggregate mutation | narrowly specified canonical editor passes where target-independent; otherwise preparation/MIR | mutation must be transactional and effect-declared |
| prepared liveness dense values | a revision-bound analysis only if needed before MIR | dense indices never become BIR identity or durable authority |
| out-of-SSA, regalloc, frame, call/storage plans, carriers | preparation or MIR contracts outside this directory | categorically forbidden to canonical BIR passes |
| legacy notes and prepared printer | diagnostics/instrumentation and a read-only renderer | text cannot feed a later pass or repair missing semantics |

The legacy route chain often rebuilds indexes per block/function and validates
references through facade adapters. Its useful behavior is complete semantic
coverage and explicit failure categories. Its rejected behavior is duplicated
identity, name-based fallback, manually coordinated freshness, prepared facts
mixed with semantic facts, and mutable publication records that downstream code
treats as authority.

## 15. Reference compiler comparison

Evidence from `ref/claudes-c-compiler/src/passes` is adopted selectively.

### Strengths to retain

- a visible ordered pipeline rather than pass-local self-scheduling;
- explicit function-pass versus interprocedural/module behavior;
- per-function dirty tracking to avoid revisiting proven-converged functions;
- shared CFG/dominator/loop computation across passes whose mutations preserve
  those facts;
- explicit opportunity dependencies for safe skip decisions;
- bounded loops and early convergence checks;
- per-pass timing and change statistics;
- cleanup passes repeated after transformations that create their work.

### Weaknesses this contract closes

- passes mutate `IrModule`/function vectors directly rather than through an
  exclusive editor transaction;
- change counts are pass assertions rather than core-derived summaries;
- CFG preservation is justified in comments, not checked against mutations and
  exact revision stamps;
- shared analysis is passed as a borrowed ad hoc object instead of manager-owned
  revision-bound state;
- dirty/changed vectors use dense function positions as orchestration identity;
- environment strings disable passes without proving publication properties;
- ordering and dependency logic are embedded in one driver function and macro;
- diminishing-return heuristics can terminate work without a general semantic
  postcondition/convergence contract;
- no atomic rollback boundary surrounds one pass or a parallel phase;
- verifier gates and preservation audits are not part of pass execution.

The new design preserves the reference implementation's pragmatic scheduling
knowledge while making soundness depend on stable IDs, revisions, capabilities,
declared effects, verification, and explicit stage publication.

## 16. Cross-document seam rules

- `core/README.md` is authoritative if an editor operation, ID lifetime, or
  revision rule here appears broader than core permits.
- `verify/README.md` is authoritative for profiles, rule IDs, report shape, and
  publication verification. This framework chooses when to invoke it.
- `analysis/README.md` and each analysis document must define stamps,
  dependencies, semantic equality, and invalidation traits before a pass may
  preserve that analysis.
- the root `BIR README` alone defines stage/pass order. `pipeline/README.md`
  expands canonical occurrence and fixed-point orchestration beneath it. This
  file defines what executing any canonical occurrence means.
- An individual pass README may narrow its authority but may not widen this
  framework, reorder itself, weaken a verifier rule, or declare an analysis
  preserved contrary to registered traits.
- Preparation and MIR documentation consume immutable canonical output; they do
  not retroactively require canonical passes to publish target decisions.

When adjacent contracts disagree, publication fails closed until the documents
are reconciled. An implementation must not choose the more permissive reading.

## 17. Implementation prerequisites and fail-closed defaults

The normative choices used below are assigned to the existing owners. The
remaining work is implementation or local-schema completion; none permits a
different stage order, publication route, identity model, or invalidation
policy:

1. The concrete internal spelling of move-only `PassStage` remains an
   implementation detail. It must carry verified Raw semantics across private
   occurrence commits, cannot expose a forgeable stage tag, and cannot mint
   `CanonicalBir` outside the B8 publication gate.
2. Core/framework must implement the owning private occurrence fork and atomic
   occurrence publication primitive before even serial function-scope
   execution is admitted. The deterministic multi-function proposal/merge
   primitive is an additional prerequisite for parallel execution; without it
   the runner uses the serial private-fork path, and without the serial
   prerequisite it fails before consuming input.
3. `PassProperty`, `MutationEffect`, `AnalysisId`, statistic, and diagnostic
   identities are closed registries owned by their named documents. Generating
   the exact numeric tables and rejecting unknown or duplicate entries is
   implementation work, not permission to add a second registry.
4. Every analysis owner must implement its declared semantic equality and any
   preservation validator. Until a validator exists and proves equality, every
   overlapping non-empty mutation invalidates that analysis and its transitive
   dependents.
5. The initial `PassKind` choices are exactly the function/module scope table in
   `pipeline/README.md`. A future interprocedural pass that edits multiple
   bodies requires the already-specified exclusive `ModuleEditor` transaction
   and a separately reviewed registry extension; no current pass has an
   undecided scope.
6. The seven current `PassId` values remain the registered units. Any future
   split into concrete subpasses requires local transactional seams, reserved
   IDs, an explicit root/pipeline review, and no reinterpretation of an
   existing ID.
7. Each local pass contract supplies its canonical postcondition and
   unreachable-code policy; implementing those checkers cannot weaken the B8
   cumulative Canonical verifier or substitute a local check for publication.
8. Resource accounting may use only the host-independent closed units already
   admitted by `PassContext`: semantic work units, entity creation,
   diagnostics, and recursion/worklist depth. An implementation that has not
   defined a deterministic unit for an operation cannot use host timing or an
   implementation-dependent count to make a semantic decision.
9. The canonical semantic fingerprint is an implementation of canonical
   semantic equality over the exact stable-ID-addressed graph content and
   declared semantic attachments. It excludes pointers, cache state, worker
   count, timing, rendering, and diagnostic-only attachments; it is never
   sufficient by itself to excuse a non-empty mutation or mint a stage
   capability.
10. `AnalysisManager` owns cache publication and may share only immutable
    exact-key results or one exact-key in-flight computation across threads.
    Deep immutable versus reference-counted immutable storage is an
    implementation choice with no semantic effect; editors, mutable storage,
    and retargeted handles remain forbidden.
11. A committed attachment-only edit advances the owning core revision and is
    classified separately in `MutationSummary`; there is no second public
    attachment-revision axis. Registered traits may preserve and reinstall a
    semantically unaffected analysis under the new exact key, but an old handle
    is always stale after the revision advances.

Until a required implementation or local checker exists, the conservative
behavior is single-threaded execution, full invalidation, full verification,
bounded one-shot passes, and no stage publication based on an unproven
property.

## 18. Acceptance checklist

The framework design is ready for implementation only when review can answer
yes to every item:

- [ ] Every registered pass has one stable closed `PassId` and `PassKind`.
- [ ] The registry rejects duplicates, missing implementations, unknown
      analyses/properties, and invalid fixed-point contracts.
- [ ] Pipeline order is transcribed from the root `BIR README` into one checked
      descriptor and is never independently invented by this framework,
      pipeline options, or pass callbacks.
- [ ] Pass code receives immutable views plus exactly one scoped editor facade;
      no direct storage mutation path exists.
- [ ] The executor, not the pass, owns commit/rollback and derives the mutation
      summary.
- [ ] No-change/changed intent mismatches roll back and fail deterministically.
- [ ] Every successful mutation increments the correct revision exactly once.
- [ ] Every analysis handle rejects stale epoch/revision/digest access.
- [ ] Preservation is checked against actual mutation effects and analysis
      traits; old handles never become valid at a new revision.
- [ ] Debug/CI can recompute preserved analyses and compare semantic equality.
- [ ] Function parallelism freezes module tables and merges diagnostics/results
      deterministically.
- [ ] Module passes run behind an exclusive barrier with no live function
      editor, view, range, or instrumentation callback.
- [ ] Cancellation, resource exhaustion, exceptions, and verifier failure leave
      the published input and caches unchanged.
- [ ] Incremental verification is never weaker than full publication
      verification.
- [ ] Only the pipeline publication gate can manufacture `CanonicalBir`.
- [ ] Idempotent passes are second-run testable; fixed-point groups have bounds,
      progress detection, and oscillation failure.
- [ ] Instrumentation and statistics cannot mutate IR, analyses, scheduling, or
      semantic decisions.
- [ ] Legacy Route1-Route8 behavior is mapped to core, analysis, pass, or
      verifier ownership without retaining legacy APIs.
- [ ] ABI, call placement, frame, regalloc, spill, MIR, target, and rendering
      facts are absent from canonical pass inputs and outputs.
- [ ] Reference backend scheduling strengths are retained without direct
      mutation, manual freshness, or heuristic-only correctness.
- [ ] Negative tests cover stale analysis, undeclared mutation, false
      preservation, double commit, failed rollback, nondeterministic merge,
      non-convergence, cancellation, budget exhaustion, and forged publication.

## 19. Evidence inspected

- New BIR contracts: `core/README.md`, `verify/README.md`,
  `analysis/README.md`, `pipeline/README.md`, and the Raw importer contract.
- Legacy semantic routes: `bir_route1.cpp` through `bir_route8.cpp`, route
  facade/index headers, control-flow/memory/comparison/call/publication views,
  `query.cpp`, `bir_validate.cpp`, and legacy LIR-to-BIR entry points.
- Legacy orchestration: `prealloc/prealloc.cpp`, `legalize.cpp`, `liveness.cpp`,
  `out_of_ssa.cpp`, `regalloc.cpp`, prepared contract verification, and the
  prealloc contract README.
- Reference compiler: `ref/claudes-c-compiler/src/passes/mod.rs`, its pass
  README, CFG analysis use, dirty tracking, shared analysis, dependency skips,
  bounded iteration, and backend analysis/regalloc/stack-layout boundaries.
