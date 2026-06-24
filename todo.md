# Current Packet

Status: Active
Source Idea Path: ideas/open/337_target_object_emitter_local_call_and_regreg_scalar_expansion.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add AArch64 RegReg Scalar Object Support

## Just Finished

- Completed Step 3 AArch64 register-register scalar object support.
- Extended `src/backend/mir/aarch64/codegen/object_emission.cpp` so the
  selected scalar object-emitter path handles GPR register-register ADD/SUB
  records in addition to the existing immediate ADD/SUB forms. The new path
  encodes same-width selected GPR operands directly, covering the
  `two_arg_helper.c` shape `add w13, w0, w1` without assembly fallback.
- Preserved existing immediate add/sub, call-boundary move, fixed LR frame, and
  direct-call object emission behavior.
- Added a focused AArch64 object-emitter unit fixture for a selected
  same-module two-argument scalar helper. The fixture asserts the
  register-register ADD encoding, the surrounding call-boundary moves, and the
  direct-call relocation offset.
- Restored the AArch64 object-byte scan case
  `backend_cli_aarch64_two_arg_helper_writes_elf_obj` beside the existing
  `return_zero`, `return_add`, and `return_add_sub_chain` object-byte cases.

## Suggested Next

- Proceed to Step 4 restored scan validation for child 337.
- Step 4 should prove the combined restored object scan baseline, including the
  RV64 local-backed scalar runtime cases from Step 2 and the AArch64
  `two_arg_helper` object-byte case from Step 3, then hand back to idea 334 if
  the scan remains green.
- Suggested Step 4 proof command:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_riscv_object_emission|backend_aarch64_object_emission|backend_cli_.*obj|backend_obj_runtime_.*|backend_rv64_runtime_(return_zero|return_add|two_arg_helper|two_arg_local_arg|two_arg_both_local_arg|two_arg_second_local_arg|local_arg_call|return_add_sub_chain|local_temp)|backend_cli_aarch64_asm_external_return_(zero|add|add_sub_chain)_smoke|backend_codegen_route_riscv64_external_no_storage_main_emits_return_path)$' --output-on-failure) > test_after.log 2>&1`

## Watchouts

- Do not add expected-failure scan labels or weaken object/asm expectations.
- AArch64 `two_arg_helper.c` is object-byte proof only in this child; AArch64
  runtime remains out of scope.
- Keep RV64 globals/data, arrays, pointers, aggregates, broad memory/control
  flow, broad AArch64 frame/local-memory expansion, AArch64 runtime, x86 object
  output, object stdout, c-testsuite object defaults, and semantic-BIR object
  mode out of this child.
- Parent idea 334 should resume only after this child restores the next object
  scan candidates as green native object-output tests.

## Proof

- Ran exact delegated proof:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_object_emission|backend_cli_aarch64_(return_zero|return_add|return_add_sub_chain|two_arg_helper)_writes_elf_obj|backend_cli_aarch64_asm_external_return_add_smoke)$' --output-on-failure) > test_after.log 2>&1`
- Result: 6/6 tests passed. Proof log: `test_after.log`.
- Ran `git diff --check`; no whitespace errors.
