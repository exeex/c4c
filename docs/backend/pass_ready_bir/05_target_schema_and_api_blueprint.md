# Target Schema and API Blueprint

This blueprint makes Steps 1–4 implementable. Names are proposed C++ source
paths and public symbols; later migration may split implementation files for
build size, but must preserve these ownership and stage boundaries.

## Target source tree

```text
src/backend/bir/
  core/
    ids.hpp                    FunctionId, BlockId, InstId, ValueId, LocalId
    handles.hpp                resolving Handle<T>, mutation-scoped Ref<T>
    opcode.hpp                 opcode/terminator/attribute definitions
    value.hpp                  constants, ValueDef, ValueUse
    instruction.hpp            InstData, operand/successor descriptors
    block.hpp                  BlockData and instruction order
    function.hpp               FunctionData, local storage, FunctionRevision
    module.hpp                 ModuleData, symbols/types/globals/functions
    storage.hpp                generation-checked SlotMap<T, Id>
    traversal.hpp/.cpp         instructions, operands, users, successors
    builder.hpp/.cpp           ModuleBuilder, FunctionBuilder, RawBir
    editor.hpp/.cpp            FunctionEditor transactions and RAUW
    cfg_mutation.hpp/.cpp      split_block, redirect_edge, erase_block
  verify/
    verifier.hpp/.cpp          structured verification and profiles
    rules_core.cpp             ownership, IDs, types, def-use, dominance
    rules_cfg.cpp              terminators, edges, phi/block arguments
    rules_stage.cpp            raw/canonical stage legality
  analysis/
    manager.hpp/.cpp           revision-keyed FunctionAnalysisManager
    cfg.hpp/.cpp               terminator-derived dense CFG
    dominance.hpp/.cpp
    memory_effects.hpp/.cpp
    provenance.hpp/.cpp
    call_graph.hpp/.cpp        ModuleRevision-keyed module analysis
  passes/
    pass.hpp                   FunctionPass, ModulePass, PreservedAnalyses
    pipeline.hpp/.cpp          canonical pass pipeline and verification gates
    canonicalize_*.cpp         semantic BIR-to-BIR passes only
  compatibility/
    legacy_capsule.hpp/.cpp    LegacyBirCompatibilityCapsule implementation
    legacy_manifest.cpp        generated field/reader allowlist checks
    legacy_adapters.cpp        named temporary readers only
  preparation/
    prepared_bir.hpp           PreparedBir and per-function prepared records
    prepare.hpp/.cpp           verified CanonicalBir -> PreparedBir
    abi_plan.hpp/.cpp          parameter/result/call ABI and call moves
    frame_plan.hpp/.cpp        frame objects, offsets, prologue/epilogue
    allocation_plan.hpp/.cpp   virtual/physical register allocation products
    address_plan.hpp/.cpp      target address materialization decisions
  mir/
    mir.hpp                    target-independent machine IR ownership
    lower_bir_to_mir.hpp/.cpp  PreparedBir -> MirModule
    instruction_select.cpp     target opcode/constraint selection
  lir_to_bir/
    import.hpp/.cpp            lower_lir_to_raw_bir entry point
    ...                        existing adapter families, progressively isolated
```

Existing `bir.hpp`, route files, prealloc, and target emitters become migration
facades; they are not target ownership locations. Core headers cannot include
`compatibility/`, `preparation/`, `mir/`, LIR, prealloc, or target headers.
Analysis may include core; passes may include core/analysis/verify; preparation
may include the read-only canonical API; MIR lowering may include preparation.
Dependency direction is enforced in the build.

## Core IDs and storage

`ids.hpp` defines opaque owner/generation IDs:

```cpp
struct FunctionId { ModuleEpoch epoch; SlotIndex slot; Generation generation; };
struct BlockId    { FunctionId owner; SlotIndex slot; Generation generation; };
struct InstId     { FunctionId owner; SlotIndex slot; Generation generation; };
struct ValueId    { FunctionId owner; ValueKind kind; SlotIndex slot; Generation generation; };
```

Module symbols/types/globals use analogous module-owned IDs. `SlotMap` owns
objects, generation counters, and tombstones; separate `IdOrder` lists own
function, block, and instruction iteration order. Compaction changes addresses,
never IDs. `Handle<T>` resolves ID on access and returns an error for wrong
owner/epoch/generation. `Ref<T>` and iterators carry a structural-mutation token
and cannot be retained after an editor commits.

```cpp
struct ModuleData {
  ModuleEpoch epoch;
  ModuleRevision revision;
  SymbolTable symbols;
  TypeStore types;
  SlotMap<GlobalData, GlobalId> globals;
  SlotMap<FunctionData, FunctionId> functions;
  IdOrder<FunctionId> function_order;
};

struct FunctionData {
  FunctionSignature signature;
  FunctionRevision revision;
  SlotMap<BlockData, BlockId> blocks;
  SlotMap<InstData, InstId> insts;
  SlotMap<ValueDef, ValueId> values;
  SlotMap<LocalData, LocalId> locals;
  IdOrder<BlockId> block_order;
  DefUseStore uses;
};

struct BlockData {
  IdOrder<InstId> instructions;
  Terminator terminator;
  DebugBlockName debug_name; // optional, non-authoritative
};
```

`InstData` is `{Opcode, result ValueIds, operand ValueIds, semantic attributes}`.
Opcode descriptors enumerate operand roles, results, memory effects, and legal
successor/terminator forms. Atomic operations are ordinary instructions.
Calls contain semantic callee, arguments, type/signature attributes, and source
calling-convention constraints, but no computed register/stack plan. Phi
incomings are `(BlockId, ValueId)`; labels are debug data. `Terminator` alone
owns successor `BlockId`s.

## Public construction, traversal, and mutation APIs

`builder.hpp`:

```cpp
Expected<RawBir> lower_lir_to_raw_bir(const lir::LirModule&, ImportOptions);
FunctionId ModuleBuilder::create_function(FunctionSignature, SymbolId);
BlockId FunctionBuilder::create_block(BlockPosition, DebugBlockName = {});
BuildResult FunctionBuilder::append(BlockId, InstSpec);
void FunctionBuilder::set_terminator(BlockId, TerminatorSpec);
```

Adapter-private spelling maps, aggregate-layout parsing, and legacy resolution
stay inside `lir_to_bir/`; they cannot appear in `RawBir` core fields. Optional
legacy observations attach through an opaque compatibility token.

`traversal.hpp` exposes stable-ID ranges:

```cpp
InstRange instructions(const FunctionView&, BlockId);
OperandRange operands(const FunctionView&, InstId);
UseRange users(const FunctionView&, ValueId);
BlockRange successors(const FunctionView&, BlockId); // from terminator
ResultRange results(const FunctionView&, InstId);
```

`editor.hpp` and `cfg_mutation.hpp` expose the only writable path:

```cpp
Expected<InstId> insert_before(InstId, InstSpec);
Expected<InstId> insert_after(InstId, InstSpec);
Expected<InstId> replace_inst(InstId, InstSpec, ResultMapping);
Expected<void> erase_inst(InstId);
Expected<void> set_operand(ValueUse, ValueId);
Expected<void> replace_all_uses_with(ValueId, ValueId); // RAUW
Expected<BlockId> split_block(BlockId, InstId before);
Expected<void> redirect_edge(BlockId, BlockId, BlockId, PhiTransferPlan);
Expected<void> erase_block(BlockId);
Expected<MutationSummary> commit();
```

An editor is an exclusive per-function transaction. It maintains def-use,
orders, terminator/phi consistency, and generations; commit runs local verifier
rules, increments `FunctionRevision` once, and hands the mutation summary to the
analysis manager. Direct mutable access to storage/order vectors is private.

## Verification and analysis APIs

`verifier.hpp`:

```cpp
enum class VerifyProfile { Raw, Canonical, PreparedInput };
VerificationResult verify(const ModuleView&, VerifyProfile);
VerificationResult verify(const FunctionView&, VerifyProfile);
struct VerificationError { RuleId rule; FunctionId function; EntityId entity;
                           OperandRole role; std::string message; };
```

Core verification covers IDs/ownership/generations/order membership, types and
opcode arity, exact def-use, dominance, terminator-derived edges, phi predecessor
sets, signatures/calls, and stage legality. `Canonical` rejects lowering-only,
prepared/MIR, raw pointer/index identity, and compatibility access.
`PreparedInput` additionally requires the canonical stage token described below.

`analysis/manager.hpp`:

```cpp
template<class A> Shared<const A::Result> get(FunctionId);
void apply(FunctionId, FunctionRevision, const MutationSummary&,
           const PreservedAnalyses&);
void invalidate(FunctionId, AnalysisSet);
```

Every result includes `{FunctionId, FunctionRevision}`. CFG builds `BlockId ->
DenseBlockIndex` locally and stores `FlatAdj`-style dense adjacency; dense
indices never enter core or pass APIs. Module analyses use `{ModuleEpoch,
ModuleRevision}`. A pass returns `PreservedAnalyses`, checked against mutation
categories; unknown effects invalidate all. Analyses are immutable and
recomputable, never semantic side tables.

## Typed stage pipeline

The required top-level flow is:

```cpp
Expected<RawBir> raw = lower_lir_to_raw_bir(lir_module, import_options);
verify(raw.view(), VerifyProfile::Raw);

Expected<CanonicalBir> canonical =
    run_canonical_bir_pipeline(std::move(raw), pipeline_options);
// pipeline verifies after passes as configured and always verifies Canonical

Expected<PreparedBir> prepared =
    prepare_bir(canonical->view(), target, preparation_options);
Expected<MirModule> mir = lower_bir_to_mir(*prepared);
```

`RawBir` and `CanonicalBir` need not duplicate full module storage. They are
move-only wrappers around `unique_ptr<ModuleData>` plus an unforgeable state
token:

```cpp
class RawBir       { unique_ptr<ModuleData> data_; RawState token_; };
class CanonicalBir { unique_ptr<ModuleData> data_; CanonicalState token_; };
```

Only `run_canonical_bir_pipeline` can consume `RawBir` and construct
`CanonicalBir`; it does so after successful canonical verification. Mutation of
a published `CanonicalBir` requires consuming it into a new canonicalization
session and re-verifying before a new wrapper is returned. `CanonicalBir::view`
is read-only. The wrapper therefore represents proof/state, not a duplicate IR.

`PreparedBir` owns a read-only reference/shared lifetime to `CanonicalBir` plus
all target-derived products and the exact canonical/module revisions they were
built from. It is invalid if those revisions differ. `MirModule` owns its own
machine-stage identities and instructions; BIR IDs may appear only as optional
debug/provenance links, not MIR identity.

## Stage authority ledger

| Authority | Owning type/file | Prohibited locations |
|---|---|---|
| symbols, resolved types, globals, functions, blocks, values, semantic instructions, terminators, locals | `ModuleData`/`FunctionData` in `core/` | analyses, capsule, prepared plans, MIR must not override them |
| CFG successors | core `Terminator` | no authoritative predecessor/successor side table |
| predecessors, dominance, loops, def-path/provenance/effects, call graph | immutable revision-keyed results in `analysis/` | core fields and compatibility capsule |
| legacy spellings/fallbacks/route snapshots | `LegacyBirCompatibilityCapsule` in `compatibility/` | core passes, verifier decisions, preparation decisions |
| import parsing/layout parity/name bridges | `lir_to_bir/` temporaries | any published `RawBir`/`CanonicalBir` field |
| target data layout choice and ABI classification | `PreparedBir::{TargetContext,AbiPlan}` | canonical Module/Function/Call fields except explicit source calling-convention semantics |
| call register/stack assignment and call moves | `CallPlan` in `preparation/abi_plan.*` | core call instruction and compatibility data |
| frame objects, offsets, alignment realization, prologue/epilogue | `FramePlan` in `preparation/frame_plan.*` | canonical locals/instructions |
| virtual/physical register allocation, spill/reload decisions | `AllocationPlan` and MIR | canonical values and analyses |
| address materialization mode/placement | `AddressPlan` and MIR | canonical global/memory instruction policy fields |
| target opcode, constraints, inline-asm realization, instruction selection | `MirModule` / `instruction_select.cpp` | canonical BIR opcode metadata beyond semantic intrinsic/asm operation |

Preparation may compute ABI, frame, allocation, call-move, and address plans in
separate subpasses, but later subpasses consume typed earlier outputs rather
than mutating canonical BIR. Instruction selection consumes verified prepared
facts and produces MIR. Target emitters consume MIR plus object-data products;
they cannot query the compatibility capsule or adapter tables.

## Module and function processing

`CanonicalPipeline` freezes module-owned symbol/type/global/function tables
during a parallel function phase. Each worker receives `ModuleSymbolView`, one
exclusive `FunctionEditor`, and a function analysis manager. Module passes use
`ModuleEditor` at a barrier, update `ModuleRevision`, and invalidate module and
interprocedural analyses. Stable IDs survive module vector/storage growth.

Preparation follows the same decomposition but is read-only with respect to
`CanonicalBir`: per-function plans can be built in parallel against immutable
module context, then assembled deterministically in function order. No prepared
worker can publish facts back into canonical storage.

## Compatibility boundary

`compatibility/legacy_capsule.hpp` is forward-declared only by the raw wrapper;
its definition is private to compatibility code. `LegacyReaderToken` factories
exist only for manifest allowlisted adapters. Build rules reject compatibility
includes from `core/`, `analysis/`, `passes/`, `verify/`, `preparation/`, and
`mir/`. Field/reader manifests and zero-count gates are those from Step 2.

This leaves no mixed-stage authority: canonical semantic state is stable-ID
core IR; derived semantic questions are analyses; legacy state is observational;
and target realization begins only after a verified `CanonicalBir` token exists.
