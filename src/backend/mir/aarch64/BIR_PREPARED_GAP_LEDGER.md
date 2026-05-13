# AArch64 BIR / Prepared Gap Ledger

This ledger compares the Step 4 `BACKEND_ENTRY_CONTRACT.md` requirements
against the current BIR and `PreparedBirModule` surfaces. It records whether
the fact is available now, whether it needs AArch64 target-local design, and
whether a separate preparation-carrier initiative must be split before backend
implementation resumes.

Status terms:

- `present`: structured data exists in current BIR / `PreparedBirModule` and
  may be consumed directly by AArch64 lowering.
- `missing`: the required carrier or target-local structure does not exist.
- `ambiguous`: a carrier exists, but target-specific meaning or completeness is
  not settled enough for AArch64 lowering to treat it as authoritative.
- `deferred`: the fact belongs after target MIR, assembler, object, linker, or
  separately prioritized feature work.

## Source Surfaces

- Contract: `src/backend/mir/aarch64/BACKEND_ENTRY_CONTRACT.md`.
- Responsibility/classification index:
  `src/backend/mir/aarch64/CLASSIFICATION_INDEX.md`.
- Shared prepared carrier: `src/backend/prealloc/prealloc.hpp`
  `PreparedBirModule`.
- Shared public backend staging: `src/backend/backend.hpp` and
  `src/backend/backend.cpp`.
- Existing target-local precedent: x86 consumes `PreparedBirModule` through
  `src/backend/mir/x86/api/api.hpp`, `src/backend/mir/x86/module/module.hpp`,
  `src/backend/mir/x86/prepared/prepared.hpp`, and route-debug surfaces.
- Current AArch64 codegen headers still expose raw `bir::Module` and LIR text
  paths through `src/backend/mir/aarch64/codegen/emit.hpp`.
- Current AArch64 assembler/linker headers expose staged text/object/link
  surfaces under `src/backend/mir/aarch64/assembler/` and
  `src/backend/mir/aarch64/linker/`.

## Layout Ownership Ledger

This table is the Step 2 ownership contract. "Owner" names the first AArch64
file or deferred contract allowed to own the family. "Target-local record"
names the existing or required AArch64 record surface; it is not permission to
add behavior in this plan. "First allowed route" is the first implementation
initiative that may touch behavior after this markdown contract is accepted.

| Feature family | Owner | Carrier status | Target-local record type | First allowed route |
| --- | --- | --- | --- | --- |
| Public prepared-module entry | `api/api.hpp`, `api/api.cpp` | Present prepared entry: `build_prepared_module(const PreparedBirModule&)`. Raw `bir::Module`, LIR text, and assembly text fallback are rejected. | `module::BuildResult` / `module::Module` returned through the public API. | AArch64 prepared-module target MIR boundary; no direct text emitter route. |
| Target profile and handoff validation | `abi/abi.hpp`, `abi/abi.cpp` | Present prepared carrier: `PreparedBirModule::target_profile`; AArch64/AAPCS64 validation exists. | `abi::HandoffError`, `abi::HandoffErrorKind`. | AArch64 target ABI validation and diagnostics only; no lowering behavior. |
| AAPCS64 ABI call/return/variadic policy | Later `abi/` contract plus `module/` snapshots | Prepared call plans are present, but target ABI completeness is ambiguous until AAPCS64 lowering policy is reviewed. | Existing snapshots: `CallRecord`, `CallArgumentRecord`, `CallResultRecord`, `CallPreservedValueRecord`, `CalleeSaveRecord`, `AbiBindingRecord`. | AArch64 target ABI/call-frame design idea before instruction selection. |
| Module/function/block containers | `module/module.hpp`, `module/module.cpp` | Present BIR and prepared carriers: retained BIR module plus `PreparedControlFlow`. | `Module`, `FunctionRecord`, `BlockRecord`. | AArch64 prepared-module target MIR boundary; containers stay module-owned. |
| Semantic ids and name tables | `module/module.hpp`, with lookup authority from shared preparation | Present prepared carrier: `PreparedNameTables` and BIR ids. | Stored ids/labels inside `Module`, `FunctionRecord`, `BlockRecord`, `OperandRecord`, data records. | Target MIR boundary; labels are display-only, not semantic recovery. |
| Operand/value identity | `module/module.hpp` | Present prepared carriers: `PreparedValueLocations`, `PreparedStoragePlans`, retained BIR value/type facts. | `OperandRecord`. | Target MIR operand model; no rendered-name or old register-string recovery. |
| Register banks/classes and physical references | `module/module.hpp` for snapshots; later `abi/` or `codegen/` may define typed AArch64 registers | Present prepared carrier for banks/classes/assignments; target-specific typed register enum remains missing. | `TargetRegisterRecord` plus `CalleeSaveRecord` register fields. | Target MIR/register model idea before instruction selection. |
| Frame layout, stack slots, prologue, and epilogue facts | `module/module.hpp` owns prepared snapshots; later `abi/` owns AAPCS64 prologue/epilogue policy | Present prepared carriers: `PreparedStackLayout`, `PreparedFramePlan`, saved-register plan. | `FrameRecord`, `FrameSlotRecord`, `CalleeSaveRecord`. | Target ABI frame/prologue design after target MIR records are accepted. |
| Dynamic stack | `module/module.hpp` | Present prepared carrier: `PreparedDynamicStackPlan`. | `DynamicStackRecord` inside `FrameRecord`. | Target MIR dynamic-stack operation design. |
| Branches, compares, block transfers | `module/module.hpp`; later `codegen/` owns lowering rules | Present prepared carriers: `PreparedControlFlow`, `PreparedBranchCondition`, `PreparedJoinTransfer`, branch labels. | `BranchRecord`, `BlockRecord`, `ParallelCopyRecord`. | Target MIR branch/compare model; instruction selection comes later. |
| Join copies and parallel copies | `module/module.hpp` | Present prepared carriers: `PreparedParallelCopyBundle`, `PreparedMoveBundle`, `PreparedJoinTransfer`. | `ParallelCopyRecord`, `MoveRecord`. | Target MIR move/copy design over prepared bundles. |
| Calls and indirect calls | `module/module.hpp` snapshots; later `abi/` owns AAPCS64 lowering policy | Present prepared carrier: `PreparedCallPlans`; target ABI completeness ambiguous. | `CallRecord`, `CallArgumentRecord`, `CallResultRecord`, `CallPreservedValueRecord`. | AArch64 target ABI/call-frame design; do not synthesize calling convention locally. |
| Returns | `abi/` contract for AAPCS64 policy, with `module/` call/result snapshots | BIR return terminators are present; prepared call result/storage facts exist; complete AAPCS64 return policy is ambiguous. | Existing `BlockRecord` terminator fields and `CallResultRecord`; a dedicated return record is not present. | Target ABI return-value design before return lowering. |
| Memory and addressing | `module/module.hpp` for current snapshots; shared preparation owns missing facts | Prepared addressing exists for base/offset/size/alignment; volatility and address-space are missing from visible prepared memory access. | Existing `OperandRecord` address fields; a dedicated AArch64 memory operand record is still required before lowering. | Split shared prepared volatility/address-space carrier before memory lowering, then target MIR memory operand design. |
| Globals, strings, data, symbols, relocation needs | `module/module.hpp`; later object/binary-utils contract owns encoding/emission | Present BIR carriers for globals/strings and link names; object relocation semantics deferred. | `GlobalDataRecord`, `StringDataRecord`, `DataRelocationNeedRecord`, `SymbolVisibilityRecordKind`, `DataRelocationNeedKind`. | Structured data/object boundary idea; no object writer behavior in this plan. |
| Moves, ABI bindings, spills, reloads | `module/module.hpp` | Present prepared carriers: regalloc, value locations, storage plans, move bundles, spill/reload ops. | `MoveRecord`, `AbiBindingRecord`, `SpillReloadRecord`, `ParallelCopyRecord`. | Target MIR move/spill/reload design. |
| Scalar integer operations and casts | Later `codegen/` after target MIR exists | BIR operation/type facts are present in retained module; no AArch64 instruction record/lowering owner yet. | Missing target instruction/operation record; do not use legacy `codegen/*.md` text patterns as records. | Target MIR instruction record design, then scalar instruction-selection idea. |
| Floating-point scalar operations | Later `codegen/` after target MIR exists | BIR operation/type facts are present for supported types; no AArch64 FP instruction record/lowering owner yet. | Missing target FP instruction/operation record. | Target MIR instruction record design, then FP lowering idea. |
| F128 and i128 special-width operations | Deferred contract | Support policy and complete carriers are deferred; legacy notes are `delete/defer`. | Deferred; no target-local record accepted in this plan. | Separate f128/i128 support policy idea before any lowering. |
| Atomics | Deferred target ABI/codegen contract | BIR/prepare authority for ordering and target lowering completeness is not accepted here. | Deferred; no target-local atomic record accepted in this plan. | Separate atomics lowering idea with ABI and memory-order carrier review. |
| Intrinsics | Deferred codegen contract | Intrinsic inventory and carriers are not accepted as part of this layout contract. | Deferred; no target-local intrinsic record accepted in this plan. | Separate intrinsic inventory/lowering idea. |
| Inline assembly | Deferred structured inline-asm contract | Legacy string-substitution route is obsolete; no structured operand/clobber contract accepted. | Deferred; no target-local inline-asm record accepted in this plan. | Separate inline-asm contract idea; no parser/string recovery. |
| Assembler parser and directives | Deferred assembler contract | Legacy parser exists only as a classified markdown candidate; parser operands cannot supply backend semantics. | Deferred assembler request/operand records, not current MIR records. | Explicit assembler-layer rebuild idea after target MIR/instruction records. |
| Encoder families | Deferred assembler/encoder contract | Legacy encoder facts may be salvageable later; no codegen dependency is accepted now. | Deferred structured instruction/encoding records. | Encoder API rebuild over target instruction records. |
| ELF object writer | Deferred object/binary-utils contract | Legacy object-writer notes are binary-utils candidates; no current AArch64 object writer behavior is owned here. | Deferred object/section/symbol/relocation records. | Shared binary-utils/object writer idea. |
| Binary utilities | Deferred shared binary-utils contract | Useful only after object/relocation scope is selected. | Deferred shared records, not target MIR records. | Shared binary-utils contract idea. |
| Linker input, symbols, relocations, PLT/GOT, static/dynamic/shared emission | Deferred linker contract | Legacy linker notes are not backend output requirements. | Deferred linker records. | Separate linker initiative; no AArch64 backend dependency. |

Current `module/module.hpp` records remain module-owned when they are snapshots
of accepted BIR or `PreparedBirModule` facts: module/function/block identity,
operands, frame snapshots, branches, calls, moves, ABI bindings,
spills/reloads, parallel copies, globals, strings, and relocation needs. They
do not own final AAPCS64 policy, instruction selection, assembler encoding,
object emission, binary utility behavior, or linker orchestration. Those
families require later `abi/`, `codegen/`, `assembler/`, object/binary-utils,
linker, or separate deferred contracts before behavior changes.

## BIR / Prepared Carrier Checklist

This checklist expands the ownership ledger into a reviewer-readable carrier
map. "BIR carrier" names the semantic source when the fact remains in retained
BIR. "Prepared carrier" names the structured preparation surface when it exists.
"Target record" names the AArch64 snapshot or the explicitly missing/deferred
record. The status column is the current permission boundary for later work.

| Feature family | BIR carrier | Prepared carrier | Target record | Owner | Status |
| --- | --- | --- | --- | --- | --- |
| Public prepared-module entry | Retained `bir::Module` only as input to preparation; raw BIR emitters are not accepted entry carriers. | `PreparedBirModule` passed through `build_prepared_module(const PreparedBirModule&)`. | `module::BuildResult`, `module::Module`. | `api/api.hpp`, `api/api.cpp`. | present for prepared entry; raw `emit_direct_bir_module`, raw `emit_module`, LIR text, assembly text fallback are rejected for this route. |
| Target profile and handoff validation | Target triple facts before preparation. | `PreparedBirModule::target_profile`. | `abi::HandoffError`, `abi::HandoffErrorKind`. | `abi/abi.hpp`, `abi/abi.cpp`. | present for AArch64/AAPCS64 validation; lowering must still gate on this. |
| Module containers | `bir::Module`, `bir::Function`, `bir::Block`. | `PreparedBirModule::module`, `PreparedControlFlow`. | `Module`, `FunctionRecord`, `BlockRecord`. | `module/module.hpp`. | present as snapshots; container ownership does not imply instruction selection. |
| Semantic ids and names | BIR function, block, value, slot, link, and text ids. | `PreparedNameTables`. | Id and label fields inside module, function, block, operand, data, and relocation records. | `module/module.hpp`, with lookup authority from shared preparation. | present; labels are display or diagnostics only, not semantic recovery. |
| Operand/value identity | `bir::Value`, BIR type facts, value names. | `PreparedValueLocations`, `PreparedRegalloc`, `PreparedStoragePlans`, retained BIR values. | `OperandRecord`. | `module/module.hpp`. | present as snapshots; no rendered-name or old register-string recovery. |
| Register banks/classes and physical references | Type and value identity in BIR. | `PreparedRegisterGroupOverrides`, `PreparedRegalloc`, `PreparedStoragePlans`, `PreparedSavedRegister`. | `TargetRegisterRecord`, `CalleeSaveRecord` register fields. | `module/module.hpp` snapshots; later `abi/` or `codegen/` typed-register contract. | present for prepared banks/classes and string physical references; missing final typed AArch64 register enum/model. |
| Stack objects and frame slots | Stack object and allocation-driving BIR facts. | `PreparedStackLayout`, `PreparedFrameSlot`. | `FrameSlotRecord`, fields in `FrameRecord`. | `module/module.hpp`. | present as prepared snapshots. |
| Frame layout, prologue, and epilogue facts | Function bodies and stack-using operations. | `PreparedFramePlan`, `PreparedSavedRegister`. | `FrameRecord`, `CalleeSaveRecord`. | `module/module.hpp` snapshots; later `abi/` owns AAPCS64 prologue/epilogue policy. | present for frame facts; AAPCS64 prologue/epilogue lowering policy deferred. |
| Dynamic stack | BIR dynamic alloca/save/restore operations. | `PreparedDynamicStackPlan`, `PreparedDynamicStackOp`. | `DynamicStackRecord` inside `FrameRecord`. | `module/module.hpp`. | present as prepared snapshots; target operation selection remains later work. |
| Branches and compares | BIR terminators and compare values. | `PreparedControlFlow`, `PreparedBranchCondition`, `PreparedBranchTargetLabels`. | `BranchRecord`, `BlockRecord`. | `module/module.hpp` snapshots; later `codegen/` owns lowering. | present for structured branch facts; no target branch instruction record yet. |
| Join transfers and parallel copies | BIR phi/select/edge value obligations. | `PreparedJoinTransfer`, `PreparedParallelCopyBundle`, `PreparedMoveBundle`, `PreparedMoveResolution`. | `ParallelCopyRecord`, `MoveRecord`. | `module/module.hpp`. | present as prepared snapshots; no local phi-text recovery is accepted. |
| Calls and indirect calls | BIR call instructions and callee values. | `PreparedCallPlans`, `PreparedCallPlan`, `PreparedIndirectCalleePlan`, `PreparedCallArgumentPlan`, `PreparedCallResultPlan`, `PreparedCallPreservedValue`, `PreparedClobberedRegister`. | `CallRecord`, `CallArgumentRecord`, `CallResultRecord`, `CallPreservedValueRecord`, `CalleeSaveRecord`. | `module/module.hpp` snapshots; later `abi/` owns AAPCS64 call-frame policy. | prepared carrier present, but AAPCS64 completeness is ambiguous for all direct, indirect, memory-return, preserved, clobbered, and variadic cases. |
| Returns | BIR return terminators and return values. | `PreparedMoveBundle` with `PreparedMovePhase::BeforeReturn`; call/result storage plans where returns share ABI movement facts. | `BlockRecord` terminator fields plus `AbiBindingRecord`/`MoveRecord`; no dedicated return record. | Later `abi/` contract with `module/module.hpp` snapshots. | ambiguous; return-value placement and AAPCS64 completeness need target ABI design before lowering. |
| Variadic call metadata | BIR extern/call shape. | `PreparedCallPlan::wrapper_kind` and `variadic_fpr_arg_register_count`. | `CallRecord` carries wrapper kind; no final AAPCS64 variadic-lowering record. | Later `abi/` contract. | ambiguous; known variadic metadata exists, but completeness for AAPCS64 call lowering is not accepted. |
| Memory and addressing | BIR load/store/global/string/pointer operations and BIR types. | `PreparedAddressing`, `PreparedAddressingFunction`, `PreparedMemoryAccess`, `PreparedAddress`. | Address fields on `OperandRecord`; dedicated AArch64 memory operand record is still missing. | `module/module.hpp` snapshots; shared preparation owns missing carrier facts. | present for base/offset/size/alignment; missing volatility and address-space; no target workaround from printed BIR. |
| Globals, strings, data, symbols | `bir::Module` globals, string constants, link names, initializers. | `PreparedNameTables` link/text names plus retained BIR data. | `GlobalDataRecord`, `StringDataRecord`, `DataRelocationNeedRecord`, `SymbolVisibilityRecordKind`, `DataRelocationNeedKind`. | `module/module.hpp`; later object/binary-utils contract owns emission. | present as data snapshots; object relocation and encoding behavior deferred. |
| Moves, ABI bindings, spills, reloads | BIR value movement obligations after preparation. | `PreparedValueLocations`, `PreparedMoveBundle`, `PreparedAbiBinding`, `PreparedRegalloc`, `PreparedSpillReloadOp`, `PreparedStoragePlans`. | `MoveRecord`, `AbiBindingRecord`, `SpillReloadRecord`, `ParallelCopyRecord`. | `module/module.hpp`. | present as prepared snapshots; final instruction forms remain later work. |
| Scalar integer operations and casts | Retained BIR instructions, operands, opcodes, and `TypeKind`. | `PreparedBirModule::module`; prepared value/storage/location facts. | missing target instruction/operation record. | Later `codegen/` after target MIR records. | missing target record; legacy `codegen/*.md` text patterns are not accepted records. |
| Floating-point scalar operations | Retained BIR FP instructions and `TypeKind`. | `PreparedBirModule::module`; prepared value/storage/location facts. | missing target FP instruction/operation record. | Later `codegen/` after target MIR records. | missing target record; lowering deferred until target instruction model exists. |
| F128 and i128 operations | Retained BIR type/op facts where present. | No accepted complete prepared support policy for this contract. | deferred; no target-local record accepted. | Separate f128/i128 support policy idea. | deferred; do not add partial lowering under this plan. |
| Atomics | Retained BIR atomic-like semantics if/when present. | No accepted ordering and target-lowering carrier for this contract. | deferred; no target-local atomic record accepted. | Separate atomics lowering idea. | deferred; needs ABI and memory-order carrier review before any AArch64 lowering. |
| Intrinsics | Retained BIR intrinsic-like operations if/when present. | No accepted intrinsic inventory carrier for this contract. | deferred; no target-local intrinsic record accepted. | Separate intrinsic inventory/lowering idea. | deferred; inventory and lowering contract are outside this layout plan. |
| Inline assembly | BIR inline-asm instruction payload where present. | `stack_layout::FunctionInlineAsmSummary` only summarizes count/side effects for stack planning. | deferred; no structured inline-asm operand/clobber record accepted. | Separate structured inline-asm contract. | deferred; legacy string substitution and parser recovery are rejected. |
| Assembler parser and directives | No backend semantic BIR carrier; assembly text is post-MIR syntax. | No prepared semantic carrier. | deferred assembler request/operand records. | Deferred assembler contract. | deferred; parser operands cannot supply backend semantics. |
| Encoder families | No backend semantic BIR carrier beyond future target instructions. | No prepared encoder carrier. | deferred structured instruction/encoding records. | Deferred assembler/encoder contract. | deferred until target instruction records exist. |
| ELF object writer | BIR data can describe globals/strings/symbol references before object emission. | Prepared names plus retained BIR data only. | deferred object/section/symbol/relocation records. | Deferred object/binary-utils contract. | deferred; no current AArch64 object writer behavior is owned here. |
| Binary utilities | No target MIR or BIR semantic carrier selected for utility behavior. | No prepared utility carrier. | deferred shared binary-utils records. | Deferred shared binary-utils contract. | deferred until object/relocation scope is selected. |
| Linker input, symbols, relocations, PLT/GOT, static/dynamic/shared emission | BIR link names and data relocation needs only. | `PreparedNameTables` plus retained BIR data; no linker orchestration carrier. | deferred linker records. | Separate linker initiative. | deferred; no AArch64 backend dependency in this plan. |
| Invariants and completed prepare phases | BIR state after shared preparation. | `PreparedBirModule::invariants`, `PreparedBirModule::completed_phases`. | Handoff validation may inspect before building `Module`. | `api/`/`abi/` entry validation. | present as gate carriers; required phase policy still belongs in the entry contract. |

## Required Prepared Facts

| Contract fact | Current carrier | Status | Gap owner | Ledger note |
| --- | --- | --- | --- | --- |
| Accepted entry type is `PreparedBirModule`, not raw `bir::Module`. | Shared prepare exposes `prepare_semantic_bir_module_with_options(...)`; x86 has `emit_prepared_module(const PreparedBirModule&)`. AArch64 still exposes raw BIR/LIR emitters. | missing | AArch64 MIR design | Add an AArch64 prepared-module handoff before target lowering. Raw BIR may remain only as upstream staging into preparation. |
| Semantic module structure. | `PreparedBirModule::module` retains BIR functions, blocks, instructions, globals, strings, and name tables. | present | shared preparation | AArch64 may read semantic operations from the retained module after prepared facts have been accepted. |
| Target profile and AAPCS64 target ABI. | `PreparedBirModule::target_profile` includes `TargetArch::Aarch64` and `BackendAbiKind::Aapcs64` when triples resolve through `target_profile_from_triple(...)`. | present | shared preparation | Target entry must reject non-AArch64 prepared modules and require `backend_abi == Aapcs64` for AAPCS64 routes. |
| Prepared intern tables for names and symbols. | `PreparedBirModule::names` carries `function_names`, `block_labels`, `value_names`, `slot_names`, `link_names`, and `texts`. | present | shared preparation | Use interned ids as lookup authority; display spellings are final spelling or diagnostics only. |
| Control-flow blocks and branch targets. | `PreparedBirModule::control_flow.functions[].blocks` records block labels and branch target labels. | present | shared preparation | Target MIR blocks should be keyed by `FunctionNameId` / `BlockLabelId`. |
| Branch conditions. | `PreparedBranchCondition` records materialized or fused compare branches, predicate/type/lhs/rhs, and true/false labels. | present | shared preparation | AArch64 compare/branch lowering can consume this instead of reconstructing from rendered labels. |
| Join transfers and former phi obligations. | `PreparedJoinTransfer` and `PreparedParallelCopyBundle` record phi/select/edge-copy obligations and execution sites. | present | shared preparation | AArch64 lowering must emit move/copy records from these prepared bundles, not re-read phi text. |
| Stack objects and frame slots. | `PreparedStackLayout::objects` and `frame_slots` record object ids, slot ids, sizes, alignments, offsets, and exposure/home requirements. | present | shared preparation | Target frame lowering can consume ids and offsets directly. |
| Frame plan. | `PreparedFramePlan` records frame size/alignment, saved callee registers, frame-slot order, dynamic stack use, and fixed-slot frame-pointer policy. | present | shared preparation | AArch64 must map prepared saved register strings/banks into target physical register references. |
| Dynamic stack plan. | `PreparedDynamicStackPlan` records stack-save, dynamic-alloca, and stack-restore operations with function/block/instruction identity. | present | shared preparation | AArch64 target MIR needs dynamic-stack operations before prologue/epilogue and instruction selection. |
| Address bases for memory access. | `PreparedAddressing` and `PreparedAddress` carry frame-slot, global-symbol, pointer-value, and string-constant base kinds. | present | shared preparation | AArch64 memory operands can be selected from prepared base kind plus offset/size/alignment. |
| Volatility and address-space facts for memory access. | `PreparedMemoryAccess` carries identity and `PreparedAddress`, but no explicit volatile or address-space fields are visible in the prepared access. | missing | shared preparation | Split or extend prepared addressing if AArch64 lowering must distinguish volatile or non-default address spaces. Do not recover these from printed BIR. |
| Liveness. | `PreparedLiveness` records per-function intervals, call points, block live-in/live-out, and value records. | present | shared preparation | May inform target-local validation and late insertion policy. |
| Register group overrides. | `PreparedRegisterGroupOverrides` records per-function/value register class and contiguous width. | present | shared preparation | Target MIR must translate class/width into AArch64 register-class operands. |
| Register allocation. | `PreparedRegalloc` records allocation constraints, interference, move resolution, spill/reload operations, and assigned register/stack homes. | present | shared preparation | AArch64 must consume prepared allocation instead of choosing caller/callee policy locally. |
| Value locations. | `PreparedValueLocations` records value homes and move bundles keyed by prepared value/function/block/instruction identity. | present | shared preparation | Target MIR should preserve `PreparedValueId` and `ValueNameId` in operands. |
| Storage plans. | `PreparedStoragePlans` records register, frame-slot, immediate, computed-address, and symbol-address encodings. | present | shared preparation | AArch64 needs a target-local operand layer that turns encodings into instruction operands. |
| Call plans for direct, extern, variadic, indirect, memory return, arguments, results, preserved values, and clobbers. | `PreparedCallPlans` records wrapper kind, direct name, indirect callee, memory return, arguments, result, preserved values, clobbers, and variadic FP-register count. | present | shared preparation | AArch64 lowering may consume this as the call fact carrier. |
| AAPCS64-specific call/register semantics are complete enough for every call shape. | Prepared calls carry generic banks/register strings and storage facts, but no AArch64 MIR call-lowering contract consumes or validates all AAPCS64 cases yet. | ambiguous | target ABI work | Step 6 should decide whether the next idea is AArch64 target ABI/call-frame design before instruction selection. |
| Invariants and completed prepare phases. | `PreparedBirModule::invariants` and `completed_phases` record `NoTargetFacingI1`, `NoPhiNodes`, and phase completion. | present | shared preparation | AArch64 entry must gate lowering on required invariants/phases being present. |

## Target-Local MIR And Assembly Structures

| Required target-local structure | Current AArch64 availability | Status | Gap owner | Ledger note |
| --- | --- | --- | --- | --- |
| AArch64 module/function/block MIR container keyed back to prepared ids. | No live AArch64 MIR container exists. Current headers expose text emitters and staged assembler/linker contracts. | missing | AArch64 MIR design | Define a target-local MIR module before codegen implementation resumes. |
| Typed virtual or prepared-value operands retaining `PreparedValueId`, `ValueNameId`, and `TypeKind`. | No AArch64 operand model exists. | missing | AArch64 MIR design | Do not use rendered value names or old emitter register strings as semantic operands. |
| Target register classes and physical register references separated from semantic value ids. | Prepared records carry register classes/banks and physical register strings; AArch64 has no typed target register reference layer. | missing | AArch64 MIR design | Add AArch64 register enum/class references or an equivalent typed target register surface. |
| Frame, stack-slot, dynamic-stack, and callee-save MIR records sourced from prepared plans. | Prepared facts exist; AArch64 target records do not. | missing | AArch64 MIR design | MIR should copy ids/references from prepared plans, not recompute frame placement. |
| Memory operands preserving base kind, frame-slot id, symbol `LinkNameId`, pointer value id/name, string identity, offset, size, alignment, volatility, and address space. | Prepared memory facts mostly exist, but volatility/address-space facts are missing and no AArch64 memory operand model exists. | missing | shared preparation | Carrier extension is required for volatility/address-space before full memory lowering; target MIR must then preserve those facts. |
| Branch and compare records sourced from prepared branch conditions and `BlockLabelId`. | Prepared branch facts exist; no AArch64 branch/compare MIR records exist. | missing | AArch64 MIR design | Lower compare/branch from `PreparedBranchCondition` into typed AArch64 MIR. |
| Call records sourced from `call_plans`. | Prepared call plans exist; no AArch64 target call record model exists. | missing | target ABI work | AArch64 ABI design must map prepared call plans into target MIR call records. |
| Move, copy, spill, reload, and ABI-binding records sourced from prepared value/regalloc/storage plans and parallel copies. | Prepared move/storage/regalloc facts exist; no AArch64 move/spill/reload MIR records exist. | missing | AArch64 MIR design | Target MIR needs explicit records before instruction selection. |
| Data/object side table for globals, string constants, symbol visibility, TLS, constants, initializers, and later relocation needs. | BIR module carries globals and strings. AArch64 has assembler/linker markdown and headers, but no codegen-owned data/object MIR side table. | missing | assembler/object work | Split object/relocation details behind an explicit assembler/object boundary; codegen should only publish structured data records. |
| Assembly generation boundary after MIR or explicit assembler/object contract. | AArch64 assembler headers are text-first (`AssembleRequest::asm_text`) and object emission is staged, not implemented. | deferred | assembler/object work | Assembly text emission must not be the first rebuilt lowering API. Add only after MIR exists or an assembler/object boundary is accepted. |
| Built-in object emission and linking. | AArch64 assembler/linker contract surfaces exist, but object emission and linking are outside the backend entry contract. | deferred | assembler/object work | Keep this outside Step 6 unless the supervisor selects a separate binary-utils or linker idea. |

## Deferred Or Separate-Idea Items

| Item | Status | Owner | Reason |
| --- | --- | --- | --- |
| Volatile and non-default address-space preservation in prepared memory facts. | missing | shared preparation | The Step 4 contract requires these facts for memory operands, but the visible prepared memory access carrier does not expose them. |
| Inline assembly operand substitution and clobber policy. | deferred | separate idea | Current markdown classifies old inline-asm routes as obsolete for this contract; rebuild requires an explicit structured inline-asm contract. |
| NEON and broad vector instruction coverage. | deferred | separate idea | Classification marks NEON as delete/defer; do not expand Step 6 around it without a new source idea. |
| F128 and i128 special lowering. | deferred | separate idea | Legacy notes are delete/defer or niche support risks, not entry-contract blockers. |
| Local assembler parser operand recovery. | deferred | assembler/object work | Parser facts may inform later assembler work, but must not substitute for target MIR operands. |
| Built-in linker orchestration. | deferred | separate idea | Linker work is explicitly outside the backend entry boundary. |

## Final Contract Validation

This ledger is the responsibility-boundary output for
`ideas/open/205_aarch64_arm_reference_layout_contract.md`. It covers the
required feature families from that source idea: public prepared-module entry,
target ABI and AAPCS64 facts, module/function/block containers, operands and
value identity, registers and register classes, frame/prologue/epilogue and
callee-save records, branch/compare records, call/return/variadic records,
memory/addressing records, globals/string/data records, moves/spills/reloads
and parallel copies, scalar ALU/casts/float ops, atomics, intrinsics, inline
asm, f128/i128, assembler, encoder, object writer, binary utilities, and
linker surfaces.

The only required shared BIR/prepared carrier gap discovered by this contract
is volatile and non-default address-space preservation in
`PreparedMemoryAccess` / `PreparedAddress`. That gap is split into
`ideas/open/206_prepared_memory_volatility_address_space_carrier.md`; AArch64
target code must not work around it by recovering volatility or address-space
facts from rendered names, printed BIR, legacy examples, or assembly text.

No other required BIR/prepared carrier gap blocks this layout contract. The
remaining non-present items are either target-local AArch64 MIR records that
belong to the next AArch64 design wave or deferred/ambiguous contracts already
named here: AAPCS64 call/return/variadic completeness, target instruction
records for scalar and floating-point lowering, f128/i128, atomics, intrinsics,
inline asm, assembler/encoder, object/binary-utils, and linker behavior.

## Proceed Versus Split Decision

Backend implementation must not proceed directly to AArch64 instruction
selection or assembly text emission.

Proceed is allowed only for the next planning action: select or create Step 6
work that defines the AArch64 prepared-module target MIR boundary. The main
blocking gap is target-local: no AArch64 MIR module, operand, register, frame,
memory, branch, call, move, spill/reload, or data/object side-table model
currently exists to consume `PreparedBirModule`.

A separate shared BIR/prepared carrier initiative is not required before Step 6
for most facts because `PreparedBirModule` already carries the contract's core
module, identity, control-flow, frame, stack, liveness, regalloc, storage, and
call plans. The one required shared carrier initiative is already recorded as
`ideas/open/206_prepared_memory_volatility_address_space_carrier.md`. If the
next wave chooses memory lowering as its first implementation slice, that gap
idea is a prerequisite; otherwise proceed with AArch64 MIR design around the
present prepared facts and keep full memory lowering blocked.

No target-local workaround is accepted for missing facts. In particular, do not
recover backend facts from rendered names, printed BIR, legacy LIR type
strings, old AArch64 markdown examples, assembly-string parsing, parser operand
recovery, or legacy shape recognizers.
