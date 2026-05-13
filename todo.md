# Current Packet

Status: Active
Source Idea Path: ideas/open/204_aarch64_prepared_module_mir_boundary.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Add Data/Object Side-Table Skeletons

## Just Finished

Step 6 added descriptive AArch64 data/object side-table skeleton records for
`plan.md` Step 6. `module::Module` now preserves representative prepared
globals, string constants, symbol visibility/declaration status, TLS,
constantness, scalar and aggregate initializers, and future relocation needs as
structured records without emitting assembly, object sections, relocations, or
binary output.

## Suggested Next

Implement Step 7 by consolidating boundary proof and preparing the completed
AArch64 prepared-module MIR boundary for reviewer scrutiny.
Recommended next owned files:

- `todo.md`
- `test_after.log`

The next packet should run a fresh focused boundary proof across the AArch64
prepared-module tests and any relevant backend prepare tests, then record the
green commands plus deferred-field notes for later target MIR, target ABI,
instruction-selection, assembler/object, or shared-preparation scope.

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
- Step 6 records are side tables only: relocation needs describe future object
  work but do not emit relocations, sections, assembly text, or bytes.
- Preserve unrelated dirty files and transient `review/` artifacts.

## Proof

Delegated Step 6 proof command:

```sh
cmake --build --preset default --target backend_aarch64_prepared_data_identity_test > test_after.log 2>&1 && ctest --test-dir build -R '^backend_aarch64_prepared_data_identity$' --output-on-failure >> test_after.log 2>&1
```

Proof log: `test_after.log`.
