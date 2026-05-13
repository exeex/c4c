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
call plans. However, memory volatility and address-space facts are missing from
the visible prepared memory carrier. If Step 6 chooses memory lowering as its
first implementation slice, split a shared-preparation carrier extension first;
otherwise record that gap and proceed with the AArch64 MIR design around the
present prepared facts.

No target-local workaround is accepted for missing facts. In particular, do not
recover backend facts from rendered names, printed BIR, legacy LIR type
strings, old AArch64 markdown examples, assembly-string parsing, parser operand
recovery, or legacy shape recognizers.
