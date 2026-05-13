Status: Active
Source Idea Path: ideas/open/209_aarch64_scalar_alu_cast_first_instruction_slice.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Simple Integer Cast Target Records

# Current Packet

## Just Finished

Completed Step 3 from `plan.md`: added prepared/BIR-to-target scalar ALU record
conversion under `src/backend/mir/aarch64/codegen/records.hpp` and `.cpp`.

Concrete work completed:
- Added explicit `PreparedScalarAluRecordError` diagnostics and result wrappers
  for fail-closed prepared scalar ALU conversion.
- Added `make_prepared_scalar_alu_record(...)` and
  `make_prepared_scalar_alu_instruction_record(...)` consuming structured
  `bir::BinaryInst`, `PreparedNameTables`, `PreparedValueLocationFunction`, and
  `PreparedStoragePlanFunction` facts.
- Supported integer `Add`, `Sub`, `And`, `Or`, and `Xor` conversion preserves
  BIR opcode, operand/result types, result `PreparedValueId` / `ValueNameId`,
  and typed register or rematerialized-immediate source `OperandRecord`s.
- Unsupported opcodes and compare predicates, missing result homes, missing
  source storage, unsupported frame-slot source storage, unsupported operand
  types, and register view mismatches fail closed with explicit errors.
- Added `backend_aarch64_prepared_scalar_alu_records` coverage for successful
  conversion across the Step 2 vocabulary plus guard behavior.

## Suggested Next

Execute Step 4 from `plan.md`: add record-only simple integer cast vocabulary
for selected `SExt`, `ZExt`, and `Trunc` forms, preserving prepared ids,
source/result types, and structured source operands without adding emission or
concrete opcode selection.

## Watchouts

- Keep the next slice record-only; do not add assembly emission, encoding,
  object output, memory lowering, calls, or returns.
- Reuse the Step 3 prepared operand guard shape for casts where practical, but
  keep cast-specific unsupported type/opcode errors distinct if needed.
- Keep target record ownership under `src/backend/mir/aarch64/codegen/`.
- Do not route compare predicates through the scalar ALU owner; branch/compare
  records already own compare-adjacent facts.
- Step 3 supports register operands and rematerialized immediate operands only;
  frame-slot, computed-address, symbol, and mismatched register facts currently
  fail closed instead of synthesizing loads or moves.

## Proof

Proof passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') 2>&1 | tee test_after.log`

Result: backend subset passed with `backend_aarch64_prepared_scalar_alu_records`
included and green: 125 tests passed, 0 failed; 12 disabled MIR trace tests
were not run. Proof log path: `test_after.log`.
