# BIR Prealloc Memory Pointer Storage Boundary Audit

## Goal

Analyze whether BIR memory/provenance facts are duplicated by prealloc stack
layout, addressing, storage, and pointer-carrier preparation, then produce
focused follow-up ideas for any real authority overlap.

## Why This Exists

BIR memory lowering already builds structured facts for local slots, globals,
pointer values, memory addresses, GEP-derived addresses, dynamic stack access,
and provenance-like relationships. Prealloc then builds stack layout,
prepared addressing, storage plans, decoded homes, pointer carriers, and
prepared memory/access lookups.

For x86 and RISC-V rebuild work, this boundary must be clean: BIR should own
target-neutral memory semantics and identity, while prealloc should own stack
placement, storage/home authority, and target-facing prepared address plans.
The audit should find places where prealloc reconstructs BIR memory provenance
from instruction shape instead of consuming a BIR fact, as well as places where
BIR lacks a target-neutral fact and prealloc legitimately fills the gap.

## Owned Files

- Audit/read:
  - `src/backend/bir/bir.hpp`
  - `src/backend/bir/lir_to_bir/memory/*.cpp`
  - `src/backend/bir/lir_to_bir/memory/*.hpp`
  - `src/backend/bir/lir_to_bir/global_initializers.cpp`
  - `src/backend/bir/lir_to_bir/globals.cpp`
  - `src/backend/prealloc/stack_layout/*.cpp`
  - `src/backend/prealloc/addressing.hpp`
  - `src/backend/prealloc/storage_plans.cpp`
  - `src/backend/prealloc/decoded_home_storage.*`
  - `src/backend/prealloc/regalloc/pointer_carriers.cpp`
  - `src/backend/prealloc/prepared_lookups.*`
- No implementation files should be edited by this idea.

## In Scope

- Inventory BIR memory/address/provenance facts and classify their authority.
- Inventory prealloc stack-layout/addressing/storage/pointer-carrier facts.
- Trace overlaps around:
  - local slot and frame slot identity
  - global symbol and address materialization identity
  - pointer value plus offset provenance
  - dynamic stack / alloca / VLA access
  - typed stack source and stack object facts
  - storage-home and decoded-home facts
- Classify each overlap as:
  - `bir-semantic-fact`
  - `prealloc-placement-authority`
  - `prealloc-rederives-bir-provenance`
  - `bir-missing-target-neutral-fact`
  - `contract-ambiguous`
  - `needs-follow-up-idea`
- Produce follow-up ideas only for narrow, proofable boundaries.

## Out Of Scope

- Implementing memory, stack-layout, storage, or pointer-carrier cleanup.
- Moving target stack placement, register/storage homes, or frame offset
  assignment into BIR.
- Reopening AArch64 memory owner cleanup routes.
- Treating every prealloc scan over BIR instructions as duplicated authority
  without tracing the fact being reconstructed.

## Proof Expectations

- This is analysis-only; no backend tests are required unless implementation
  files are accidentally changed.
- The close note must name any generated follow-up ideas and explicitly list
  memory/pointer/storage overlaps judged intentional.

## Reviewer Reject Signals

- The audit says "move memory to BIR" without naming a missing target-neutral
  fact.
- It proposes broad stack-layout or prepared-lookup rewrites without a traced
  overlap.
- It ignores the distinction between BIR provenance and prealloc placement.

## Step 5 Synthesis

Generated follow-up ideas:

- `ideas/open/104_bir_prealloc_pointer_carrier_provenance_contract.md`
- `ideas/open/105_prealloc_raw_global_address_identity_fallback_contract.md`
- `ideas/open/106_prealloc_stack_layout_slice_family_fact_contract.md`
- `ideas/open/107_prealloc_inline_asm_memory_effect_metadata_contract.md`

Intentional retained BIR semantic authority:

- Local slot identity, scalar storage facts, local pointer-to-slot provenance,
  `MemoryAddress` annotations, and local/global load/store operation facts.
- Global identity, extent, initializer facts, TLS/constant/external flags,
  target-neutral global address materialization policy, and GEP-derived global
  offsets.
- Runtime pointer base plus offset facts where surfaced as
  `MemoryAddress::PointerValue`, dynamic local/global array interpretation,
  dynamic alloca as call-like pointer production until a later target route
  proves a distinct lifetime/stack-adjustment fact is needed, and structured
  inline-asm/intrinsic memory operand metadata.

Intentional retained prealloc placement/storage authority:

- Stack object candidacy, address-publication and coalescing placement hints,
  frame-slot IDs, frame offsets, frame size/alignment, and stack object
  ordering.
- Prepared frame/global/string/pointer addresses, address materializations,
  target relocation/TLS interpretation, and base-plus-offset availability.
- Storage plans, decoded homes, register/stack homes, spill slots,
  immediates/symbol storage encodings, and aggregate or typed stack-source
  publication when based on prepared homes and frame-layout facts.

Overlaps deliberately not split into follow-up ideas:

- `prepared_lookups.cpp` same-block and source-producer helpers remain query
  glue unless a later implementation route proves they create semantic memory
  provenance instead of indexing prepared accesses and frame-range facts.
- Dynamic alloca remains a call-like pointer producer for this audit; a future
  idea should only be created if target stack adjustment, lifetime, or extent
  handling needs a distinct target-neutral dynamic-allocation fact.
- Missing aggregate-copy authority is target-facing lowering/codegen follow-up
  material, not evidence that BIR should own stack-source placement.

Close decision: source-complete but close rejected by the close-time regression
guard. The backend guard had the same existing failure before and after
(`c_testsuite_aarch64_backend_src_00204_c`) and no new failing tests, but the
guard script rejected the comparison because the pass count did not strictly
increase (`389/390` before and after). Keep this idea open until the close-gate
policy is satisfied or the supervisor chooses a different lifecycle action.
