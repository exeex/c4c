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
- Allocation resource contract:
  `src/backend/mir/aarch64/ALLOCATION_CONTRACT.md`.
- AAPCS64 call/return/frame contract:
  `src/backend/mir/aarch64/AAPCS64_CALL_RETURN_FRAME_CONTRACT.md`.
- Responsibility/classification index:
  `src/backend/mir/aarch64/CLASSIFICATION_INDEX.md`.
- Shared prepared carrier: `src/backend/prealloc/prealloc.hpp`
  `PreparedBirModule`.
- Shared public backend staging: `src/backend/backend.hpp` and
  `src/backend/backend.cpp`.
- Existing target-local precedent: x86 consumes `PreparedBirModule` through
  `src/backend/mir/x86/api/api.hpp`, `src/backend/mir/x86/module/module.hpp`,
  `src/backend/mir/x86/prepared/prepared.hpp`, and route-debug surfaces.
- Current AArch64 codegen enters through
  `src/backend/mir/aarch64/codegen/emit.hpp` as
  `build_module(const PreparedBirModule&)`, then splits orchestration,
  traversal, instruction/operand construction, feature lowering, returns, and
  compatibility projection across the current `codegen/` `.cpp/.hpp` files.
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
| AAPCS64 ABI call/return/variadic policy | `AAPCS64_CALL_RETURN_FRAME_CONTRACT.md` plus `module/` snapshots | Present minimum contract over prepared call, return, variadic, memory-return, frame, and special-register facts; specific missing carriers remain deferred as named contract candidates. | Existing snapshots: `CallRecord`, `CallArgumentRecord`, `CallResultRecord`, `CallPreservedValueRecord`, `CalleeSaveRecord`, `AbiBindingRecord`, `MoveRecord`, `BlockRecord`, `FrameRecord`. | Consume the AAPCS64 contract before any call, return, variadic, frame, prologue, or epilogue instruction-selection route. |
| Module/function/block containers | `module/module.hpp`, `module/module.cpp` | Present BIR and prepared carriers: retained BIR module plus `PreparedControlFlow`. | `Module`, `FunctionRecord`, `BlockRecord`. | AArch64 prepared-module target MIR boundary; containers stay module-owned. |
| Semantic ids and name tables | `module/module.hpp`, with lookup authority from shared preparation | Present prepared carrier: `PreparedNameTables` and BIR ids. | Stored ids/labels inside `Module`, `FunctionRecord`, `BlockRecord`, `OperandRecord`, data records. | Target MIR boundary; labels are display-only, not semantic recovery. |
| Operand/value identity | `module/module.hpp` | Present prepared carriers: `PreparedValueLocations`, `PreparedStoragePlans`, retained BIR value/type facts. | `OperandRecord`. | Target MIR operand model; no rendered-name or old register-string recovery. |
| Register banks/classes and physical references | `module/module.hpp` for snapshots; `ALLOCATION_CONTRACT.md` for allocation-result ownership; later `abi/` or `codegen/` may define typed AArch64 registers | Present prepared carrier for banks/classes/assignments; target-specific typed register enum remains missing. | `TargetRegisterRecord` plus `CalleeSaveRecord` register fields. | Target MIR/register model idea before instruction selection; no feature slice may allocate homes locally. |
| Frame layout, stack slots, prologue, and epilogue facts | `module/module.hpp` owns prepared snapshots; `AAPCS64_CALL_RETURN_FRAME_CONTRACT.md` owns AAPCS64 frame/prologue/epilogue policy boundaries | Present prepared carriers: `PreparedStackLayout`, `PreparedFramePlan`, saved-register plan. | `FrameRecord`, `FrameSlotRecord`, `CalleeSaveRecord`, `DynamicStackRecord`. | Later prologue/epilogue machine-node work must consume the AAPCS64 contract and frame records; it must not recompute frame layout or save/restore policy locally. |
| Dynamic stack | `module/module.hpp` | Present prepared carrier: `PreparedDynamicStackPlan`. | `DynamicStackRecord` inside `FrameRecord`. | Target MIR dynamic-stack operation design. |
| Branches, compares, block transfers | `module/module.hpp`; later `codegen/` owns lowering rules | Present prepared carriers: `PreparedControlFlow`, `PreparedBranchCondition`, `PreparedJoinTransfer`, branch labels. | `BranchRecord`, `BlockRecord`, `ParallelCopyRecord`. | Target MIR branch/compare model; instruction selection comes later. |
| Join copies and parallel copies | `module/module.hpp` | Present prepared carriers: `PreparedParallelCopyBundle`, `PreparedMoveBundle`, `PreparedJoinTransfer`. | `ParallelCopyRecord`, `MoveRecord`. | Target MIR move/copy design over prepared bundles. |
| Calls and indirect calls | `module/module.hpp` snapshots; `ALLOCATION_CONTRACT.md` for call-preservation resources; `AAPCS64_CALL_RETURN_FRAME_CONTRACT.md` for AAPCS64 call-boundary policy | Present prepared carrier: `PreparedCallPlans`; accepted target-MIR contract now covers direct, indirect, memory-return, preserved, clobbered, and variadic minimum facts. | `CallRecord`, `CallArgumentRecord`, `CallResultRecord`, `CallPreservedValueRecord`, `AbiBindingRecord`, `MoveRecord`. | Later call machine-node work must consume prepared call records and the AAPCS64 contract; it must not choose argument/result registers, stack arguments, indirect-call scratch, or call-preservation resources locally. |
| Returns | `AAPCS64_CALL_RETURN_FRAME_CONTRACT.md` for AAPCS64 policy, with `module/` return/move/ABI-binding snapshots | BIR return terminators are present; prepared `BeforeReturn` move and ABI-binding facts exist; a dedicated return-boundary carrier remains deferred. | Existing `BlockRecord` terminator fields plus `MoveRecord`/`AbiBindingRecord`; no dedicated return record. | Later return-control machine-node work must consume the AAPCS64 contract; it must not assign return registers, link-register effects, or epilogue behavior locally. |
| Memory and addressing | `module/module.hpp` for current snapshots; split `codegen/operands.*`, `codegen/instruction.*`, and later feature-specific memory lowering for target-MIR memory operands and selected memory machine nodes; shared preparation owns typed memory facts | Prepared addressing exists for base/offset/size/alignment, with `PreparedMemoryAccess::address_space` and `PreparedMemoryAccess::is_volatile` carrying the shared address-space and volatility facts. | `OperandRecord` address fields plus memory operands/instruction records for the current target-MIR layer; selected memory machine nodes for the accepted load/store subset. | Memory instruction-selection and addressing-legality design; preserve the typed prepared fields instead of recovering facts from text. |
| Globals, strings, data, symbols, relocation needs | `module/module.hpp`; later object/binary-utils contract owns encoding/emission | Present BIR carriers for globals/strings and link names; object relocation semantics deferred. | `GlobalDataRecord`, `StringDataRecord`, `DataRelocationNeedRecord`, `SymbolVisibilityRecordKind`, `DataRelocationNeedKind`. | Structured data/object boundary idea; no object writer behavior in this plan. |
| Moves, ABI bindings, spills, reloads | `module/module.hpp` snapshots plus `ALLOCATION_CONTRACT.md` for spill-slot ids and reserved MIR scratch policy | Present prepared carriers: regalloc, value locations, storage plans, move bundles, spill/reload ops. | `MoveRecord`, `AbiBindingRecord`, `SpillReloadRecord`, `ParallelCopyRecord`. | Target MIR move/spill/reload design; do not create rendered stack-address spills or steal scratch in local lowering. |
| Scalar integer operations and casts | `codegen/alu.*`, `codegen/comparison.*`, `codegen/returns.*`, `codegen/instruction.*`, and `codegen/dispatch.*` over target MIR | BIR operation/type facts are present in retained module, target-MIR scalar ALU/cast records cover the accepted subset, and selected machine-node records now carry structured opcode identity for that subset. | `ScalarInstructionRecord`, `ScalarAluRecord`, `ScalarCastRecord`, and selected scalar machine-node records for the current subset; broader opcode families and final printer/encoder behavior remain later. | Scalar instruction-selection idea over structured records; do not use legacy `codegen/*.md` text patterns as records. |
| Floating-point scalar operations | Later `codegen/` after target MIR records | BIR operation/type facts are present for supported types; current scalar records do not yet own FP lowering. | Missing FP-specific structured machine instruction node. | FP lowering idea over prepared value/location records and allocation-result facts. |
| F128 and i128 special-width operations | Deferred contract | Support policy and complete carriers are deferred; legacy notes are `delete/defer`. | Deferred; no target-local record accepted in this plan. | Separate f128/i128 support policy idea before any lowering. |
| Atomics | Deferred target ABI/codegen contract | BIR/prepare authority for ordering and target lowering completeness is not accepted here. | Deferred; no target-local atomic record accepted in this plan. | Separate atomics lowering idea with ABI and memory-order carrier review. |
| Intrinsics | Deferred codegen contract | Intrinsic inventory and carriers are not accepted as part of this layout contract. | Deferred; no target-local intrinsic record accepted in this plan. | Separate intrinsic inventory/lowering idea. |
| Inline assembly | Deferred structured inline-asm contract | Legacy string-substitution route is obsolete; no structured operand/clobber contract accepted. | Deferred; no target-local inline-asm record accepted in this plan. | Separate inline-asm contract idea; no parser/string recovery. |
| Assembler parser and directives | Deferred assembler contract | Legacy parser exists only as a classified markdown candidate; parser operands cannot supply backend semantics. | Deferred assembler request/operand records, not current MIR records. | Explicit assembler-layer rebuild idea after target MIR and machine-node records. |
| Encoder families | Deferred assembler/encoder contract | Legacy encoder facts may be salvageable later; no codegen dependency is accepted now. | Deferred structured machine-instruction-node or lower encoding records. | Encoder API rebuild over machine instruction nodes or derived structured encoding records. |
| ELF object writer | Deferred object/binary-utils contract | Legacy object-writer notes are binary-utils candidates; no current AArch64 object writer behavior is owned here. | Deferred object/section/symbol/relocation records derived from structured nodes or encoding records. | Shared binary-utils/object writer idea. |
| Binary utilities | Deferred shared binary-utils contract | Useful only after object/relocation scope is selected. | Deferred shared records, not target MIR records. | Shared binary-utils contract idea. |
| Linker input, symbols, relocations, PLT/GOT, static/dynamic/shared emission | Deferred linker contract | Legacy linker notes are not backend output requirements. | Deferred linker records. | Separate linker initiative; no AArch64 backend dependency. |

Current `module/module.hpp` records remain module-owned when they are snapshots
of accepted BIR or `PreparedBirModule` facts: module/function/block identity,
operands, frame snapshots, branches, calls, moves, ABI bindings,
spills/reloads, parallel copies, globals, strings, and relocation needs. They
do not own final instruction selection, assembler encoding, object emission,
binary utility behavior, or linker orchestration. AAPCS64 call/return/frame
policy is owned by `AAPCS64_CALL_RETURN_FRAME_CONTRACT.md`; the other families
require later `codegen/`, `assembler/`, object/binary-utils, linker, or
separate deferred contracts before behavior changes.

## BIR / Prepared Carrier Checklist

This checklist expands the ownership ledger into a reviewer-readable carrier
map. "BIR carrier" names the semantic source when the fact remains in retained
BIR. "Prepared carrier" names the structured preparation surface when it exists.
"Target record" names the AArch64 snapshot or the explicitly missing/deferred
record. The status column is the current permission boundary for later work.

| Feature family | BIR carrier | Prepared carrier | Target record | Owner | Status |
| --- | --- | --- | --- | --- | --- |
| Public prepared-module entry | Retained `bir::Module` only as input to preparation; raw BIR emitters are not accepted entry carriers. | `PreparedBirModule` passed through `build_prepared_module(const PreparedBirModule&)`. | `module::BuildResult`, `module::Module`. | `api/api.hpp`, `api/api.cpp`. | present for prepared entry; raw BIR emitters, LIR text, and assembly text fallback are rejected for this route. |
| Target profile and handoff validation | Target triple facts before preparation. | `PreparedBirModule::target_profile`. | `abi::HandoffError`, `abi::HandoffErrorKind`. | `abi/abi.hpp`, `abi/abi.cpp`. | present for AArch64/AAPCS64 validation; lowering must still gate on this. |
| Module containers | `bir::Module`, `bir::Function`, `bir::Block`. | `PreparedBirModule::module`, `PreparedControlFlow`. | `Module`, `FunctionRecord`, `BlockRecord`. | `module/module.hpp`. | present as snapshots; container ownership does not imply instruction selection. |
| Semantic ids and names | BIR function, block, value, slot, link, and text ids. | `PreparedNameTables`. | Id and label fields inside module, function, block, operand, data, and relocation records. | `module/module.hpp`, with lookup authority from shared preparation. | present; labels are display or diagnostics only, not semantic recovery. |
| Operand/value identity | `bir::Value`, BIR type facts, value names. | `PreparedValueLocations`, `PreparedRegalloc`, `PreparedStoragePlans`, retained BIR values. | `OperandRecord`. | `module/module.hpp`. | present as snapshots; no rendered-name or old register-string recovery. |
| Register banks/classes and physical references | Type and value identity in BIR. | `PreparedRegisterGroupOverrides`, `PreparedRegalloc`, `PreparedStoragePlans`, `PreparedSavedRegister`. | `TargetRegisterRecord`, `CalleeSaveRecord` register fields. | `module/module.hpp` snapshots; later `abi/` or `codegen/` typed-register contract. | present for prepared banks/classes and string physical references; missing final typed AArch64 register enum/model. |
| Stack objects and frame slots | Stack object and allocation-driving BIR facts. | `PreparedStackLayout`, `PreparedFrameSlot`. | `FrameSlotRecord`, fields in `FrameRecord`. | `module/module.hpp`. | present as prepared snapshots. |
| Frame layout, prologue, and epilogue facts | Function bodies and stack-using operations. | `PreparedFramePlan`, `PreparedSavedRegister`, `PreparedDynamicStackPlan`. | `FrameRecord`, `CalleeSaveRecord`, `DynamicStackRecord`. | `module/module.hpp` snapshots; `AAPCS64_CALL_RETURN_FRAME_CONTRACT.md` owns AAPCS64 prologue/epilogue boundaries. | present for the accepted target-MIR contract; final save/restore, stack-adjust, frame-pointer setup, and epilogue instruction selection are deferred. |
| Dynamic stack | BIR dynamic alloca/save/restore operations. | `PreparedDynamicStackPlan`, `PreparedDynamicStackOp`. | `DynamicStackRecord` inside `FrameRecord`. | `module/module.hpp`. | present as prepared snapshots; target operation selection remains later work. |
| Branches and compares | BIR terminators and compare values. | `PreparedControlFlow`, `PreparedBranchCondition`, `PreparedBranchTargetLabels`. | `BranchRecord`, `BlockRecord`. | `module/module.hpp` snapshots; later `codegen/` owns lowering. | present for structured branch facts; no target branch instruction record yet. |
| Join transfers and parallel copies | BIR phi/select/edge value obligations. | `PreparedJoinTransfer`, `PreparedParallelCopyBundle`, `PreparedMoveBundle`, `PreparedMoveResolution`. | `ParallelCopyRecord`, `MoveRecord`. | `module/module.hpp`. | present as prepared snapshots; no local phi-text recovery is accepted. |
| Calls and indirect calls | BIR call instructions and callee values. | `PreparedCallPlans`, `PreparedCallPlan`, `PreparedIndirectCalleePlan`, `PreparedCallArgumentPlan`, `PreparedCallResultPlan`, `PreparedMemoryReturnPlan`, `PreparedCallPreservedValue`, `PreparedClobberedRegister`. | `CallRecord`, `CallArgumentRecord`, `CallResultRecord`, `CallPreservedValueRecord`, `CalleeSaveRecord`, `AbiBindingRecord`, `MoveRecord`. | `module/module.hpp` snapshots; `AAPCS64_CALL_RETURN_FRAME_CONTRACT.md` owns AAPCS64 call-frame policy. | present for the accepted target-MIR contract; direct `CallRecord` variadic-count and outgoing-area carriers remain deferred if later consumers require them. |
| Returns | BIR return terminators and return values. | `PreparedMoveBundle` with `PreparedMovePhase::BeforeReturn`; call/result storage plans where returns share ABI movement facts. | `BlockRecord` terminator fields plus `AbiBindingRecord`/`MoveRecord`; no dedicated return record. | `AAPCS64_CALL_RETURN_FRAME_CONTRACT.md` with `module/module.hpp` snapshots. | present for the accepted target-MIR contract; dedicated return-control and `x30` side-effect records remain deferred candidates. |
| Variadic call metadata | BIR extern/call shape. | `PreparedCallPlan::wrapper_kind` and `variadic_fpr_arg_register_count`. | `CallRecord` carries wrapper kind and retains `source_call`; no full AAPCS64 variadic function-entry record. | `AAPCS64_CALL_RETURN_FRAME_CONTRACT.md`. | present for variadic call-boundary minimums; full `va_list`/register-save-area function-entry carriers remain deferred. |
| Memory and addressing | BIR load/store/global/string/pointer operations and BIR types. | `PreparedAddressing`, `PreparedAddressingFunction`, `PreparedMemoryAccess`, `PreparedAddress`; `PreparedMemoryAccess::address_space` and `PreparedMemoryAccess::is_volatile` carry the typed shared address-space and volatility facts. | Address fields on `OperandRecord`; memory operands/instruction records for the target-MIR layer and selected memory machine nodes for the accepted load/store subset. | `module/module.hpp` snapshots plus split `codegen/operands.*` and `codegen/instruction.*`; shared preparation owns typed carrier facts. | present for base/offset/size/alignment, volatility, and address space; no target workaround from printed BIR. |
| Globals, strings, data, symbols | `bir::Module` globals, string constants, link names, initializers. | `PreparedNameTables` link/text names plus retained BIR data. | `GlobalDataRecord`, `StringDataRecord`, `DataRelocationNeedRecord`, `SymbolVisibilityRecordKind`, `DataRelocationNeedKind`. | `module/module.hpp`; later object/binary-utils contract owns emission. | present as data snapshots; object relocation and encoding behavior deferred. |
| Moves, ABI bindings, spills, reloads | BIR value movement obligations after preparation. | `PreparedValueLocations`, `PreparedMoveBundle`, `PreparedAbiBinding`, `PreparedRegalloc`, `PreparedSpillReloadOp`, `PreparedStoragePlans`. | `MoveRecord`, `AbiBindingRecord`, `SpillReloadRecord`, `ParallelCopyRecord`. | `module/module.hpp`. | present as prepared snapshots; final instruction forms remain later work. |
| Allocation result and reserved MIR scratch policy | BIR value identity plus target-facing prepared value/storage/regalloc facts. | `PreparedValueLocations`, `PreparedRegalloc`, `PreparedStoragePlans`, `PreparedSpillReloadOp`, `PreparedCallPlans`. | Allocation-result locations, structured spill-slot ids, reserved scratch records, and future virtual-register placeholders as defined by `ALLOCATION_CONTRACT.md`. | Allocation contract consumed by target MIR. | present as a contract boundary; scalar, memory, branch, call, return, vector, inline-asm, and prologue work must consume it instead of allocating registers or inventing spills locally. |
| Scalar integer operations and casts | Retained BIR instructions, operands, opcodes, and `TypeKind`. | `PreparedBirModule::module`; prepared value/storage/location facts. | `ScalarInstructionRecord`, `ScalarAluRecord`, `ScalarCastRecord`, and selected scalar machine-node records for the accepted integer ALU/cast subset. | Split `codegen/alu.*`, `codegen/comparison.*`, `codegen/returns.*`, `codegen/instruction.*`, and `codegen/dispatch.*` after target MIR records. | present for the accepted structured scalar subset; broader scalar/floating opcode families, printer output, and encoder/object behavior remain deferred. |
| Floating-point scalar operations | Retained BIR FP instructions and `TypeKind`. | `PreparedBirModule::module`; prepared value/storage/location facts. | missing FP-specific selected machine instruction node. | Later `codegen/` after target MIR records. | target-MIR facts exist through retained BIR and prepared carriers; selected FP lowering remains deferred until an FP instruction model exists. |
| F128 and i128 operations | Retained BIR type/op facts where present. | No accepted complete prepared support policy for this contract. | deferred; no target-local record accepted. | Separate f128/i128 support policy idea. | deferred; do not add partial lowering under this plan. |
| Atomics | Retained BIR atomic-like semantics if/when present. | No accepted ordering and target-lowering carrier for this contract. | deferred; no target-local atomic record accepted. | Separate atomics lowering idea. | deferred; needs ABI and memory-order carrier review before any AArch64 lowering. |
| Intrinsics | Retained BIR intrinsic-like operations if/when present. | No accepted intrinsic inventory carrier for this contract. | deferred; no target-local intrinsic record accepted. | Separate intrinsic inventory/lowering idea. | deferred; inventory and lowering contract are outside this layout plan. |
| Inline assembly | BIR inline-asm instruction payload where present. | `stack_layout::FunctionInlineAsmSummary` only summarizes count/side effects for stack planning. | deferred; no structured inline-asm operand/clobber record accepted. | Separate structured inline-asm contract. | deferred; legacy string substitution and parser recovery are rejected. |
| Assembler parser and directives | No backend semantic BIR carrier; assembly text is post-MIR syntax. | No prepared semantic carrier. | deferred assembler request/operand records. | Deferred assembler contract. | deferred; parser operands cannot supply backend semantics. |
| Encoder families | No backend semantic BIR carrier beyond future target instructions. | No prepared encoder carrier. | deferred structured machine-node or encoding records. | Deferred assembler/encoder contract. | deferred until machine instruction nodes exist; parser operands are not codegen input. |
| ELF object writer | BIR data can describe globals/strings/symbol references before object emission. | Prepared names plus retained BIR data only. | deferred object/section/symbol/relocation records derived from structured nodes or encoding records. | Deferred object/binary-utils contract. | deferred; no current AArch64 object writer behavior is owned here. |
| Binary utilities | No target MIR or BIR semantic carrier selected for utility behavior. | No prepared utility carrier. | deferred shared binary-utils records. | Deferred shared binary-utils contract. | deferred until object/relocation scope is selected. |
| Linker input, symbols, relocations, PLT/GOT, static/dynamic/shared emission | BIR link names and data relocation needs only. | `PreparedNameTables` plus retained BIR data; no linker orchestration carrier. | deferred linker records. | Separate linker initiative. | deferred; no AArch64 backend dependency in this plan. |
| Invariants and completed prepare phases | BIR state after shared preparation. | `PreparedBirModule::invariants`, `PreparedBirModule::completed_phases`. | Handoff validation may inspect before building `Module`. | `api/`/`abi/` entry validation. | present as gate carriers; required phase policy still belongs in the entry contract. |

## Required Prepared Facts

| Contract fact | Current carrier | Status | Gap owner | Ledger note |
| --- | --- | --- | --- | --- |
| Accepted entry type is `PreparedBirModule`, not raw `bir::Module`. | Shared prepare exposes `prepare_semantic_bir_module_with_options(...)`; AArch64 exposes `build_prepared_module(const PreparedBirModule&)` through its prepared-module API. | present | AArch64 API/module boundary | Keep raw BIR only as upstream staging into preparation. Lowering consumers must enter through the prepared-module target record layer. |
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
| Volatility and address-space facts for memory access. | `PreparedMemoryAccess::address_space` and `PreparedMemoryAccess::is_volatile` carry the shared typed address-space and volatility facts alongside identity and `PreparedAddress`. | present | shared preparation | AArch64 lowering must preserve these fields through the target-local memory operand model. Do not recover these facts from printed BIR. |
| Liveness. | `PreparedLiveness` records per-function intervals, call points, block live-in/live-out, and value records. | present | shared preparation | May inform target-local validation and late insertion policy. |
| Register group overrides. | `PreparedRegisterGroupOverrides` records per-function/value register class and contiguous width. | present | shared preparation | Target MIR must translate class/width into AArch64 register-class operands. |
| Register allocation. | `PreparedRegalloc` records allocation constraints, interference, move resolution, spill/reload operations, and assigned register/stack homes. | present | shared preparation | AArch64 must consume prepared allocation instead of choosing caller/callee policy locally. |
| Value locations. | `PreparedValueLocations` records value homes and move bundles keyed by prepared value/function/block/instruction identity. | present | shared preparation | Target MIR should preserve `PreparedValueId` and `ValueNameId` in operands. |
| Storage plans. | `PreparedStoragePlans` records register, frame-slot, immediate, computed-address, and symbol-address encodings. | present | shared preparation | AArch64 needs a target-local operand layer that turns encodings into instruction operands. |
| Call plans for direct, extern, variadic, indirect, memory return, arguments, results, preserved values, and clobbers. | `PreparedCallPlans` records wrapper kind, direct name, indirect callee, memory return, arguments, result, preserved values, clobbers, and variadic FP-register count. | present | shared preparation | AArch64 lowering may consume this as the call fact carrier. |
| AAPCS64-specific call/register semantics are complete enough for the accepted target-MIR boundary. | Prepared calls carry generic banks/register strings and storage facts, and AArch64 records call, move, ABI-binding, preservation, memory-return, and frame snapshots. `AAPCS64_CALL_RETURN_FRAME_CONTRACT.md` defines the accepted minimum policy and names deferred carrier candidates. | present for target MIR; deferred for final machine-node lowering where named carriers are missing | AAPCS64 contract plus later machine-node/carrier ideas | Later call, return, frame, prologue, epilogue, memory-return, indirect-call, and variadic consumers must follow the AAPCS64 contract instead of selecting local ABI policy. |
| Invariants and completed prepare phases. | `PreparedBirModule::invariants` and `completed_phases` record `NoTargetFacingI1`, `NoPhiNodes`, and phase completion. | present | shared preparation | AArch64 entry must gate lowering on required invariants/phases being present. |

## Target-Local MIR And Assembly Structures

| Required target-local structure | Current AArch64 availability | Status | Gap owner | Ledger note |
| --- | --- | --- | --- | --- |
| AArch64 module/function/block MIR container keyed back to prepared ids. | `module::Module`, `FunctionRecord`, and `BlockRecord` are live prepared-module records. Legacy text emitters remain historical surfaces only. | present | AArch64 module records | Later scalar, memory, branch, call, return, and printer work must consume these containers instead of reintroducing direct text-first entry points. |
| Typed virtual or prepared-value operands retaining `PreparedValueId`, `ValueNameId`, and `TypeKind`. | `module::OperandRecord` preserves prepared value identity, value names, type facts, allocation locations, and storage-plan snapshots. Split `codegen/instruction.*` and `codegen/operands.*` define the current downstream operand record surface. | present | AArch64 module records plus split codegen instruction/operand records | Do not use rendered value names or old emitter register strings as semantic operands. Missing prepared carriers should become separate open-idea candidates, not local workarounds. |
| Target register classes and physical register references separated from semantic value ids. | `TargetRegisterRecord` records target-facing register class/bank, physical spelling, prepared value identity, allocation location, scratch authority, occupied register set, and deferred virtual placeholders. | present | AArch64 module records plus `ALLOCATION_CONTRACT.md` | Final register enum/encoding policy remains later work, but consumers must use these structured records rather than allocate homes locally. |
| Frame, stack-slot, dynamic-stack, and callee-save MIR records sourced from prepared plans. | `FrameRecord`, `FrameSlotRecord`, `DynamicStackRecord`, and `CalleeSaveRecord` preserve prepared frame and slot snapshots. | present | AArch64 module records plus `AAPCS64_CALL_RETURN_FRAME_CONTRACT.md` | Prologue/epilogue work may consume these snapshots, `ALLOCATION_CONTRACT.md`, and the AAPCS64 contract; it must not recompute frame placement, save/restore policy, or spill slots locally. |
| Memory operands preserving base kind, frame-slot id, symbol `LinkNameId`, pointer value id/name, string identity, offset, size, alignment, volatility, and address space. | Prepared memory facts exist, including `PreparedMemoryAccess::address_space` and `PreparedMemoryAccess::is_volatile`; split `codegen/instruction.*` and `codegen/operands.*` preserve these facts for the target-MIR memory layer, and selected memory machine nodes preserve them for the accepted load/store subset. | present for target-MIR and accepted selected-node layer | AArch64 module records plus split codegen instruction/operand records | Final addressing legality, printer spelling, encoder records, and object behavior are later work; consumers must preserve the typed prepared address-space and volatility facts through structured records. |
| Branch and compare records sourced from prepared branch conditions and `BlockLabelId`. | `module::BranchRecord` preserves prepared branch facts and split `codegen/comparison.*`, `codegen/instruction.*`, and `codegen/dispatch.*` carry current branch/compare candidates where supported. Selected branch machine nodes preserve structured branch opcode identity without printing assembly. | present for target-MIR and accepted selected-node layer | AArch64 module records plus split codegen comparison/instruction/dispatch records | Final compare/branch syntax, layout, relaxation, and encoding remain later work over structured nodes. |
| Call records sourced from `call_plans`. | `CallRecord`, `CallArgumentRecord`, `CallResultRecord`, `CallPreservedValueRecord`, and `AbiBindingRecord` preserve prepared call and ABI-binding snapshots. | present for the accepted target-MIR contract; later direct variadic-count, outgoing-area, and special-role records are deferred candidates | `AAPCS64_CALL_RETURN_FRAME_CONTRACT.md` over module records | AArch64 call consumers must use these records, `ALLOCATION_CONTRACT.md`, and the AAPCS64 contract; they must not synthesize calling convention, argument/result registers, stack arguments, call-clobber resources, or call-time spills locally. |
| Move, copy, spill, reload, and ABI-binding records sourced from prepared value/regalloc/storage plans, allocation-result facts, and parallel copies. | `MoveRecord`, `AbiBindingRecord`, `SpillReloadRecord`, and `ParallelCopyRecord` are live target-MIR snapshots with prepared provenance, structured slot ids, target register records, scratch authority, and matched move/copy provenance where available. Selected spill/reload pseudo-node records preserve the accepted prepared spill/reload subset as structured machine nodes. | present for target-MIR snapshots and accepted spill/reload pseudo nodes | AArch64 module records plus `ALLOCATION_CONTRACT.md` | Instruction selection can consume explicit move/copy/spill/reload records. Spill-slot ids, destination homes, and reserved scratch policy must come from these records and the allocation contract, not local lowering slices. |
| Data/object side table for globals, string constants, symbol visibility, TLS, constants, initializers, and later relocation needs. | BIR module carries globals and strings. AArch64 has assembler/linker markdown and headers, but no codegen-owned data/object MIR side table. | missing | assembler/object work | Split object/relocation details behind an explicit assembler/object boundary; codegen should only publish structured data records. |
| Assembly generation boundary after MIR or explicit assembler/object contract. | AArch64 assembler headers are text-first (`AssembleRequest::asm_text`) and object emission is staged, not implemented. | deferred | assembler/object work | Assembly text emission must be a printer over structured machine instruction nodes, not the first rebuilt lowering API. Add encoding/object emission only after nodes or lower structured encoding records exist. |
| Built-in object emission and linking. | AArch64 assembler/linker contract surfaces exist, but object emission and linking are outside the backend entry contract. | deferred | assembler/object work | Keep this outside allocation/MIR-record consumer work unless the supervisor selects a separate binary-utils or linker idea. |

## Deferred Or Separate-Idea Items

| Item | Status | Owner | Reason |
| --- | --- | --- | --- |
| Final memory addressing legality and load/store instruction selection. | deferred | AArch64 codegen design | The shared carrier exposes `PreparedMemoryAccess::address_space` and `PreparedMemoryAccess::is_volatile`, and current split instruction/operand records preserve them; final lowering remains blocked until instruction-selection and addressing-legality policy consume those records. |
| Inline assembly operand substitution and clobber policy. | deferred | separate idea | Current markdown classifies old inline-asm routes as obsolete for this contract; rebuild requires an explicit structured inline-asm contract. |
| NEON and broad vector instruction coverage. | deferred | separate idea | Classification marks NEON as delete/defer; do not expand allocation/MIR-record consumer work around it without a new source idea. |
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

The required shared BIR/prepared carrier gap discovered by this contract was
volatile and non-default address-space preservation in prepared memory facts.
That gap is now covered by `PreparedMemoryAccess::address_space` and
`PreparedMemoryAccess::is_volatile`. AArch64 target code must preserve those
typed fields and must not recover volatility or address-space facts from
rendered names, printed BIR, legacy examples, or assembly text.

No other required BIR/prepared carrier gap blocks this layout contract. The
formerly missing AArch64 module, operand, allocation-result, move/copy,
spill/reload, ABI-binding, frame, call snapshot, AAPCS64 target-MIR
call/return/frame, target-MIR scalar/memory surfaces, and the accepted selected
branch/scalar/memory/spill-reload machine-node subset now exist as structured
records, selected machine nodes, or accepted contracts. The
remaining non-present or deferred items are later consumer contracts already
named here: dedicated return-boundary records, direct variadic-count exposure,
outgoing call-area carriers, explicit special-register role records, full
variadic function-entry carriers, final memory addressing legality and
load/store printer/encoder selection, structured machine instruction nodes
beyond the current selected branch/scalar/memory/spill-reload subset,
f128/i128, atomics, intrinsics, inline asm, assembler/encoder,
object/binary-utils, and linker behavior.

## Proceed Versus Split Decision

Backend implementation must not proceed directly to broad AArch64 instruction
selection or assembly text emission.

Proceed is allowed only through the structured target-record and selected
machine-node route. Later scalar, memory, branch, call, return, prologue,
epilogue, memory-return, indirect-call, variadic, vector, inline-asm, printer,
encoder, and object slices must consume `module::Module` records, the split
`codegen/` selected machine-node records where present, the allocation-result
authority in
`ALLOCATION_CONTRACT.md`, and the AAPCS64 boundary rules in
`AAPCS64_CALL_RETURN_FRAME_CONTRACT.md` for call/return/frame behavior. They
must not recover facts from rendered names, printed BIR, legacy LIR strings,
assembly text, parser operand recovery, or stale markdown examples.

Structured asm/encoding and later object records must preserve prepared
authority instead of duplicating it. `PreparedLiveness`,
`PreparedLiveInterval`, `PreparedRegalloc`, `PreparedRegallocValue`,
`PreparedInterferenceEdge`, `PreparedMoveResolution`,
`PreparedSpillReloadOp`, `PreparedValueLocations`,
`PreparedCallPreservedValue`, and `PreparedClobberedRegister` remain the
authority for live intervals, value homes, interference, move resolution,
spill/reload authority, preserved values, and call-clobbered registers. A
selected machine node or derived encoding record may add only the facts that
exist after opcode selection: implicit register uses/defs, selected opcode
clobber effects, condition flags, selected scratch lifetimes, operator side
effects, and section/relocation ownership for object-facing output. Those
post-selection facts still sit below the machine instruction node semantic
boundary and above printer, encoder, object, linker, or external assembler
consumers.

A separate shared BIR/prepared carrier initiative is not required for the
current target-MIR allocation/move/spill record layer because
`PreparedBirModule` already carries the contract's core module, identity,
control-flow, frame, stack, liveness, regalloc, storage, call plans, and typed
memory address-space/volatility facts, and AArch64 now has structured records
for the allocation-sensitive snapshots. If a later consumer discovers a missing
prepared carrier, record it as a separate open-idea candidate rather than
patching around it in AArch64-local lowering.

No target-local workaround is accepted for missing facts. In particular, do not
recover backend facts from rendered names, printed BIR, legacy LIR type
strings, old AArch64 markdown examples, assembly-string parsing, parser operand
recovery, or legacy shape recognizers.
