Status: Active
Source Idea Path: ideas/open/207_aarch64_target_register_and_instruction_record_core.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Target Operand Records

# Current Packet

## Just Finished

Completed `plan.md` Step 4 by adding target-local AArch64 operand record
surfaces under `src/backend/mir/aarch64/codegen/records.hpp` and `.cpp`.

Work completed:
- Added target operand records for typed registers, immediates, prepared
  values, frame slots, symbols/link names, branch targets, and memory operands.
- Preserved prepared ids and typed facts as fields, including
  `PreparedValueId`, `ValueNameId`, `PreparedFrameSlotId`, `LinkNameId`,
  register bank/class, BIR type, address space, volatility, and offsets.
- Marked branch-target and memory operands with `RecordSurfaceKind::RecordOnly`
  so the Step 4 surface does not imply instruction selection or lowering.
- Added `backend_aarch64_target_operand_records` coverage and wired
  `records.cpp` into `c4c_backend`.

## Suggested Next

Step 5 packet: add target instruction record containers under the same
`src/backend/mir/aarch64/codegen/` owner, using the Step 4 operand payloads but
without selecting concrete AArch64 instructions or emitting assembly.

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

## Proof

Ran:

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') 2>&1 | tee test_after.log
```

Result: passed, including `backend_aarch64_register_vocabulary` and
`backend_aarch64_prepared_register_conversion`; new
`backend_aarch64_target_operand_records` is included and green. The delegated
backend subset reported 117 executed tests, 0 failed.

Proof log: `test_after.log`.
