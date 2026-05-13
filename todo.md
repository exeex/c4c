Status: Active
Source Idea Path: ideas/open/209_aarch64_scalar_alu_cast_first_instruction_slice.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Convert Prepared Scalar ALU Facts Into Target Records

# Current Packet

## Just Finished

Completed Step 2 from `plan.md`: added record-only scalar ALU target vocabulary
under `src/backend/mir/aarch64/codegen/records.hpp` and `.cpp`.

Concrete work completed:
- Added `ScalarAluOperationKind` for integer `Add`, `Sub`, `And`, `Or`, `Xor`,
  plus explicit `Deferred` for scalar forms outside this slice.
- Added `ScalarAluRecord` carrying `RecordSurfaceKind::RecordOnly`, source
  `bir::BinaryOpcode`, operand type, result type, result `PreparedValueId` /
  `ValueNameId`, and structured lhs/rhs `OperandRecord`s.
- Added helpers for generic diagnostic names, BIR opcode classification, and
  conversion from supported BIR binary opcodes to scalar ALU operation kinds.
- Added `make_scalar_alu_instruction_record(...)` so scalar ALU payloads also
  preserve the existing generic `ScalarInstructionRecord` fields.
- Added `backend_aarch64_scalar_alu_records` coverage for add/sub/bitwise
  records, diagnostic names, preserved BIR/prepared metadata, structured
  operands, and explicit deferred handling for out-of-scope scalar opcodes.

## Suggested Next

Execute Step 3 from `plan.md`: add prepared/BIR-to-target scalar ALU record
conversion for the Step 2 integer ALU vocabulary, including missing home/type
guard behavior without adding concrete opcode selection or emission.

## Watchouts

- Keep the next slice record-only; do not add assembly emission, encoding,
  object output, memory lowering, calls, or returns.
- Use structured BIR and prepared facts as authority; do not parse rendered
  names or printed BIR.
- Keep target record ownership under `src/backend/mir/aarch64/codegen/`.
- Do not route compare predicates through the scalar ALU owner; branch/compare
  records already own compare-adjacent facts.
- Step 2 deliberately introduced no prepared conversion helper; Step 3 should
  decide exact error/guard shape for missing homes and unsupported source
  opcodes.
- Keep simple casts deferred until Step 4/5 even though `SExt`, `ZExt`, and
  `Trunc` are visible in BIR today.

## Proof

Proof passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') 2>&1 | tee test_after.log`

Result: backend subset passed with `backend_aarch64_scalar_alu_records`
included and green. Proof log path: `test_after.log`.
