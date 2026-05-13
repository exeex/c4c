# Current Packet

Status: Active
Source Idea Path: ideas/open/204_aarch64_prepared_module_mir_boundary.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Operand And Register Skeletons

## Just Finished

Step 4 added the first AArch64 operand and target-register skeleton records for
`plan.md` Step 4. `module::FunctionRecord` now carries semantic
`OperandRecord`s keyed by prepared `PreparedValueId` plus prepared function and
value-name IDs, with value names, BIR types, value kinds, frame-slot IDs,
symbol/link-name IDs, pointer-base IDs, and carrier pointers preserved where
already published by prepared value-location, regalloc, and storage-plan facts.
Physical register spellings are represented as separate indexed
`TargetRegisterRecord`s, so home, assigned, spill-authority, and storage-plan
register references do not replace semantic value identity.

## Suggested Next

Implement Step 5 by adding frame, branch, call, and move skeleton records from
prepared frame/control/call/value-location/regalloc/storage-plan facts without
lowering them to target instructions.
Recommended next owned files:

- `src/backend/mir/aarch64/module/module.hpp`
- `src/backend/mir/aarch64/module/module.cpp`
- `tests/backend/backend_aarch64_prepared_frame_control_test.cpp`
- `tests/backend/CMakeLists.txt`

The next packet should preserve representative prepared frame slots, branch
labels, call plan records, and move/spill/reload identities as descriptive MIR
skeleton records only.

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
- Preserve unrelated dirty files and transient `review/` artifacts.

## Proof

Delegated Step 4 proof command:

```sh
cmake --build --preset default --target backend_aarch64_prepared_operand_identity_test > test_after.log 2>&1 && ctest --test-dir build -R '^backend_aarch64_prepared_operand_identity$' --output-on-failure >> test_after.log 2>&1
```

Proof log: `test_after.log`.
