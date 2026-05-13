Status: Active
Source Idea Path: ideas/open/207_aarch64_target_register_and_instruction_record_core.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm Owners And Existing Prepared Register Facts

# Current Packet

## Just Finished

Completed `plan.md` Step 1 inspection for AArch64 target-register and
instruction-record owners.

Findings:
- Intended register-vocabulary owners: `src/backend/mir/aarch64/abi/abi.hpp`
  and `src/backend/mir/aarch64/abi/abi.cpp`. They currently only own target
  profile / AAPCS64 handoff validation, so Step 2 can add typed AArch64
  register references and role helpers there without expanding `module/`.
- Intended target record owners for later operand/instruction containers:
  add new files under `src/backend/mir/aarch64/codegen/`, likely a small
  `records.hpp` / `records.cpp` or similarly named target-MIR model. Existing
  `src/backend/mir/aarch64/codegen/*.md` files are legacy notes, not live C++
  owners.
- Existing snapshot boundary: `src/backend/mir/aarch64/module/module.hpp` and
  `.cpp` already copy prepared facts into `Module`, `FunctionRecord`,
  `OperandRecord`, `TargetRegisterRecord`, `FrameRecord`, `BranchRecord`,
  `CallRecord`, `MoveRecord`, `AbiBindingRecord`, `SpillReloadRecord`,
  `ParallelCopyRecord`, `GlobalDataRecord`, and `StringDataRecord`. Do not make
  this the new typed-register or instruction-selection owner.
- Prepared register/fact carriers: `src/backend/prealloc/prealloc.hpp` carries
  `PreparedRegisterBank`, `PreparedRegisterClass`,
  `PreparedRegisterGroupOverride`, `PreparedPhysicalRegisterAssignment`,
  `PreparedSavedRegister`, `PreparedValueHome`, `PreparedRegallocValue`,
  `PreparedStoragePlanValue`, `PreparedSpillReloadOp`,
  `PreparedCallArgumentPlan`, `PreparedCallResultPlan`,
  `PreparedCallPreservedValue`, `PreparedClobberedRegister`, and
  `PreparedIndirectCalleePlan`. Physical registers are still string spellings
  plus bank/class/group metadata.
- Prepared non-register fact carriers already available: `PreparedNameTables`,
  `PreparedStackLayout`, `PreparedFramePlan`, `PreparedDynamicStackPlan`,
  `PreparedControlFlow`, `PreparedBranchCondition`, `PreparedJoinTransfer`,
  `PreparedParallelCopyBundle`, `PreparedMoveBundle`, `PreparedAddressing`,
  `PreparedMemoryAccess`, `PreparedCallPlans`, `PreparedStoragePlans`,
  retained `PreparedBirModule::module`, `invariants`, and
  `completed_phases`.
- Ledger result: no shared BIR/prepared carrier split is required before Step
  2. The missing piece is target-local typing: AArch64 register references and
  later operand/instruction records. Current `module::TargetRegisterRecord`
  preserves prepared strings; it does not validate AArch64 register spelling,
  bank/class/view compatibility, or special-register roles.
- Current AArch64 test style: focused C++ executables in
  `tests/backend/CMakeLists.txt`, linked against `c4c_backend`, registered as
  CTest tests with labels `backend;internal;cpp;aarch64`. Existing examples are
  `backend_aarch64_prepared_handoff_gate`,
  `backend_aarch64_prepared_operand_identity`,
  `backend_aarch64_prepared_frame_control`,
  `backend_aarch64_prepared_data_identity`, and
  `backend_aarch64_prepared_module_identity`.

## Suggested Next

Step 2 packet: add the typed AArch64 register vocabulary and role helpers in
`src/backend/mir/aarch64/abi/abi.hpp` and `src/backend/mir/aarch64/abi/abi.cpp`,
with a focused test such as
`tests/backend/backend_aarch64_register_vocabulary_test.cpp` registered in
`tests/backend/CMakeLists.txt`.

Recommended smallest credible proof command for Step 2:

```sh
cmake --build build --target backend_aarch64_register_vocabulary_test && ctest --test-dir build -R '^backend_aarch64_register_vocabulary$' --output-on-failure
```

## Watchouts

- Keep `module/` as a prepared/BIR snapshot boundary.
- Do not add instruction selection, assembly emission, object output, or linker
  behavior.
- Do not infer semantics from rendered names or printed BIR.
- Fail closed on unknown or mismatched prepared register facts.
- Do not treat prepared physical-register strings as validated AArch64
  registers until Step 3 conversion helpers exist.
- Avoid extending `src/backend/mir/aarch64/codegen/emit.hpp`; it still exposes
  raw BIR/LIR text-era surfaces and is not the target-record owner for this
  plan.

## Proof

Not run; inspection-only packet. Per delegation, no compile proof was required
and no `test_before.log` or `test_after.log` was created.
