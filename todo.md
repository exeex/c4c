# Current Packet

Status: Active
Source Idea Path: ideas/open/204_aarch64_prepared_module_mir_boundary.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Add Frame, Branch, Call, And Move Skeletons

## Just Finished

Step 5 added descriptive AArch64 frame, branch, call, move, ABI-binding,
spill/reload, and parallel-copy skeleton records for `plan.md` Step 5.
`module::FunctionRecord` now preserves representative prepared frame-slot,
stack-object, dynamic-stack, callee-save, compare-branch, call-plan,
value-location move, regalloc move, spill/reload, ABI-binding, and parallel-copy
carrier identities without selecting target instructions or emitting assembly.

## Suggested Next

Implement Step 6 by adding data/object side-table skeleton records from
prepared module-level data facts without starting assembly, object, relocation,
or binary emission.
Recommended next owned files:

- `src/backend/mir/aarch64/module/module.hpp`
- `src/backend/mir/aarch64/module/module.cpp`
- `tests/backend/backend_aarch64_prepared_data_identity_test.cpp`
- `tests/backend/CMakeLists.txt`

The next packet should preserve representative prepared globals, strings,
symbol visibility, TLS, constants, initializers, and future relocation identity
as descriptive MIR side-table records only.

## Watchouts

- Do not edit `ideas/open/204_aarch64_prepared_module_mir_boundary.md` unless
  source intent itself proves wrong.
- Do not add instruction selection, assembly text emission, assembler/object,
  linker, or executable production.
- Do not route the prepared MIR boundary through existing AArch64
  `codegen/emit.hpp`; it still exposes raw BIR/LIR/text paths and is outside
  the prepared MIR handoff.
- Do not recover semantic facts from rendered names, printed BIR, legacy LIR
  text, assembly strings, parser operands, or markdown examples.
- Do not weaken, skip, or reclassify tests to claim boundary progress.
- `PreparedMemoryAccess` still lacks explicit volatility/address-space fields;
  that is not a current boundary blocker, but do not start memory lowering from
  it.
- Step 3 records deliberately stop at module/function/block identity. Do not
  add instruction selection or assembly emission when extending them with
  operands/registers.
- Step 4 records intentionally keep physical register names in
  `TargetRegisterRecord` and semantic identities in `OperandRecord`; do not
  collapse those surfaces in the next packet.
- Regalloc physical assignments currently expose register class but no
  register-bank carrier; Step 4 preserves the class and leaves bank as `None`
  for those records.
- Step 5 records are descriptive only: route-local block/instruction indexes,
  physical register names, and parallel-copy step indexes remain prepared-route
  coordinates, not semantic identity or selected AArch64 instructions.
- Preserve unrelated dirty files and transient `review/` artifacts.

## Proof

Delegated Step 5 proof command:

```sh
cmake --build --preset default --target backend_aarch64_prepared_frame_control_test > test_after.log 2>&1 && ctest --test-dir build -R '^backend_aarch64_prepared_frame_control$' --output-on-failure >> test_after.log 2>&1
```

Proof log: `test_after.log`.
