# AArch64 Target Register And Instruction Record Core Runbook

Status: Active
Source Idea: ideas/open/207_aarch64_target_register_and_instruction_record_core.md

## Purpose

Create the shared AArch64 target-local register, operand, and instruction
record vocabulary before later branch, scalar, memory, call, assembler, or
object slices add behavior.

## Goal

Give AArch64 codegen a typed place to carry prepared operands and target
instruction records without expanding `module/module.cpp` or reviving
assembly-text emitter shapes.

## Core Rule

Stay at the target-record layer: do not select real instructions, emit assembly,
encode objects, infer semantics from rendered names, or add target behavior to
`src/backend/mir/aarch64/module/module.cpp`.

## Read First

- `ideas/open/207_aarch64_target_register_and_instruction_record_core.md`
- `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md`
- `src/backend/mir/aarch64/abi/abi.hpp`
- `src/backend/mir/aarch64/abi/abi.cpp`
- `src/backend/mir/aarch64/module/module.hpp`
- `src/backend/mir/aarch64/module/module.cpp`
- `ref/claudes-c-compiler/src/backend/arm/codegen/README.md`
- `ref/claudes-c-compiler/src/backend/arm/codegen/emit.rs`
- `ref/claudes-c-compiler/src/backend/arm/codegen/prologue.rs`
- `ref/claudes-c-compiler/src/backend/arm/codegen/calls.rs`

## Current Targets

- Typed AArch64 register references for GP, SP, and FP/SIMD views.
- Special-register role helpers for frame pointer, link register, sret,
  indirect-call scratch, platform-reserved, stack pointer, and caller/callee
  saved groups.
- Target-local operand records for registers, immediates, prepared values,
  frame slots, symbols/link names, branch targets, and placeholder memory
  operands.
- Target-local instruction record containers in a `codegen/` or clearly named
  target-MIR owner, not `module/`.
- Conversion helpers that validate prepared register strings, banks, and
  classes into typed register references and fail closed on mismatches.

## Non-Goals

- Do not lower scalar, branch, memory, call, return, float, atomic, intrinsic,
  or vector operations.
- Do not emit assembly text, parse assembly, encode instructions, write
  objects, or link binaries.
- Do not implement AAPCS64 call/return/variadic policy beyond the register role
  vocabulary.
- Do not infer semantic identity from rendered names, printed BIR, old
  markdown, or assembly text.
- Do not copy ARM reference code mechanically.

## Working Model

- `module/` remains the prepared/BIR snapshot boundary.
- `abi/` should own register vocabulary unless the ledger proves a narrower
  boundary.
- `codegen/` or a dedicated target-MIR owner should hold target operand and
  instruction record containers.
- Prepared ids and typed prepared facts are authoritative; display strings are
  display only.

## Execution Rules

- Keep changes behavior-preserving until later instruction-selection ideas.
- Add record types and helpers in small compile-proven packets.
- Prefer focused tests for parsing/classification and representative prepared
  register preservation.
- Document placeholders as deferred surfaces for later branch, scalar, memory,
  call, return, assembler, or object slices.
- Fail closed for unknown physical register strings or mismatched
  bank/class/view combinations.

## Step 1: Confirm Owners And Existing Prepared Register Facts

Goal: establish the exact file boundaries and prepared-register inputs before
adding new target records.

Primary targets:
- `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md`
- `src/backend/mir/aarch64/abi/`
- `src/backend/mir/aarch64/codegen/`
- `src/backend/mir/aarch64/module/`

Actions:
- Inspect the ledger and current AArch64 headers for prepared register, value,
  frame, control-flow, storage, call, and memory fact carriers.
- Identify the current test style and the smallest compile/test command that
  proves new AArch64 ABI/codegen model types.
- Decide the concrete owner files for register vocabulary and target record
  containers, keeping `module/` as a snapshot boundary.

Completion check:
- The executor can name the intended owner files, proof command, and prepared
  inputs without editing implementation files outside those boundaries.

## Step 2: Add The Typed Register Vocabulary

Goal: define AArch64 register references and role helpers in the accepted
owner.

Primary targets:
- `src/backend/mir/aarch64/abi/abi.hpp`
- `src/backend/mir/aarch64/abi/abi.cpp`

Actions:
- Add typed representations for GP registers `x0`-`x30`, stack pointer, and
  `wN` 32-bit views.
- Add typed representations for FP/SIMD scalar and vector views such as `sN`,
  `dN`, `qN`, and `vN`.
- Add helper APIs for frame pointer, link register, stack pointer, sret,
  indirect-call scratch, platform-reserved, caller-saved, and callee-saved
  roles.
- Keep names and roles explicit enough for later call, prologue, branch,
  scalar, and memory slices to reuse.

Completion check:
- Focused compile proof or tests cover representative GP, SP, FP/SIMD, and
  special-role classification.

## Step 3: Add Prepared Register Conversion Helpers

Goal: validate existing prepared register facts into the typed AArch64
register vocabulary.

Primary targets:
- the owner chosen in Step 1, likely `src/backend/mir/aarch64/abi/`

Actions:
- Add parsing or conversion helpers for prepared physical register strings.
- Validate register bank/class/view information when available.
- Return explicit failure for unknown names, unsupported views, or mismatched
  bank/class combinations.
- Avoid defaulting unknown registers to x0, sp, or any other fallback.

Completion check:
- Tests or compile proof demonstrate successful conversion for representative
  prepared GP and FP/SIMD registers plus fail-closed behavior for invalid or
  mismatched inputs.

## Step 4: Add Target Operand Records

Goal: create the shared target-local operand layer consumed by later codegen
families.

Primary targets:
- `src/backend/mir/aarch64/codegen/` or the target-MIR owner chosen in Step 1

Actions:
- Define operand records for typed registers, immediates, prepared values,
  frame slots, symbols/link names, branch targets, and placeholder memory
  operands.
- Preserve prepared ids and typed facts as authority.
- Mark placeholder memory and branch-target records as record surfaces only,
  not lowering behavior.

Completion check:
- The new operand model compiles and exposes typed fields for the required
  prepared identities without parsing rendered text.

## Step 5: Add Target Instruction Record Containers

Goal: provide a typed instruction-record container without selecting concrete
AArch64 instructions.

Primary targets:
- `src/backend/mir/aarch64/codegen/` or the target-MIR owner chosen in Step 1

Actions:
- Define instruction record containers that can hold later branch, scalar,
  memory, call, return, assembler, and object slices.
- Keep the records generic enough to carry typed operands and opcode-family
  placeholders, but not so generic that unsupported behavior silently succeeds.
- Document which record variants are intentionally placeholders for later open
  ideas.

Completion check:
- The target instruction container compiles, is owned outside `module/`, and
  contains no assembly strings or concrete instruction selection behavior.

## Step 6: Add Focused Tests And Documentation Notes

Goal: prove the core model and leave clear guardrails for later ideas.

Primary targets:
- existing test locations identified in Step 1
- relevant AArch64 codegen or ABI docs if a local note is needed

Actions:
- Add focused tests for register parsing/classification and role helpers.
- Add representative proof that prepared register identity survives conversion
  into typed target records.
- Add concise documentation notes for deferred branch, scalar, memory, call,
  return, assembler, and object behavior when the code itself does not make
  that obvious.

Completion check:
- Fresh build or test proof passes.
- The final diff contains no assembly emission, instruction selection, object
  emission, linker behavior, or `module/module.cpp` target-selection growth.
