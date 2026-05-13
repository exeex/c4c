# AArch64 Scalar ALU Cast First Instruction Slice

Status: Closed
Created: 2026-05-13
Closed: 2026-05-13

Depends On:
- `ideas/open/207_aarch64_target_register_and_instruction_record_core.md`
- `ideas/open/208_aarch64_branch_compare_target_mir_records.md`

## Goal

Implement the first tiny AArch64 scalar instruction-selection slice over the
accepted target-MIR record core: integer ALU and simple cast records only,
without assembly emission, assembler encoding, calls, memory lowering, or
object output.

The slice should prove that BIR scalar operations can be represented as typed
AArch64 target instruction records in the correct owner files.

## Why This Idea Exists

After the target register/instruction core and branch/compare records exist,
scalar ALU/cast is the smallest behavior slice that can demonstrate real
instruction selection without dragging in memory operands, AAPCS64 calls,
returns, or assembler output. The ARM reference ALU/cast notes can inform
operation coverage, but the implementation must use current BIR and prepared
records.

## Reference Inputs

- `src/backend/mir/aarch64/codegen/alu.md`
- `src/backend/mir/aarch64/codegen/cast_ops.md`
- `ref/claudes-c-compiler/src/backend/arm/codegen/alu.rs`
- `ref/claudes-c-compiler/src/backend/arm/codegen/cast_ops.rs`
- BIR scalar `BinaryInst`, `CastInst`, `SelectInst` where selected
- prepared value, storage, and register facts from `PreparedBirModule`

## In Scope

- Select a narrow set of scalar integer target instruction records for BIR
  operations such as add, sub, and simple bitwise or compare-adjacent forms
  selected by the runbook.
- Select a narrow set of simple integer cast target instruction records such
  as sign/zero extension or truncation when BIR/prepared type facts are
  complete.
- Preserve BIR opcode, operand type, result type, prepared value ids, and
  typed register/operand records.
- Add tests or compile proof that the selected scalar BIR operations produce
  target instruction records in the `codegen/` owner, not ad hoc module
  fields.
- Document unsupported scalar opcodes and casts as deferred, not silently
  lowered through generic placeholders.

## Out Of Scope

- Assembly text emission, instruction encoding, object output, or linker work.
- Memory loads/stores, calls, returns, variadic behavior, prologue/epilogue,
  atomics, intrinsics, inline asm, f128/i128, or broad floating-point lowering.
- Full ALU coverage in one slice.
- Register allocation policy changes; consume prepared register/value facts.
- Testcase-shaped instruction selection.

## Acceptance Criteria

- The selected scalar BIR operations lower into typed AArch64 target
  instruction records.
- The implementation lives in the `codegen/` owner chosen by the layout and
  core record ideas.
- Unsupported scalar/cast forms fail closed or remain explicitly deferred.
- Tests or compile proof cover at least representative successful scalar ALU
  and cast records plus an unsupported/deferred guard where appropriate.
- No assembly text, object emission, memory lowering, or call lowering is
  added.

## Closure Notes

Closed after the active runbook completed all six steps. The accepted
implementation added record-only AArch64 scalar ALU and simple integer cast
target records under `src/backend/mir/aarch64/codegen/`, prepared conversion
helpers for the selected forms, documentation, and contract tests.

The completed slice covers integer ALU `Add`, `Sub`, `And`, `Or`, and `Xor`
records plus simple integer cast `SExt`, `ZExt`, and `Trunc` records. Unsupported
scalar opcodes and cast forms remain explicit deferred or fail-closed cases.

Close validation used the backend proof scope:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') 2>&1 | tee test_after.log`

Regression guard passed with `--allow-non-decreasing-passed`: 128 passed before,
128 passed after, 0 failed.

## Reviewer Reject Signals

- The slice emits assembly strings or encodes instructions instead of target
  records.
- Scalar lowering parses rendered value names, printed BIR, old markdown, or
  legacy LIR text to recover operands.
- The implementation expands into memory, calls, returns, f128/i128, atomics,
  or broad floating-point support.
- Unsupported forms silently become no-op/default target records.
- The code lands in a convenient but wrong owner such as `module/module.cpp`.
