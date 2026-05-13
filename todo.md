Status: Active
Source Idea Path: ideas/open/209_aarch64_scalar_alu_cast_first_instruction_slice.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Convert Prepared Cast Facts Into Target Records

# Current Packet

## Just Finished

Completed Step 4 from `plan.md`: added record-only simple integer cast target
record vocabulary under `src/backend/mir/aarch64/codegen/records.hpp` and
`.cpp`.

Concrete work completed:
- Added `ScalarCastOperationKind` vocabulary for `SignExtend`, `ZeroExtend`,
  `Truncate`, and explicit `Deferred` cast forms.
- Added simple integer cast helpers:
  `is_simple_integer_cast_opcode(...)`,
  `scalar_cast_operation_from_cast_opcode(...)`, and
  `scalar_cast_operation_kind_name(...)`.
- Added `ScalarCastRecord` and `make_scalar_cast_instruction_record(...)`
  preserving source `bir::CastOpcode`, source/result types, result
  `PreparedValueId` / `ValueNameId`, and a structured source `OperandRecord`.
- Extended `ScalarInstructionRecord` with an optional `scalar_cast` payload
  without changing prepared conversion or concrete instruction selection.
- Added `backend_aarch64_scalar_cast_records` coverage for representative
  `SExt`, `ZExt`, and `Trunc` records plus explicit deferred diagnostics for
  unsupported cast forms.

## Suggested Next

Execute Step 5 from `plan.md`: add prepared/BIR-to-target scalar cast record
conversion for the Step 4 simple integer cast vocabulary, reusing the existing
prepared value-home and storage-plan guard shape where it applies.

## Watchouts

- Keep Step 5 record-only; do not add assembly emission, encoding, object
  output, memory lowering, calls, returns, or concrete opcode selection.
- Step 4 added vocabulary and instruction wrapping only; there is intentionally
  no prepared cast conversion yet.
- Cast conversion should preserve source/result types separately; do not infer
  source width from the result type.
- Unsupported cast opcodes currently map to `ScalarCastOperationKind::Deferred`
  and should fail closed during prepared conversion rather than being treated as
  generic supported scalar work.

## Proof

Proof passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') 2>&1 | tee test_after.log`

Result: backend subset passed with `backend_aarch64_scalar_cast_records`
included and green: 126 tests passed, 0 failed; 12 disabled MIR trace tests
were not run. Proof log path: `test_after.log`.
