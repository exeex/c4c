# BIR Prealloc Legacy Compatibility Residue Audit

## Goal

Audit the remaining BIR/prealloc compatibility residues that were explicitly
retained or deferred by recent closure notes, then produce focused follow-up
ideas for the residues that should be converted into clearer shared authority
before x86/RISC-V rebuild work depends on them.

## Why This Exists

Recent BIR/prealloc boundary work closed the known call ABI, memory/provenance,
control/publication, pointer-carrier, global identity, stack-layout, inline-asm,
and select-chain follow-ups. The close notes also deliberately retained a few
compatibility or deferred paths rather than treating them as immediate bugs.

Those retained paths are acceptable short term, but they are exactly the kind
of implicit contract that can make the next architecture rebuild duplicate
logic in `src/backend/mir/*` instead of consuming shared BIR/prealloc facts.
Before starting broader x86/RISC-V implementation, this audit should separate
harmless compatibility glue from residues that deserve new, narrow ideas.

## Owned Files

- Audit/read:
  - `ideas/closed/100_aarch64_00204_stdarg_hfa_runtime_repair.md`
  - `ideas/closed/101_closure_note_followup_recovery_audit.md`
  - `ideas/closed/104_bir_prealloc_pointer_carrier_provenance_contract.md`
  - `ideas/closed/106_prealloc_stack_layout_slice_family_fact_contract.md`
  - `ideas/closed/107_prealloc_inline_asm_memory_effect_metadata_contract.md`
  - `ideas/closed/108_prepared_select_chain_dump_contract_coverage.md`
  - `src/backend/bir/**`
  - `src/backend/prealloc/**`
  - `src/backend/mir/aarch64/codegen/**` only for consumer examples
- Write:
  - new `ideas/open/*.md` follow-up ideas only

## In Scope

- Inventory retained compatibility and deferred paths, including:
  - `LegacySlotNameSliceFamilyCompatibility`
  - `LegacyFrameAddressNameCompatibility`
  - pointer-carrier and memory-base publication/dereference boundary residue
  - deferred store-source dump visibility
  - dynamic alloca / VLA no-action notes
  - `prepared_lookups.cpp` helpers that may be doing more than indexing
- Classify each item as:
  - `safe-compatibility-glue`
  - `needs-explicit-bir-fact`
  - `needs-prealloc-contract-test`
  - `needs-prepared-dump-visibility`
  - `arch-consumer-risk`
  - `stale-no-action`
  - `needs-follow-up-idea`
- For every `needs-follow-up-idea`, create a narrow idea with exact ownership,
  proof route, and reject signals.
- Produce a close note mapping each audited residue to its disposition.

## Out Of Scope

- Implementing any residue cleanup.
- Moving stack placement, register allocation, frame offsets, or target ABI
  placement into BIR.
- Reopening already-completed ideas `100` through `108`.
- Creating direct x86/RISC-V backend implementation work before shared
  authority gaps are named.
- Treating every `prepared_lookups.cpp` helper as duplicate authority without
  proving it derives semantic facts.

## Proof Expectations

- This is analysis-only; no backend tests are required unless implementation
  files are accidentally changed.
- The proof is the generated set of focused `ideas/open/*.md` files plus the
  disposition table in the close note.
- If no follow-up ideas are needed, the close note must explicitly justify why
  each retained residue is safe or stale.

## Reviewer Reject Signals

- The audit proposes implementation before producing the residue inventory.
- It creates broad "clean BIR/prealloc" follow-ups without naming exact facts.
- It treats compatibility naming as automatically wrong without checking
  whether later work already made it unreachable.
- It ignores x86/RISC-V consumer risk when deciding no-action.

## Closure Notes

Closed after the active audit runbook reached Step 4, "Prepare Audit Closure
Evidence." The audit remained analysis-only: no implementation files, backend
tests, or expectation contracts were changed by the audit itself.

Final disposition:

| Audited residue | Final disposition | Follow-up idea path | Closure reason |
| --- | --- | --- | --- |
| `LegacySlotNameSliceFamilyCompatibility` | `safe-compatibility-glue` / retained bootstrap | None | Still reachable as the only current `PreparedStackSliceFamily` producer for dotted scalarized local-slot names. It feeds stack-layout slice coverage and fixed placement, but does not create independent physical placement authority. Removing or narrowing it belongs only after a non-legacy prepared slice-family producer exists. |
| `LegacyFrameAddressNameCompatibility` stack-layout bridge | `safe-compatibility-glue` / retained bootstrap | None | Still reachable in stack layout to seed frame-address publication facts. Prepared frame-address materialization authority now exists downstream, but stack layout still needs the legacy bridge until BIR/prepared publishes durable frame-address value identity directly. |
| `find_local_frame_address_source()` call-planning fallbacks using `LegacyFrameAddressNameCompatibility` | `needs-follow-up-idea` | `ideas/open/110_call_planning_frame_address_materialization_authority.md` | The three remaining call-planning fallbacks are reachable name-derived compatibility lookups. Ordinary local-frame-address lowering appears covered by explicit `PreparedAddressMaterializationKind::FrameSlot` production and lookup, so replacement should be owned by a focused follow-up that proves the computed pointer-carrier caveat before removing the fallback. |
| Pointer-carrier publication / dereference boundary residue | `safe-compatibility-glue` / retained-by-boundary | None | Current carrier propagation requires explicit prepared/BIR authority from prepared pointer-value access, frame-address materialization, pointer-symbol identity, or immediate pointer add/sub from an authorized base. No path was found that publishes or dereferences a carrier solely from raw load/store order, slot spelling, byte deltas, or absent `MemoryAddress`. |
| Memory-base publication / dereference boundary residue | `safe-compatibility-glue` / retained-by-boundary | None | Object-specific pointer/global/string/inline-asm memory facts require structured `MemoryAddress`, inline-asm operand metadata, structured symbol identity, or an existing prepared memory/address fact. The no-address local-slot route only reinforces conservative prepared frame homes and does not mint non-frame provenance. |
| Deferred store-source dump visibility | `needs-follow-up-idea` | `ideas/open/111_store_source_publication_dump_visibility.md` | Store-source source-producer and direct-global select-chain facts now exist as bounded `PreparedStoreSourcePublicationPlan` fields and are consumed by target-side planning, but `prepare::print()` still has no prepared-module dump surface for them. The gap is contract visibility, not missing semantic authority. |
| Dynamic alloca / VLA no-action notes | `stale-no-action` | None | Live support is already represented through `PreparedDynamicStackPlan`, prepared-printer coverage, prepared frame/register policy, and fail-closed AArch64 lowering. No concrete missing target-neutral lifetime, extent, or target stack-adjustment fact was found. Raw helper spellings remain bounded pseudo-intrinsic identity for producing or matching prepared dynamic-stack operations. |
| `prepared_lookups.cpp` / `select_chain_lookups.cpp` helper authority questions | `safe-compatibility-glue` | None | The audited helpers index, select, validate, or package existing prepared/BIR facts. No helper was found creating durable semantic authority, re-deriving memory provenance, or exposing a consumer-facing API gap beyond the separately recorded store-source dump-visibility follow-up. |

Actionable residue was split into focused follow-up ideas:

- `ideas/open/110_call_planning_frame_address_materialization_authority.md`
- `ideas/open/111_store_source_publication_dump_visibility.md`

The remaining retained paths are intentionally left as compatibility/query glue
or stale/no-action residue under the reasons above. Reopening any retained path
needs fresh current-code evidence of semantic leakage, missing shared authority,
or consumer-facing contract risk beyond the facts recorded here.
