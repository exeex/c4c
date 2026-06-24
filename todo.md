# Current Packet

Status: Active
Source Idea Path: ideas/open/337_target_object_emitter_local_call_and_regreg_scalar_expansion.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add RV64 Local-Backed Scalar Call Object Support

## Just Finished

- Completed Step 2 RV64 local-backed scalar call object support.
- Extended `src/backend/mir/riscv/codegen/object_emission.cpp` so prepared RV64
  object functions can combine a small local frame with a direct call frame:
  the call frame now reserves local-frame bytes plus the saved-`ra` area and
  saves/restores `ra` above the local frame instead of overlapping local slots.
- Added RV64 register-sourced GPR call argument materialization in
  `fragment_for_prepared_call(...)` using target-native `addi rd, rs, 0`
  moves beside the existing immediate argument path.
- Fixed prepared frame-slot object access to add the prepared frame-slot base
  offset to the access-local byte offset. This preserves single-slot
  `local_temp.c` behavior and correctly handles `two_arg_both_local_arg.c`
  where both slot-relative accesses have offset 0 but slot #1 starts at frame
  offset 4.
- Added a focused RV64 object-emitter unit fixture covering a local frame plus
  register-sourced same-module call args (`t0 -> a0`, `s1 -> a1`) and result
  handling.
- Restored the four RV64 object runtime scan cases:
  `backend_obj_runtime_rv64_two_arg_local_arg`,
  `backend_obj_runtime_rv64_two_arg_second_local_arg`,
  `backend_obj_runtime_rv64_two_arg_both_local_arg`, and
  `backend_obj_runtime_rv64_local_arg_call`.

## Suggested Next

- Proceed to Step 3 for AArch64 register-register scalar object support.
- Recommended Step 3 owned files: `src/backend/mir/aarch64/codegen/object_emission.cpp`,
  `src/backend/mir/aarch64/codegen/object_emission.hpp` only if needed,
  `tests/backend/mir/backend_aarch64_object_emission_test.cpp`,
  `tests/backend/CMakeLists.txt`, and `todo.md`.
- Step 3 target seam remains
  `scalar_add_sub_immediate_fragment(...)` /
  `fragment_for_machine_instruction(...)`: add selected register-register
  scalar add support for the `two_arg_helper.c` shape (`add w13, w0, w1`) and
  restore `backend_cli_aarch64_two_arg_helper_writes_elf_obj`.
- Suggested Step 3 proof command:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_object_emission|backend_cli_aarch64_(return_zero|return_add|return_add_sub_chain|two_arg_helper)_writes_elf_obj|backend_cli_aarch64_asm_external_return_add_smoke)$' --output-on-failure) > test_after.log 2>&1`

## Watchouts

- Do not add expected-failure scan labels or weaken object/asm expectations.
- RV64 Step 2 proof is green for the restored object runtime scan subset; do
  not revisit RV64 in Step 3 unless a compile-only interaction appears.
- AArch64 `two_arg_helper.c` needs only object-byte proof in this child, not
  AArch64 runtime.
- Keep RV64 globals/data, arrays, pointers, aggregates, broad memory/control
  flow, broad AArch64 frame/local-memory expansion, AArch64 runtime, x86 object
  output, object stdout, c-testsuite object defaults, and semantic-BIR object
  mode out of this child.
- Parent idea 334 should resume only after this child restores the next object
  scan candidates as green native object-output tests.

## Proof

- Ran exact delegated proof:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_riscv_object_emission|backend_obj_runtime_rv64_(return_zero|return_add|two_arg_helper|return_add_sub_chain|local_temp|two_arg_local_arg|two_arg_second_local_arg|two_arg_both_local_arg|local_arg_call)|backend_rv64_runtime_(two_arg_local_arg|two_arg_second_local_arg|two_arg_both_local_arg|local_arg_call))$' --output-on-failure) > test_after.log 2>&1`
- Result: 14/14 tests passed. Proof log: `test_after.log`.
