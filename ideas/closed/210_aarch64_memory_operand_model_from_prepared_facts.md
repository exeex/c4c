# AArch64 Memory Operand Model From Prepared Facts

Status: Closed
Created: 2026-05-13
Closed: 2026-05-13

Depends On:
- `ideas/open/207_aarch64_target_register_and_instruction_record_core.md`
- `ideas/closed/206_prepared_memory_volatility_address_space_carrier.md`

## Goal

Define the AArch64 target-local memory operand model that consumes BIR memory
semantics and shared prepared memory facts, including volatility and
address-space, without selecting load/store instructions or emitting assembly.

This idea is the bridge between the shared carrier fixed in idea 206 and later
AArch64 memory lowering.

## Why This Idea Exists

Idea 206 added typed prepared memory facts:
`PreparedMemoryAccess::address_space` and `PreparedMemoryAccess::is_volatile`.
The AArch64 layout ledger now says full memory lowering is allowed only after
the target-local memory operand model preserves those facts. Without this
model, a later load/store slice would either drop volatility/address-space or
invent ad hoc memory records in the first file that needs them.

## Reference Inputs

- `ideas/closed/206_prepared_memory_volatility_address_space_carrier.md`
- `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md`
- `src/backend/mir/aarch64/codegen/memory.md`
- `ref/claudes-c-compiler/src/backend/arm/codegen/memory.rs`
- `ref/claudes-c-compiler/src/backend/arm/assembler/encoder/load_store.rs`

## In Scope

- Define target-local AArch64 memory operand records that preserve:
  - prepared memory access identity
  - base kind
  - frame-slot id
  - symbol `LinkNameId`
  - pointer value id/name
  - string identity
  - byte offset
  - size and alignment
  - volatility
  - address space
- Source memory operand records from BIR retained in `PreparedBirModule` plus
  `PreparedAddressing` / `PreparedMemoryAccess` facts.
- Place memory operand ownership in the file/directory selected by ideas 205
  and 207, without expanding `module/` beyond prepared snapshots.
- Add focused tests or compile proof for frame-slot, global-symbol,
  pointer-value, and string-constant bases where existing fixtures permit.
- Prove volatile and non-default address-space facts survive into the
  target-local memory operand records.

## Out Of Scope

- Selecting or emitting `ldr`, `str`, pair load/store, atomic memory ops,
  address materialization, memcpy loops, or dynamic alloca instruction
  sequences.
- Assembly text emission, instruction encoding, object output, or linking.
- Inventing target-local defaults for volatility or address space.
- Fixing frontend/LIR production of volatile or address-space facts if a
  source route still cannot produce them; split a separate upstream idea.
- Calls, returns, scalar ALU, branch selection, atomics, inline asm, or vector
  memory operations beyond placeholder/deferred notes.

## Acceptance Criteria

- AArch64 target-local memory operand records exist and preserve typed
  prepared memory facts, including `address_space` and `is_volatile`.
- Representative prepared memory bases produce structured memory operand
  records without parsing rendered names or printed BIR.
- Missing upstream production for a memory fact is documented or split as a
  separate idea rather than bypassed in AArch64 target code.
- Tests or compile proof cover representative memory operand construction and
  volatility/address-space preservation.
- No load/store instruction selection or assembly emission is introduced.

## Closure Notes

Closed after the active runbook completed all six steps. The accepted work
defined the AArch64 target-local memory operand model under the target record
owner, added prepared-fact conversion coverage for representative memory bases,
documented the memory operand contract, and proved the record-only boundary.

The completed slice preserves prepared memory access identity, base identity,
offset, size, alignment, volatility, and address-space facts in target-local
records. Load/store instruction selection, assembly, encoding, object output,
calls, returns, and memory emission remain out of scope.

Close validation used the backend proof scope:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') 2>&1 | tee test_after.log`

Regression guard passed with `--allow-non-decreasing-passed`: 131 passed before,
131 passed after, 0 failed.

## Reviewer Reject Signals

- The slice emits load/store assembly or encodes memory instructions.
- Volatility or address-space is dropped, defaulted silently, or recovered from
  printed/rendered text.
- Memory operands use rendered global/slot/value names as authority when ids
  are available.
- The implementation supports only one named memory fixture through shape
  matching.
- Missing BIR/prepared facts are worked around inside AArch64 target code
  instead of becoming a separate source idea.
