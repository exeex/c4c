Status: Active
Source Idea Path: ideas/open/207_aarch64_target_register_and_instruction_record_core.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Add Target Instruction Record Containers

# Current Packet

## Just Finished

Completed `plan.md` Step 5 by adding target-local AArch64 instruction record
containers under `src/backend/mir/aarch64/codegen/records.hpp` and `.cpp`.

Work completed:
- Added explicit record-only instruction families for branch, scalar, memory,
  call, return, assembler, and object placeholder slices.
- Reused Step 4 typed operand records inside instruction payloads so prepared
  value ids, value names, frame slots, link names, block labels, and BIR type
  facts can flow into later lowering work without parsing display strings.
- Preserved BIR source facts only as record metadata, such as scalar binary/cast
  opcodes, memory load/store kind, and call calling convention; no concrete
  AArch64 opcode, assembly spelling, object output, or linker behavior was
  selected.
- Added `backend_aarch64_target_instruction_records` coverage and wired it into
  the backend test set.

## Suggested Next

Step 6 packet: begin the next target-record slice by consuming these record-only
instruction containers at the target-record boundary selected by the supervisor,
without widening into assembly emission, object/linker output, or concrete
instruction selection unless that step explicitly owns it.

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

## Proof

Ran:

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') 2>&1 | tee test_after.log
```

Result: passed. New `backend_aarch64_target_instruction_records` is included
and green. The delegated backend subset reported 118 executed tests, 0 failed.

Proof log: `test_after.log`.
