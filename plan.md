# AArch64 Memory Operand Model From Prepared Facts Plan

Status: Active
Source Idea: ideas/open/210_aarch64_memory_operand_model_from_prepared_facts.md
Activated: 2026-05-13

## Purpose

Define the AArch64 target-local memory operand model that consumes structured
BIR memory semantics and shared prepared memory facts, including volatility and
address space, without selecting load/store instructions or emitting assembly.

Goal: preserve prepared memory access identity, addressing facts, volatility,
and address-space information in target-local AArch64 memory operand records
under the `codegen/` owner.

## Core Rule

Build memory operand records from structured BIR and prepared facts. Do not
parse rendered value names, printed BIR, markdown examples, or legacy text
forms to recover memory bases, symbols, slots, volatility, address space, or
offsets.

## Read First

- `ideas/open/210_aarch64_memory_operand_model_from_prepared_facts.md`
- `ideas/closed/206_prepared_memory_volatility_address_space_carrier.md`
- `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md`
- `src/backend/mir/aarch64/codegen/records.hpp`
- `src/backend/mir/aarch64/codegen/records.cpp`
- `src/backend/mir/aarch64/codegen/memory.md`
- `src/backend/mir/aarch64/module/`
- `tests/backend/`

## Current Targets

- Target-local AArch64 memory operand records that preserve prepared memory
  identity, base kind, frame-slot id, symbol `LinkNameId`, pointer value
  id/name, string identity, byte offset, size, alignment, volatility, and
  address space.
- Conversion helpers that consume `PreparedBirModule`, `PreparedAddressing`,
  and `PreparedMemoryAccess` facts without using rendered names as authority.
- Focused tests or compile proof for representative frame-slot, global-symbol,
  pointer-value, and string-constant bases where existing fixtures permit.

## Non-Goals

- Selecting or emitting `ldr`, `str`, pair load/store, atomics, address
  materialization, memcpy loops, or dynamic alloca instruction sequences.
- Assembly text emission, instruction encoding, object output, or linking.
- Inventing target-local defaults for volatility or address space.
- Fixing frontend, LIR, BIR, or prepared production of missing memory facts in
  this plan.
- Calls, returns, scalar ALU, branch selection, atomics, inline asm, or vector
  memory operations beyond explicit deferred notes.
- Moving target memory ownership into `module/`.

## Working Model

- `module/` remains the prepared/BIR snapshot owner.
- `codegen/` owns AArch64 target memory operand records and conversion helpers.
- Prepared memory access ids, BIR memory values, prepared addressing facts,
  volatility, address space, base ids, typed operands, size, alignment, and
  offsets stay authoritative.
- Missing upstream facts should fail closed or be recorded as a separate open
  idea; do not synthesize target-local defaults that hide missing input.

## Execution Rules

- Keep every step record-only unless the source idea explicitly permits more.
- Prefer small tests that instantiate or convert representative prepared/BIR
  memory facts directly.
- For code-changing steps, run:
  `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') 2>&1 | tee test_after.log`
- Escalate before close if ownership spreads outside AArch64 target-record or
  prepared-memory model surfaces, or if any proof suggests volatility or
  address-space facts are being defaulted instead of preserved.

## Steps

### Step 1: Confirm Memory Inputs And Existing Owners

Goal: verify current prepared memory facts, BIR memory inputs, and target record
surfaces before changing the memory operand model.

Primary targets:
- `src/backend/mir/aarch64/codegen/records.hpp`
- `src/backend/mir/aarch64/codegen/records.cpp`
- `src/backend/mir/aarch64/codegen/memory.md`
- `src/backend/mir/aarch64/module/`
- `src/backend/prealloc/`
- `tests/backend/`

Actions:
- Inspect the existing `MemoryOperand` target record surface and any memory
  placeholders introduced by earlier target-record work.
- Identify structured `PreparedAddressing` and `PreparedMemoryAccess` facts
  available from prepared modules, including volatility and address space.
- Identify representative existing fixtures for frame-slot, global-symbol,
  pointer-value, and string-constant memory bases.
- Record unsupported or missing upstream facts in `todo.md`; create a separate
  source idea only if the missing fact is a distinct upstream initiative.

Completion check:
- `todo.md` states the owner files, usable prepared/BIR inputs, unsupported
  inputs, and the first concrete memory operand record packet.

### Step 2: Add Or Tighten Memory Operand Record Vocabulary

Goal: ensure target-local memory operand records can represent all in-scope
prepared memory facts without selecting load/store instructions.

Primary targets:
- `src/backend/mir/aarch64/codegen/records.hpp`
- `src/backend/mir/aarch64/codegen/records.cpp`
- `tests/backend/`

Actions:
- Add or tighten record-only memory operand fields for prepared memory access
  identity, base kind, frame-slot id, symbol `LinkNameId`, pointer value
  id/name, string identity, byte offset, size, alignment, volatility, and
  address space.
- Add explicit diagnostic or vocabulary helpers where useful for unsupported
  or deferred memory operand forms.
- Preserve existing branch, scalar, call, return, assembler, and object record
  behavior.
- Test representative direct memory operand record construction.

Completion check:
- Backend subset proof passes and tests show typed memory operands preserving
  prepared facts without assembly, encoding, object output, or load/store
  selection.

### Step 3: Convert Prepared Frame And Symbol Memory Bases

Goal: construct target memory operand records from structured prepared facts
for stable frame-slot and global-symbol bases.

Primary targets:
- `src/backend/mir/aarch64/codegen/records.hpp`
- `src/backend/mir/aarch64/codegen/records.cpp`
- `tests/backend/`

Actions:
- Add conversion helpers that consume BIR memory instructions and matching
  `PreparedAddressing` / `PreparedMemoryAccess` facts for frame-slot and
  symbol-backed memory references.
- Preserve prepared memory identity, slot or symbol ids, offsets, size,
  alignment, volatility, and address space.
- Fail closed when the required prepared facts are missing, mismatched, or only
  recoverable from rendered names.
- Test representative successful conversion and guard cases.

Completion check:
- Backend subset proof passes and conversion tests demonstrate structured
  frame-slot and symbol memory operand records sourced from prepared/BIR facts.

### Step 4: Convert Prepared Pointer And String Memory Bases

Goal: extend prepared memory operand conversion to pointer-value and
string-constant bases where existing prepared facts permit it.

Primary targets:
- `src/backend/mir/aarch64/codegen/records.hpp`
- `src/backend/mir/aarch64/codegen/records.cpp`
- `tests/backend/`

Actions:
- Add conversion support for pointer-value bases that preserve value id/name,
  type, offset, size, alignment, volatility, and address space.
- Add conversion support for string-constant bases when structured text or
  symbol identity exists.
- Fail closed or defer when pointer or string facts are incomplete or only
  present in rendered output.
- Test successful conversions and explicit unsupported/deferred behavior.

Completion check:
- Backend subset proof passes and tests cover representative pointer-value and
  string-constant memory operands without load/store selection.

### Step 5: Prove Volatility And Address-Space Preservation

Goal: make volatility and address-space preservation explicit across the target
memory operand model.

Primary targets:
- `src/backend/mir/aarch64/codegen/records.hpp`
- `src/backend/mir/aarch64/codegen/records.cpp`
- `tests/backend/`

Actions:
- Add focused coverage that distinguishes volatile from non-volatile memory
  records.
- Add focused coverage that preserves non-default address spaces when prepared
  facts provide them.
- Ensure missing upstream production is documented in `todo.md` or split into
  a separate idea instead of bypassed in AArch64 target code.

Completion check:
- Backend subset proof passes and tests prove volatility/address-space facts
  survive into target-local memory operand records.

### Step 6: Document Memory Operand Contract And Close Readiness

Goal: document the record-only AArch64 memory operand boundary and prove the
source idea acceptance criteria.

Primary targets:
- `src/backend/mir/aarch64/codegen/records.md`
- `src/backend/mir/aarch64/codegen/memory.md`
- `tests/backend/`
- `todo.md`

Actions:
- Document supported memory operand bases and preserved prepared facts.
- Document deferred memory lowering and unsupported facts as fail-closed, not
  silently defaulted.
- Add or update a contract test if needed to assert record-only ownership and
  absence of load/store selection, assembly, object, call, or return behavior.
- Run the backend subset proof and leave close-readiness notes in `todo.md`.

Completion check:
- Backend subset proof passes, acceptance criteria from the source idea are
  satisfied, and `todo.md` is ready for supervisor close routing.
