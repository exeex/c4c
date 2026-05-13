# Prepared Memory Volatility And Address-Space Carrier

Status: Open
Created: 2026-05-13

Discovered From:
- `ideas/open/205_aarch64_arm_reference_layout_contract.md`
- `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md`

## Goal

Extend the shared BIR/prepared memory fact carrier so backend targets can
preserve memory volatility and address-space semantics through prepared memory
accesses instead of recovering those facts from printed BIR, rendered names, or
target-local pattern matching.

The visible prepared memory surface currently exposes
`PreparedMemoryAccess` / `PreparedAddress` facts for identity, base, offset,
size, and alignment. The AArch64 layout review found no explicit prepared
fields for volatile memory operations or non-default address spaces. Full
target memory lowering must not proceed until those facts have an accepted
shared carrier.

## Why This Idea Exists

The AArch64 ARM-reference layout contract requires every target memory operand
to trace back to BIR facts and supplemental `PreparedBirModule` facts. Memory
lowering needs enough prepared information to distinguish ordinary accesses
from volatile accesses and to preserve non-default address-space semantics.

Keeping that gap inside the AArch64 target would recreate the backend drift the
layout contract is meant to prevent: target code would infer semantics from
incidental BIR spelling or from whichever narrow testcase first needs memory
lowering. This gap belongs in the shared preparation boundary.

## In Scope

- Audit the BIR memory operation/type representation for volatile and
  address-space facts that already exist before preparation.
- Decide where prepared memory facts should carry volatility and address-space
  data: `PreparedMemoryAccess`, `PreparedAddress`, a nested memory-effect
  record, or another shared prepared carrier.
- Preserve the existing prepared base, offset, size, alignment, and identity
  behavior while adding the missing semantic facts.
- Update shared preparation builders and lookup helpers so prepared memory
  consumers can retrieve volatility and address-space facts without parsing
  printed BIR.
- Add focused tests or fixture coverage proving the prepared carrier preserves
  volatile and non-default address-space facts when those facts are present in
  BIR.
- Update backend-facing documentation that describes the prepared memory
  carrier contract.

## Out Of Scope

- Implementing AArch64, x86, or any other target's final memory instruction
  selection.
- Adding assembler, object writer, linker, or relocation behavior.
- Inventing target-local volatility or address-space defaults to bypass missing
  BIR facts.
- Broadly redesigning `PreparedBirModule` beyond the memory fact carrier
  needed for this gap.
- Treating atomics, memory ordering, inline assembly memory clobbers, or
  language-level alias analysis as complete under this idea.
- Weakening, skipping, or reclassifying backend tests to make memory lowering
  appear supported before the shared carrier exists.

## Acceptance Criteria

- The shared prepared memory carrier exposes explicit volatility and
  address-space facts for memory accesses when those facts exist in BIR.
- Prepared-memory construction preserves those facts for frame-slot,
  global-symbol, pointer-value, and string-constant address bases where the BIR
  input can express them.
- Backend consumers can query the facts through typed prepared structures or
  helpers, not through rendered text, debug dumps, old markdown examples, or
  target-specific shape matching.
- Tests or documented fixture proof cover at least one volatile access and one
  non-default address-space access, or clearly document why the current BIR
  frontend cannot yet produce one of those facts.
- Existing prepared base, offset, size, and alignment behavior remains
  compatible with current consumers.
- Documentation names the exact prepared fields/helpers that targets should
  consume before implementing memory lowering.

## Reviewer Reject Signals

- The implementation makes only AArch64 target code aware of volatility or
  address spaces while leaving the shared prepared carrier unchanged.
- The route parses printed BIR, rendered operand names, debug dumps, source
  spellings, or testcase-specific instruction names to recover memory
  semantics.
- The change handles only one named testcase or one hard-coded address-space
  value without defining the general prepared carrier contract.
- Tests are weakened, skipped, reclassified, or rewritten to claim progress
  without proving the shared carrier preserves the missing facts.
- The diff only renames helpers, updates markdown classifications, or rewrites
  expectations while retaining the old absence of typed volatility and
  address-space facts.
- The route expands assembler/object/linker or target instruction selection
  behavior as part of this carrier gap.
- The new abstraction still drops volatile or non-default address-space facts
  before backend consumers can query them.
