Status: Active
Source Idea Path: ideas/open/209_aarch64_scalar_alu_cast_first_instruction_slice.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Document Scalar Record Contract And Close Readiness

# Current Packet

## Just Finished

Completed Step 5 from `plan.md`: added prepared/BIR-to-target scalar cast
record conversion for simple integer `SExt`, `ZExt`, and `Trunc` forms.

Concrete work completed:
- Added `PreparedScalarCastRecordError`,
  `PreparedScalarCastRecordResult`, and
  `PreparedScalarCastInstructionRecordResult` record-only result surfaces.
- Added `make_prepared_scalar_cast_record(...)` and
  `make_prepared_scalar_cast_instruction_record(...)` using structured
  `bir::CastInst`, prepared value homes, and storage-plan facts.
- Reused the existing prepared scalar operand conversion shape for source
  operands, preserving `PreparedValueId`, `ValueNameId`, source/result types,
  source `bir::CastOpcode`, and typed `OperandRecord` payloads.
- Fail closed for unsupported cast opcodes, unsupported scalar types such as
  `i128`, missing result/source homes or storage facts, unsupported storage
  encodings, and prepared register conversion failures.
- Added `backend_aarch64_prepared_scalar_cast_records` coverage for successful
  `SExt`, `ZExt`, and `Trunc` conversion plus guard behavior.

## Suggested Next

Execute Step 6 from `plan.md`: document the scalar ALU/cast target-record
contract and add or update close-readiness proof that the slice remains
record-only.

## Watchouts

- Keep Step 6 record-only; do not add assembly emission, encoding, object
  output, memory lowering, calls, returns, or concrete opcode selection.
- Step 5 preserves source/result types as BIR facts but does not introduce
  concrete opcode selection or result-register records.
- Unsupported scalar ALU/cast forms are explicit closed/deferred record
  boundaries; contract documentation should keep that distinction visible.

## Proof

Proof passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') 2>&1 | tee test_after.log`

Result: backend subset passed with
`backend_aarch64_prepared_scalar_cast_records` included and green: 127 tests
passed, 0 failed; 12 disabled MIR trace tests were not run. Proof log path:
`test_after.log`.
