# AArch64 Scalar ALU Cast First Instruction Slice Plan

Status: Active
Source Idea: ideas/open/209_aarch64_scalar_alu_cast_first_instruction_slice.md
Activated: 2026-05-13

## Purpose

Create the first small AArch64 instruction-selection slice on top of the accepted
target record core: scalar integer ALU and simple cast target records only.

Goal: represent selected BIR scalar operations as typed AArch64 target
instruction records in `codegen/` without assembly emission, object output, or
memory/call lowering.

## Core Rule

Lower from structured BIR and prepared facts into target records. Do not parse
rendered value names, printed BIR, markdown examples, or legacy text forms to
recover operands or opcodes.

## Read First

- `ideas/open/209_aarch64_scalar_alu_cast_first_instruction_slice.md`
- `src/backend/mir/aarch64/codegen/records.hpp`
- `src/backend/mir/aarch64/codegen/records.cpp`
- `src/backend/mir/aarch64/codegen/alu.md`
- `src/backend/mir/aarch64/codegen/cast_ops.md`
- `src/backend/mir/aarch64/module/`
- `tests/backend/`

## Current Targets

- A narrow scalar integer ALU record slice, such as add, sub, and simple
  bitwise records where prepared operand facts are complete.
- A narrow simple integer cast record slice, such as sign extension, zero
  extension, or truncation where BIR and prepared type facts are complete.
- Tests proving representative successful ALU and cast records plus explicit
  unsupported/deferred behavior.

## Non-Goals

- Assembly text emission, instruction encoding, object output, or linker work.
- Memory loads/stores, calls, returns, variadic behavior, prologue/epilogue,
  atomics, intrinsics, inline asm, f128/i128, or broad floating-point lowering.
- Register allocation policy changes.
- Testcase-shaped instruction selection or named fixture shortcuts.
- Moving target record ownership into `module/`.

## Working Model

- `module/` remains the prepared/BIR snapshot owner.
- `codegen/` owns AArch64 target instruction records and scalar selection
  helpers introduced by this plan.
- Prepared value ids, value names, typed register records, operand records,
  BIR opcodes, operand types, and result types stay authoritative.
- Unsupported scalar ALU and cast forms must fail closed or be explicitly
  deferred; they must not silently become generic placeholder instructions.

## Execution Rules

- Keep each step record-only unless the source idea explicitly permits
  behavior beyond records.
- Prefer small tests that instantiate or convert representative prepared/BIR
  facts directly.
- For code-changing steps, run:
  `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') 2>&1 | tee test_after.log`
- Escalate to broader validation before close if ownership spreads outside the
  backend target-record surfaces or if multiple narrow steps land without a
  fresh backend subset proof.

## Steps

### Step 1: Confirm Scalar Inputs And Existing Record Owners

Goal: verify the available BIR scalar, prepared value, prepared register, and
target record surfaces before adding scalar selection records.

Primary targets:
- `src/backend/mir/aarch64/codegen/records.hpp`
- `src/backend/mir/aarch64/codegen/records.cpp`
- `src/backend/mir/aarch64/module/`
- `tests/backend/`

Actions:
- Inspect existing target record core and branch/compare record surfaces.
- Identify structured BIR scalar ALU and cast inputs available from
  `PreparedBirModule`.
- Identify the prepared value/register/type facts needed for ALU and cast
  record construction.
- Record any unsupported inputs in `todo.md`; do not change source ideas unless
  a separate durable initiative is discovered.

Completion check:
- `todo.md` states the owner files and the first concrete scalar record slice
  to implement.

### Step 2: Add Scalar ALU Target Record Vocabulary

Goal: define typed AArch64 scalar ALU target records for the first supported
integer operations.

Primary targets:
- `src/backend/mir/aarch64/codegen/records.hpp`
- `src/backend/mir/aarch64/codegen/records.cpp`
- `tests/backend/`

Actions:
- Add record-only scalar ALU operation kind and instruction record fields.
- Preserve BIR opcode, operand type, result type, destination value identity,
  and structured source operands.
- Add focused tests for representative supported ALU records.
- Add or extend guard coverage for unsupported/deferred scalar forms where the
  selected vocabulary does not apply.

Completion check:
- Backend subset proof passes and tests show typed scalar ALU records without
  assembly text, encoding, object output, or memory/call behavior.

### Step 3: Convert Prepared Scalar ALU Facts Into Target Records

Goal: build scalar ALU target records from structured BIR and prepared facts.

Primary targets:
- `src/backend/mir/aarch64/codegen/records.hpp`
- `src/backend/mir/aarch64/codegen/records.cpp`
- `tests/backend/`

Actions:
- Add conversion helpers that consume BIR scalar ALU instructions and prepared
  value/register facts.
- Preserve prepared ids and typed operands without parsing rendered names.
- Ensure unsupported scalar opcodes or incomplete prepared facts fail closed.
- Test representative successful conversion and unsupported/deferred cases.

Completion check:
- Backend subset proof passes and conversion tests demonstrate structured ALU
  target records sourced from prepared/BIR facts.

### Step 4: Add Simple Integer Cast Target Records

Goal: define and prove record-only target records for the first simple integer
cast operations.

Primary targets:
- `src/backend/mir/aarch64/codegen/records.hpp`
- `src/backend/mir/aarch64/codegen/records.cpp`
- `tests/backend/`

Actions:
- Add cast record vocabulary for the selected simple forms, such as sign
  extension, zero extension, or truncation where type facts are complete.
- Preserve source and destination value identities, source/result types, and
  prepared operand records.
- Test representative supported cast records and explicit deferred forms.

Completion check:
- Backend subset proof passes and cast tests show typed records with no generic
  placeholder lowering.

### Step 5: Convert Prepared Cast Facts Into Target Records

Goal: construct simple cast records from structured BIR cast instructions and
prepared facts.

Primary targets:
- `src/backend/mir/aarch64/codegen/records.hpp`
- `src/backend/mir/aarch64/codegen/records.cpp`
- `tests/backend/`

Actions:
- Add conversion helpers for selected BIR `CastInst` forms.
- Use structured type and prepared value/register facts as authority.
- Fail closed for unsupported casts, incomplete facts, f128/i128, or broad
  floating-point cases outside this idea.
- Test successful cast conversion plus explicit unsupported/deferred behavior.

Completion check:
- Backend subset proof passes and conversion tests cover the selected cast
  records and guard behavior.

### Step 6: Document Scalar Record Contract And Close Readiness

Goal: make the scalar ALU/cast target-record boundary explicit and prove the
source idea acceptance criteria.

Primary targets:
- `src/backend/mir/aarch64/codegen/records.md`
- `tests/backend/`
- `todo.md`

Actions:
- Document supported scalar ALU and cast record surfaces.
- Document deferred scalar opcodes and casts as unsupported, not silently
  lowered placeholders.
- Add or update a contract test if needed to assert record-only ownership and
  absence of assembly/object/memory/call behavior.
- Run the backend subset proof and leave close-readiness notes in `todo.md`.

Completion check:
- Backend subset proof passes, acceptance criteria from the source idea are
  satisfied, and `todo.md` is ready for supervisor close routing.
