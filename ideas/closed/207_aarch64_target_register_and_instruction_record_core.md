# AArch64 Target Register And Instruction Record Core

Status: Closed
Created: 2026-05-13
Closed: 2026-05-13

Depends On:
- `ideas/closed/205_aarch64_arm_reference_layout_contract.md`
- `ideas/closed/206_prepared_memory_volatility_address_space_carrier.md`

## Goal

Introduce the AArch64 target-local register vocabulary and instruction/operand
record core before any instruction selection, memory lowering, call lowering,
assembly printing, or object emission grows.

The core must consume the current BIR / `PreparedBirModule` boundary defined by
ideas 204-206 and give later codegen slices a typed place to put target
operands and instructions instead of expanding `module/module.cpp` or reviving
old text-emitter shapes.

## Why This Idea Exists

The layout contract says semantic MIR facts enter AArch64 through BIR retained
inside `PreparedBirModule`, supplemented by prepared frame, control-flow,
value-location, regalloc, storage, call, and memory facts. But AArch64 still
lacks typed target register references and a target instruction/operand record
surface.

Without that core, the first scalar, branch, memory, or call slice would invent
local operand formats in whichever file is convenient. This idea creates the
shared target vocabulary first, using the ARM reference backend only as a
register/role guide, not as a text-emitter route.

## Reference Inputs

- `ref/claudes-c-compiler/src/backend/arm/codegen/README.md`
- `ref/claudes-c-compiler/src/backend/arm/codegen/emit.rs`
- `ref/claudes-c-compiler/src/backend/arm/codegen/prologue.rs`
- `ref/claudes-c-compiler/src/backend/arm/codegen/calls.rs`
- `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md`
- current `src/backend/mir/aarch64/abi/`, `api/`, and `module/`

## In Scope

- Define a typed AArch64 register model for:
  - GP registers `x0`-`x30`, `sp`, and `wN` 32-bit views
  - FP/SIMD scalar and vector views such as `sN`, `dN`, `qN`, and `vN`
  - special roles such as frame pointer, link register, sret register,
    indirect-call scratch, platform-reserved register, stack pointer, and
    caller/callee-saved groups
- Place register ownership in `abi/` unless the layout ledger proves a
  narrower file boundary is better.
- Define target-local operand records for registers, immediates, prepared
  values, frame slots, symbols/link names, branch targets, and placeholder
  memory operands.
- Define target-local instruction record containers in `codegen/` or a clearly
  named target-MIR directory, while keeping `module/` as prepared/BIR snapshot
  ownership.
- Add conversion helpers from prepared register strings/banks/classes into the
  typed register vocabulary, failing closed on unknown or mismatched registers.
- Add focused tests proving register parsing/classification, special-register
  roles, and representative prepared-register preservation.
- Document which target operand/instruction records are placeholders for later
  branch, scalar, memory, call, return, assembler, or object slices.

## Out Of Scope

- Selecting scalar, branch, memory, call, return, float, atomic, intrinsic, or
  vector instructions.
- Emitting assembly text, parsing assembly, encoding instructions, writing
  objects, or linking binaries.
- Implementing AAPCS64 call/return/variadic policy beyond register role
  vocabulary.
- Inferring semantic identity from rendered names, printed BIR, old markdown,
  or assembly text.
- Expanding `module/module.cpp` with instruction-selection behavior.

## Acceptance Criteria

- A typed AArch64 register model exists and covers GP, FP/SIMD, stack pointer,
  and special-role registers needed by the first backend slices.
- Target operand and instruction record types exist in the owner files chosen
  by the layout contract.
- Prepared register strings/banks/classes can be validated into typed AArch64
  register references or fail closed.
- `module/` remains a prepared/BIR snapshot boundary and does not become the
  owner of instruction-selection behavior.
- Focused tests or compile proof cover register role classification and at
  least representative prepared-register preservation.
- No assembly text, instruction selection, object emission, or linker behavior
  is added.

## Closure Notes

Completed by the active runbook covering typed AArch64 register vocabulary,
prepared register conversion, target operand records, target instruction
record containers, focused tests, and local record-layer documentation.

Close-time backend regression guard passed with 119 passed, 0 failed, using
the existing accepted `test_before.log` and a fresh matching `test_after.log`.

## Reviewer Reject Signals

- The slice emits assembly or chooses real instructions while claiming to add
  only the core record model.
- Target operands recover semantic identity from rendered value/block/function
  names instead of prepared ids.
- Unknown physical register strings silently map to a default register.
- Register role policy is hard-coded ad hoc inside scalar, branch, memory, or
  call code instead of the accepted owner.
- `module/module.cpp` grows target instruction-selection behavior.
- The ARM reference code is copied mechanically without mapping it to the C++
  prepared-module contract.
