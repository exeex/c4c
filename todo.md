Status: Active
Source Idea Path: ideas/open/207_aarch64_target_register_and_instruction_record_core.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Add Focused Tests And Documentation Notes

# Current Packet

## Just Finished

Completed `plan.md` Step 6 by adding focused final proof and documentation
guardrails for the AArch64 target register and instruction record core.

Work completed:
- Added `backend_aarch64_target_record_core_contract` coverage proving a
  prepared physical register converts into a typed AArch64 register and keeps
  its prepared value id, value name, class, bank, expected view, and width
  through target register, operand, scalar instruction, and memory records.
- Covered deferred branch, call, return, assembler, and object instruction
  families as explicit `RecordSurfaceKind::RecordOnly` containers.
- Added `src/backend/mir/aarch64/codegen/records.md` guardrails documenting
  branch, scalar, memory, call, return, assembler, and object behavior as
  deferred record-only surfaces.
- Added short header comments clarifying that this layer does not select
  concrete AArch64 opcodes, emit assembly, encode objects, or imply linker
  behavior.

## Suggested Next

Lifecycle close/review handoff: the Step 6 runbook slice is complete. Ask the
plan owner to decide whether to close, deactivate, or replace the active plan
for `ideas/open/207_aarch64_target_register_and_instruction_record_core.md`.

## Watchouts

- Keep `module/` as a prepared/BIR snapshot boundary.
- Do not add instruction selection, assembly emission, object output, or linker
  behavior.
- Prepared register conversion accepts aggregate-address class/bank metadata for
  GP physical registers because those prepared facts still name AArch64 address
  registers.
- The conversion layer rejects uppercase aliases, zero-padded names, `x31`, and
  prepared `sp` with GP/FPR/VREG bank metadata.
- `x18` is exposed as platform-reserved and is excluded from caller/callee
  saved groups.
- Avoid extending `src/backend/mir/aarch64/codegen/emit.hpp`; it still exposes
  raw BIR/LIR text-era surfaces and is not the target-record owner for this
  plan.
- `records.hpp` is a target-record type surface only. It deliberately does not
  parse prepared display strings, inspect retained BIR instructions, choose
  opcodes, or format assembly.
- Memory operands are placeholders that preserve prepared address facts for
  later lowering; branch targets are structured `BlockLabelId` records only.
- Instruction records remain `RecordSurfaceKind::RecordOnly`; even when they
  carry BIR source opcodes or calling convention metadata, that does not imply
  selected AArch64 opcodes or valid assembly/object output.
- Step 6 added final contract documentation and proof only; it did not consume
  target records from `module.cpp`, `emit.hpp`, assembler, object, or linker
  surfaces.

## Proof

Ran:

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') 2>&1 | tee test_after.log
```

Result: passed. New `backend_aarch64_target_record_core_contract` is included
and green. The delegated backend subset reported 119 executed tests, 0 failed.

Proof log: `test_after.log`.
