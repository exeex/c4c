# Reference Backend Comparison

The reference is `ref/claudes-c-compiler`. This comparison cites its concrete
Rust sources; “adopt” and “reject” are c4c design decisions measured against
`03_pass_ready_bir_contract.md`, not claims that the reference intended the same
contract.

## Concrete reference model

### Module, function, and block ownership

`IrModule` directly owns `Vec<IrFunction>`, globals, literal pools, constructors,
destructors, aliases, assembly, and symbol directives
(`src/ir/module.rs:13-38`). `IrFunction` directly owns `Vec<BasicBlock>` plus its
signature/linkage/ABI and cached `next_value_id` state (`module.rs:165-219`). A
`BasicBlock` directly owns `Vec<Instruction>`, one `Terminator`, and a parallel
`source_spans` vector (`src/ir/instruction.rs:57-66`). Passes normally receive
`&mut IrFunction` and edit these vectors directly; `run_on_visited` iterates
`module.functions.iter_mut()` (`src/passes/mod.rs:31-59`).

This is a clean direct-transformation ownership shape, but vector addresses and
positions are not stable identities. `source_spans` also demonstrates a
parallel-array invariant that every instruction edit must maintain.

### `BlockId`, `Value`, and instruction identity

`BlockId(pub u32)` is copyable/hashable and formats directly as `.LBB{id}`
(`instruction.rs:28-43`). `BasicBlock::label` carries it, terminators target it,
and `Instruction::Phi` stores `incoming: Vec<(Operand, BlockId)>`
(`instruction.rs:57-66`, `240-247`). `Value(pub u32)` is the SSA reference and
`Operand` is `Value` or constant (`instruction.rs:46-54`). `IrFunction` caches an
upper bound in `next_value_id`; zero falls back to scanning (`module.rs:178-183`).

There is no `InstId`: an instruction is its enum value at a vector position.
There is also no owner/generation component in `BlockId` or `Value`. Phi
elimination computes a global maximum `BlockId` across functions because labels
must not collide, then allocates monotonically (`mem2reg/phi_eliminate.rs:51-69`).
Those IDs are more semantic than names but do not detect stale handles or slot
reuse.

### `FlatAdj` and `CfgAnalysis`

`build_label_map` converts semantic-looking `BlockId` labels to dense
`usize` block positions (`src/ir/analysis.rs:91-98`). `build_cfg` reads each
block's `Terminator` and produces predecessors/successors; terminators are thus
the ordinary CFG source (`analysis.rs:100-150`). `FlatAdj` stores CSR
`offsets: Vec<u32>` and `data: Vec<u32>` and exposes a dense row by position
(`analysis.rs:28-89`).

`CfgAnalysis::build` bundles `preds`, `succs`, immediate dominators,
dominator-tree children, and `num_blocks` (`analysis.rs:352-379`). The important
granularity transition is explicit:

1. Before CFG construction, the function is addressed by stable-within-current-
   IR `BlockId`; `build_label_map` correlates each ID to current vector position.
2. After construction, `FlatAdj`, `idom`, and `dom_children` use dense function-
   local block indices. They are compact and fast, but valid only while the
   function's block membership/order and terminator edges match the build.

That separation is the model c4c wants: semantic IDs at the IR boundary, dense
indices inside a disposable analysis result.

### Pass execution and sharing

`run_passes` operates on `&mut IrModule`; per-function helpers visit selected
functions and mark changed functions by module vector index (`passes/mod.rs:31-59`,
`284-500`). `run_gvn_licm_ivsr_shared` builds one `CfgAnalysis` for each dirty
multi-block function and passes it successively to GVN, LICM, and IV strength
reduction (`passes/mod.rs:61-151`). Comments justify reuse because GVN changes
operands but not CFG, and LICM moves instructions without adding/removing blocks
(`passes/mod.rs:116-143`). Single-block GVN bypasses CFG construction.

This is manual preservation knowledge embedded in pipeline orchestration. CFG
simplification is run outside that shared group; a later group rebuilds its
analysis. There is no revision token, analysis manager, mutation summary, or
general invalidation protocol. “Changed” selects future work, but it does not
prove a cached analysis is valid.

### Phi removal and direct CFG mutation

The driver runs `promote_allocas`, optimization passes, then
`eliminate_phis`, explicitly before target code generation
(`src/driver/pipeline.rs:1078-1103`). It deliberately avoids copy propagation
afterward because the IR is no longer SSA and those copies are
program-point-sensitive (`pipeline.rs:1092-1100`).

`eliminate_phis` walks module functions and shares a module-wide next block
number (`mem2reg/phi_eliminate.rs:51-69`). Per function, `PhiElimCtx` builds a
`BlockId -> usize` map, positional predecessor facts, copy queues, trampoline
maps, and a next value number (`phi_eliminate.rs:237-283`). It snapshots phis,
detects parallel-copy conflicts, allocates temporaries, and places copies on
predecessors (`phi_eliminate.rs:288-460`). Critical edges get trampoline blocks;
`retarget_block_edge_once` edits a terminator or inline-asm goto edge
(`phi_eliminate.rs:75-120`, `434-459`). Finally it removes phi instructions,
keeps `source_spans` parallel, inserts copies, retargets edges, appends
trampolines, and updates `next_value_id` (`phi_eliminate.rs:462-500` and the
remainder of `apply_phi_transformations`).

This is valuable evidence that phi removal is a distinct stage transformation,
and that correct edge-specific copies require cycle handling and critical-edge
splitting. It also shows the risk of bespoke mutation: vector indices, labels,
spans, terminators, phi lists, and cached next IDs are synchronized manually.

## Decisions for c4c

| Decision | Reference evidence | Corresponding c4c requirement |
|---|---|---|
| **Adopt: module owns functions/globals; function owns blocks; block owns ordered instructions and one terminator.** | `IrModule`, `IrFunction`, and `BasicBlock` ownership (`module.rs:13-38`, `165-219`; `instruction.rs:57-66`). | Step 3 ownership contract, but behind stable-ID storage and editors rather than public mutable vectors. |
| **Adopt: terminator-derived CFG.** | `build_cfg` reads terminators to construct edges (`analysis.rs:100-150`). | Terminator is the sole CFG authority; `redirect_edge`/`split_block` update phi edge semantics transactionally. |
| **Adopt: stable semantic block/value IDs at IR boundaries.** | `BlockId` targets and phi predecessor keys; `Value` operands (`instruction.rs:28-54`, `240-247`). | c4c `BlockId` and `ValueId`, extended with owner/generation and verified resolution. |
| **Adopt: dense ephemeral analysis indices.** | `build_label_map` converts `BlockId` to position; `FlatAdj` and dominator vectors use dense rows (`analysis.rs:28-98`, `352-379`). | Analysis-local indices live only in immutable `{FunctionId, revision}` results and never escape as semantic handles. |
| **Adopt: disposable per-function contexts.** | `CfgAnalysis::build` and per-function `PhiElimCtx` construct local maps/vectors (`analysis.rs:362-378`; `phi_eliminate.rs:244-283`). | Function analysis manager rebuilds A data; mutation transactions own temporary state. |
| **Adopt: direct function transformations and explicit stage boundary for phi removal.** | Passes take `&mut IrFunction`; driver runs `eliminate_phis` after SSA optimization and before codegen (`passes/mod.rs:31-59`; `pipeline.rs:1078-1103`). | Canonical passes edit one function through `FunctionEditor`; out-of-SSA produces a later prepared/MIR-stage form, not hidden core side data. |
| **Adopt: critical-edge and parallel-copy correctness principles.** | Phi elimination detects conflicts, uses temporaries, and creates trampolines (`phi_eliminate.rs:302-459`). | Future c4c out-of-SSA uses `split_block`/`redirect_edge`, stable IDs, verifier checks, and explicit post-SSA stage output. |
| **Reject: absent stable instruction identity.** | `BasicBlock.instructions: Vec<Instruction>` has no `InstId` (`instruction.rs:57-66`); transformations retain/insert by position. | c4c requires generation-checked `InstId`, order storage separate from identity, and stable moved instruction handles. |
| **Reject: position identity leaking from analysis into mutation.** | `BlockId -> usize` maps feed dense `FlatAdj`; phi context and copy queues key by `usize` (`analysis.rs:91-150`; `phi_eliminate.rs:244-283`). | Dense indices cannot escape analysis/context lifetime; editor APIs accept stable IDs only. |
| **Reject: ownerless/generationless `BlockId` and `Value`.** | Both are bare `u32` wrappers (`instruction.rs:28`, `46`), and new IDs are found through maxima (`phi_eliminate.rs:51-69`, `274-279`). | c4c IDs encode owner/epoch/slot/generation; stale or cross-function references fail deterministically. |
| **Reject: implicit/manual invalidation.** | Shared CFG reuse relies on comments that particular passes preserve CFG (`passes/mod.rs:113-143`); no revision accompanies `CfgAnalysis`. | Every transaction increments revision and emits `MutationSummary`; analyses declare preservation or are invalidated automatically. |
| **Reject: public vector mutation as the transformation API.** | Passes mutate `func.blocks`/`instructions` directly; phi removal manually synchronizes `source_spans`, edges, phis, and IDs (`phi_eliminate.rs:462-500`). | Only `FunctionEditor` may insert, erase, RAUW, split, or redirect; it maintains use-lists and commits after verification. |
| **Reject: cached scalar state as unverified authority.** | `next_value_id` is a manually maintained cached upper bound with scan fallback (`module.rs:178-183`; `phi_eliminate.rs:280-283`). | Allocation authority belongs to stable storage; recomputable counts/maps are revision-bound analyses and verifier-auditable. |
| **Reject: target/ABI mutable state mixed into canonical function schema.** | `IrFunction` and `CallInfo` carry target-specific ABI classes and flags (`module.rs:200-219`; `instruction.rs:69-110`). | Step 2 assigns computed ABI/frame/allocation facts to prepared/MIR output; canonical BIR retains only semantic call attributes. |

## Net result

The reference demonstrates a productive architecture: transform owned
functions directly, derive CFG from terminators, correlate semantic `BlockId`
with compact `FlatAdj` indices, share disposable analysis where preservation is
known, and lower phi nodes at an explicit post-SSA boundary. c4c should adopt
those principles while rejecting the parts that make mutation correctness
conventional rather than enforceable: no `InstId`, bare numeric identity,
position leakage, public vector edits, and implicit invalidation.

The c4c target is therefore stricter, not structurally alien: the same useful
function/block/CFG shape wrapped in stable storage, contracted mutators, revision
checks, verification, and a clean canonical-to-prepared/MIR boundary.
