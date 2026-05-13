# Current Packet

Status: Active
Source Idea Path: ideas/open/204_aarch64_prepared_module_mir_boundary.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Define Target-Local MIR Module, Function, And Block Records

## Just Finished

Step 3 defined the first AArch64 target-local MIR module, function, and block
records for `plan.md` Step 3. `module::Module` now carries function records
keyed by prepared `FunctionNameId`, and each function carries block records
keyed by prepared `BlockLabelId`, with display labels kept as labels and source
prepared BIR/control-flow pointers preserved for inspection. The builder
populates records from `PreparedBirModule::control_flow` after the existing
handoff gate succeeds and uses structured BIR block ids when associating source
blocks. This refinement also makes structured BIR `link_name_id` authoritative
for source-function association when present, with rendered function names kept
as fallback/debug text only.

## Suggested Next

Implement Step 4 by adding the first AArch64 operand and register skeleton
records keyed by prepared value identities and target register references.
Recommended next owned files:

- `src/backend/mir/aarch64/module/module.hpp`
- `src/backend/mir/aarch64/module/module.cpp`
- `tests/backend/backend_aarch64_prepared_operand_identity_test.cpp`
- `tests/backend/CMakeLists.txt`

The next packet should populate representative operand records from prepared
value names/ids and introduce target register-reference records without
conflating semantic value identity with physical registers.

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
- Preserve unrelated dirty files and transient `review/` artifacts.

## Proof

Delegated Step 3 proof command:

```sh
cmake --build --preset default --target backend_aarch64_prepared_module_identity_test > test_after.log 2>&1 && ctest --test-dir build -R '^backend_aarch64_prepared_module_identity$' --output-on-failure >> test_after.log 2>&1
```

Proof log: `test_after.log`.
