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

