# AArch64 Duplicate Prepared Authority Audit

## Goal

Audit seven large AArch64 codegen files for duplicated BIR/prealloc/shared
prepared authority, then split seven follow-up ideas that each repair one file
family by consuming existing shared facts, adding missing shared authority, or
retiring legacy target-local recovery.

## Why This Exists

`src/backend/mir/aarch64/codegen` remains much larger than the reference ARM
layout under `ref/claudes-c-compiler/src/backend/arm/codegen`. Some of the
remaining size may be legitimate target emission, but there is a strong signal
that AArch64 still duplicates work already represented in BIR or shared
prealloc/prepared facts.

Examples to investigate include shared edge-publication source producers,
prepared scalar publication plans, prepared store-source publication plans,
prepared block-entry publications, call plans, value homes, and producer
indexes that AArch64 may still rederive through same-block scans or
target-local fallback paths.

This idea is an audit and follow-up splitting slice. It should not directly
rewrite the seven files.

## Files To Audit

- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/comparison.cpp`

## Durable Audit Findings

| File | Duplicated authority found | Existing shared facts to consume | Missing shared authority or proof questions | Target-emission residue |
| --- | --- | --- | --- | --- |
| `dispatch_edge_copies.cpp` | Edge source-producer fallback scans, predecessor producer recovery, block-entry parallel-copy matching, load-local edge source recovery, and select-chain/direct-global late materialization. | `PreparedEdgePublication`, `PreparedEdgePublicationSourceProducer`, `PreparedMoveBundle`, `PreparedEdgePublicationSourceMemoryAccessStatus`, `PreparedMemoryAccess`, `PreparedValueHome`, prepared edge-publication lookup helpers. | Need a prepared edge dependency query by source value plus edge context if `prepared_publication == nullptr` callers are real; need to decide whether late select-chain call-argument materialization belongs to call plans or a shared select-chain dependency query. | Register-read hazard checks, scratch ordering, and final move/load/store/select instruction spelling are AArch64 emission after source facts are chosen. |
| `dispatch_publication.cpp` | Value-home name scan fallback, block-entry publication reconstruction, conditional-branch/fused-compare producer scans, before-return FPR ABI move scans, local-slot address/frame-offset recovery, and fixed-formal store publication recovery. | `PreparedValueHomeLookups`, `PreparedBlockEntryPublication`, `PreparedScalarPublicationPlan`, `PreparedBranchCondition`, `PreparedMoveBundle`, `PreparedStoreSourcePublicationPlan`, `PreparedMemoryAccess`, `PreparedAggregateStackSourceAuthority`. | Need branch-condition or scalar-publication producer indexes for materialized branch/fused-compare operands; before-return move lookup by source value and destination bank; local-slot address/frame-offset query keyed by value/object if pointer-base-plus-offset homes cannot cover it. | Register spelling, current scratch choice, emitted move order, frame-address instruction spelling, and store/load mnemonics remain target-local. |
| `dispatch_value_materialization.cpp` | Same-block named producer recursion, load/global/select source reconstruction, block-entry/value-home fast-path recovery, local-slot address materialization, and direct-global select-chain materialization. | `PreparedEdgePublicationSourceProducerLookups`, `PreparedScalarPublicationPlan`, `PreparedBlockEntryPublication`, `PreparedValueHome`, `PreparedMemoryAccess`, `PreparedAddressingFunction`, `PreparedStoreSourcePublicationPlan`, recovered narrow store-source helpers. | Need proof for non-edge callers of `emit_value_publication_to_register`; possible scalar materialization/source-producer query by value plus instruction index; possible prepared global-load address/materialization query carrying GOT/direct policy; local-slot address query remains shared with publication findings. | Recursive operand emission, scratch/read-write hazard checks, load/global address spelling, and compare/select instruction spelling are target emission. |
| `memory.cpp` | Manual value-home id scans, memory identity reconstruction, before-return ABI retarget scans, stack-object/value-name matching, store-source producer recovery, pointer-base/global-symbol spelling recovery, and pending store-global publication tracking. | `PreparedMemoryAccess`, `PreparedAddressingFunction`, `PreparedValueHomeLookups`, `PreparedValueHome`, `PreparedMoveBundle`, `PreparedStoreSourcePublicationPlan`, `PreparedAggregateStackSourceAuthority`, `PreparedAddressMaterialization`, store-source recovery helpers. | Need before-return move lookup by source value and destination bank; stack-object/frame-slot lookup for local-address store values; global-symbol pointer-base field/query for pointer-base-plus-offset homes; store-global publication plan/query covering pending and duplicate stack-home state. | Load/store opcode choice, offset folding, pointer register materialization, duplicate-emission avoidance, source reloads, and stack/global store instruction emission remain target-local. |
| `alu.cpp` | Linear memory-access lookup by result value, same-block load-local producer and no-intervening-store scans, before-return ABI move scans, forward return-chain scans, and direct-global select-chain scalar publication fallback. | `PreparedValueHome`, `PreparedStoragePlanValue`, `PreparedMemoryAccess`, `PreparedAddressingFunction`, `PreparedScalarPublicationPlan`, `PreparedEdgePublicationSourceProducerLookups`, `PreparedMoveBundle`, `PreparedBranchCondition`. | Need memory-access lookup by result value id/name if ALU materializes stack/global scalar homes; scalar load-local source-producer query by value plus consumer instruction; before-return move lookup by source value and destination bank; possible return/result-chain prepared query; scalar select-chain materialization query for non-edge/non-store consumers. | Arithmetic opcode spelling, immediate optimizations, accumulator/direct-register paths, mul/div/rem scratch ordering, and register-read hazard checks remain target-local. |
| `calls.cpp` | Immediate ABI-binding scans, frame-slot call-argument move scans, recursive scalar call-argument producer walks, prepared-name BIR source scans, indirect-callee local-load/select-chain recovery, and after-call result move scans. | `PreparedCallPlan`, `PreparedCallArgumentPlan`, `PreparedCallArgumentSourceSelection`, `PreparedCallPreservedValue`, `PreparedIndirectCalleePlan`, `PreparedMemoryReturnPlan`, `PreparedCallBoundaryEffectPlan`, `PreparedMoveBundle`, `PreparedCallResultPlan`, `PreparedValueHome`. | Need shared binding-to-argument classification for immediate-only ABI bindings; move lookup by call argument or ABI index; call-argument producer/materialization query by value and call position; call-boundary source payload or source-producer query; indirect-callee source-producer/materialization facts; after-call result lookup by destination value and register bank. | AAPCS64 register/stack staging, direct `bl`/indirect `blr`, stack cleanup, source reload sequencing, and ABI result store instruction spelling remain target-local. |
| `comparison.cpp` | Fused-compare cast/load/constant producer scans, materialized compare condition producer hooks, constant RHS producer scans, stack-home/select producer gates, and block-entry publication use for fused compares. | `PreparedControlFlowBlock`, `PreparedBranchCondition`, `PreparedValueHome`, `PreparedScalarPublicationPlan`, `PreparedBlockEntryPublication`, `PreparedEdgePublicationSourceProducerLookups`, `PreparedMemoryAccess`. | Need fused-compare operand producer index/kind and load-local source query; branch-condition producer instruction index or scalar-publication hook for materialized compare conditions; constant/materialized fused-compare operand query; select/load-local/load-global producer-kind query for stack-home operands. | Condition-code spelling, `cmp`/`cmn`/`fcmp`/`cset`/`csel`/branch emission, typed register/immediate print operands, scratch selection, and `cbnz` fallback remain target-local. |

## In Scope

- Compare each file against existing BIR/prealloc/shared prepared authority.
- Compare each file's owner responsibilities against the reference ARM codegen
  layout, especially `emit.rs`, `calls.rs`, `memory.rs`, `alu.rs`, and
  `comparison.rs`.
- Identify duplicated local producer scans, source/home recovery, publication
  planning, edge-copy facts, call source decisions, and same-block fallback
  paths.
- Classify each suspicious helper as:
  - `consume-shared`: AArch64 should use an existing shared fact.
  - `missing-shared-field`: shared authority exists but lacks a field/query.
  - `target-emission`: AArch64 should keep it as register/scratch/addressing
    or instruction spelling logic.
  - `legacy-fallback`: old local recovery should be retired or fail-closed.
  - `needs-more-evidence`: requires a narrow proof before choosing a route.
- Produce exactly seven follow-up ideas, one for each audited file, with stable
  numbering and owned repair scope.
- Each follow-up idea should name the shared facts it intends to consume or the
  missing shared authority it must add.

## Out Of Scope

- Direct implementation edits in the audited files.
- Mechanical file merging or build metadata cleanup.
- Broad AArch64 codegen rewrites.
- Reopening already-closed fold-back cleanup unless the audit finds a concrete
  duplicate-authority bug.
- Treating line count reduction as success without removing duplicated
  authority.

## Acceptance Criteria

- A durable audit table exists for all seven files.
- The audit distinguishes existing shared facts from genuinely missing shared
  authority.
- Seven numbered follow-up ideas are created under `ideas/open/`, one per
  audited file.
- Follow-up ideas separate semantic authority repair from target-local
  emission cleanup.
- The final close note states which files appear to duplicate existing
  BIR/prealloc authority and which are mostly target-emission residue.

## Reviewer Reject Signals

- A patch edits implementation while claiming to be audit-only.
- A patch says "move to BIR" without naming the current duplicated helper or
  the required shared fact.
- A patch treats every producer lookup as wrong without checking whether it is
  target-local hazard/emission logic.
- A patch produces vague follow-up ideas that do not own one audited file each.
- A patch ignores the reference ARM layout or the existing prepared authority
  in `src/backend/prealloc`.
