Status: Active
Source Idea Path: ideas/open/209_aarch64_scalar_alu_cast_first_instruction_slice.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm Scalar Inputs And Existing Record Owners

# Current Packet

## Just Finished

Completed Step 1 from `plan.md`: inspected the scalar BIR, prepared fact, and
AArch64 target-record surfaces for the first scalar ALU/cast slice.

Owner files:
- Target record owner: `src/backend/mir/aarch64/codegen/records.hpp` and
  `src/backend/mir/aarch64/codegen/records.cpp`.
- Target record contract docs: `src/backend/mir/aarch64/codegen/records.md`;
  legacy guidance only lives in `codegen/alu.md` and `codegen/cast_ops.md`.
- Prepared/BIR snapshot owner: `src/backend/mir/aarch64/module/`; do not add
  target scalar records there.
- Prepared fact owner: `src/backend/prealloc/prealloc.hpp` and related
  preparation implementation.
- Tests should follow the existing backend AArch64 record tests under
  `tests/backend/` and register through `tests/backend/CMakeLists.txt`.

Available structured BIR scalar inputs:
- `bir::BinaryInst` carries `opcode`, `result`, `operand_type`, `lhs`, and
  `rhs`.
- Supported candidate ALU opcodes for the first record vocabulary are
  `Add`, `Sub`, `And`, `Or`, and `Xor`; `Mul` can be recordable but is a larger
  semantic step if the first slice wants the smallest proof.
- Compare opcodes are already owned by the branch/compare record surface and
  should not be re-owned by scalar ALU in Step 2.
- Shifts, div/rem, select-adjacent behavior, floating-point operations, i128,
  memory, calls, and returns should remain deferred for this plan stage.
- `bir::CastInst` carries `opcode`, `result`, and `operand`; simple cast
  candidates are `SExt`, `ZExt`, and `Trunc`, but cast vocabulary is Step 4/5
  after the ALU record slice.

Available prepared value/register/type facts:
- `PreparedBirModule::module` retains structured BIR functions, blocks, and
  instructions.
- `PreparedBirModule::names` and interned `FunctionNameId` / `ValueNameId`
  remain the semantic name authority; do not parse rendered BIR names.
- `PreparedValueLocationFunction::value_homes` and
  `find_prepared_value_home(...)` provide `PreparedValueId`, value names,
  home kind, register name, stack slot, rematerializable immediate, and
  pointer-base-plus-offset facts.
- `PreparedStoragePlanFunction::values` provides storage encoding, bank,
  contiguous width, physical register spelling, occupied register names,
  frame slot, stack offset, and immediate facts keyed by `PreparedValueId`.
- `PreparedRegallocValue` and `PreparedRegisterGroupOverride` provide register
  class, register group width, allocation status, and assigned physical
  register facts.
- AArch64 typed register conversion already lives in `abi/abi.hpp` and
  `abi/abi.cpp` through `RegisterReference`, `RegisterView`, and prepared
  register assignment conversion helpers.
- BIR `Value::type`, `BinaryInst::operand_type`, and `CastInst` result/operand
  values are the available type facts for scalar records.

First concrete Step 2 slice recommended:
- Add record-only scalar ALU vocabulary in `codegen/records.hpp` / `.cpp` for
  integer `Add`, `Sub`, `And`, `Or`, and `Xor`.
- Keep it vocabulary-only: explicit operation kind, result identity/type,
  destination/result prepared id/name, two structured source operands, source
  `bir::BinaryOpcode`, operand type, and `RecordSurfaceKind::RecordOnly`.
- Reuse existing `OperandRecord` / `PreparedValueOperand` /
  `ImmediateOperand` for inputs where possible; do not add conversion from
  `PreparedBirModule` until Step 3.
- Add focused tests such as
  `tests/backend/backend_aarch64_scalar_alu_records_test.cpp` proving
  representative add/sub/bitwise records, generic diagnostic names, preserved
  BIR opcode/type metadata, and absence of assembly/object behavior.

## Suggested Next

Execute Step 2 from `plan.md`: add the scalar ALU target record vocabulary for
integer `Add`, `Sub`, `And`, `Or`, and `Xor` in `codegen/records.hpp` /
`.cpp`, plus focused backend record tests.

## Watchouts

- Keep this slice record-only; do not add assembly emission, encoding, object
  output, memory lowering, calls, or returns.
- Use structured BIR and prepared facts as authority; do not parse rendered
  names or printed BIR.
- Keep target record ownership under `src/backend/mir/aarch64/codegen/`.
- Do not route compare predicates through the scalar ALU owner; branch/compare
  records already own compare-adjacent facts.
- For Step 2, avoid prepared conversion helpers; missing homes/storage facts
  are Step 3 guard behavior.
- Keep simple casts deferred until Step 4/5 even though `SExt`, `ZExt`, and
  `Trunc` are visible in BIR today.

## Proof

No compile proof required or run for this inspection-only packet. No
`test_before.log` or `test_after.log` was created or modified.

Recommended Step 2 proof command:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') 2>&1 | tee test_after.log`
