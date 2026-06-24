# Current Packet

Status: Active
Source Idea Path: ideas/open/338_aarch64_object_emitter_local_frame_and_scalar_frontier.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add AArch64 Selected Local-Frame Memory Object Support

## Just Finished

- Completed Step 2 by adding semantic AArch64 selected local-frame object
  support for the smallest leaf fixed-frame case.
- `src/backend/mir/aarch64/codegen/object_emission.cpp` now accepts
  stack-adjust-only fixed frames without link-register preservation and keeps
  the existing link-register frame setup/teardown path unchanged.
- Added selected frame-slot scalar load/store object emission for prepared
  base-plus-offset `MemoryInstructionRecord` shapes:
  - 4-byte and 8-byte GPR loads from `[sp,#offset]`.
  - 4-byte and 8-byte GPR register stores to `[sp,#offset]`.
  - 4-byte and 8-byte immediate stores materialized through the established
    reserved MIR scratch GPR before the store.
- Added focused AArch64 object-emitter unit coverage for a leaf frame-slot
  sequence matching the selected record shape behind c-testsuite `00011.c`.
- Restored the selectable AArch64 object-byte smoke
  `backend_cli_aarch64_cts_00011_writes_elf_obj` with the existing c-testsuite
  object/CLI/dual-route/smoke/scan label style.

## Suggested Next

- Step 3 should add the selected scalar multiply object encoder for the
  isolated c-testsuite `00012.c` shape: selected `mul w0, w0, w9` between
  already-supported move/add/sub/return records.
- Suggested Step 3 owned files:
  `src/backend/mir/aarch64/codegen/object_emission.cpp`,
  `src/backend/mir/aarch64/codegen/object_emission.hpp` only if needed,
  `tests/backend/mir/backend_aarch64_object_emission_test.cpp`,
  `tests/backend/CMakeLists.txt`, and `todo.md`.
- Suggested Step 3 proof command:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_object_emission|backend_cli_aarch64_(cts_00001|cts_00002|cts_00011|cts_00012|return_zero|return_add|return_add_sub_chain|two_arg_helper)_writes_elf_obj|backend_cli_aarch64_asm_external_return_add_sub_chain_smoke)$' --output-on-failure) > test_after.log 2>&1`

## Watchouts

- Do not add expected-failure scan labels or weaken object/asm expectations.
- Keep AArch64 runtime, broad branch/control-flow, globals/data, pointers,
  aggregates, byval, indirect calls, RV64, x86, object stdout, c-testsuite
  defaults, and semantic-BIR object mode out of this child.
- `00003.c`, `param_slot.c`, and the `two_arg_*_rewrite.c` family still need
  saved-callee/local-call-frame expansion beyond the leaf frame-slot memory
  slice completed here.
- Selected scalar multiply for `00012.c` is still unsupported; keep it as a
  focused scalar encoder slice rather than blending it into broader local-frame
  or branch/default-readiness work.
- Parent 334 should resume only after this child repairs the AArch64 target
  object frontier or records a precise blocker.

## Proof

- Ran the delegated proof command:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_riscv_object_emission|backend_aarch64_object_emission|backend_cli_.*obj|backend_obj_runtime_.*|backend_rv64_runtime_(return_zero|return_add|two_arg_helper|two_arg_local_arg|two_arg_both_local_arg|two_arg_second_local_arg|local_arg_call|return_add_sub_chain|local_temp)|backend_cli_aarch64_asm_external_return_(zero|add|add_sub_chain)_smoke|backend_codegen_route_riscv64_external_no_storage_main_emits_return_path)$' --output-on-failure) > test_after.log 2>&1`
- Result: passed, 38/38 tests.
- Proof log: `test_after.log`.
