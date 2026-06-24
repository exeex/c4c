# Current Packet

Status: Active
Source Idea Path: ideas/open/336_target_object_emitter_scalar_scan_expansion.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Hand Back To Object Route Scan

## Just Finished

- Step 5 recorded the final child 336 handoff to object-route scan/default
  readiness without implementation, test, or CMake changes.
- Child 336 expanded target-owned scalar object-emitter support enough to clear
  the first blocked scan candidate set:
  RV64 now covers immediate scalar returns, same-module scalar calls with
  small immediate GPR arguments, multi-arg scalar calls, and the `local_temp.c`
  frame-slot load/store shape; AArch64 now covers selected MOVZ immediate
  materialization, immediate ADD/SUB scalar instructions, same-module direct
  calls, selected GPR call-boundary moves, and the fixed-size link-register
  frame setup/teardown shape needed by `return_add.c`.
- The original blocked restored scan candidate proof now passes 28/28.
- Restored RV64 object/runtime scan coverage is selectable through
  `backend_obj_runtime_.*`: `backend_obj_runtime_rv64_return_zero`,
  `backend_obj_runtime_rv64_return_add`,
  `backend_obj_runtime_rv64_two_arg_helper`,
  `backend_obj_runtime_rv64_return_add_sub_chain`, and
  `backend_obj_runtime_rv64_local_temp`. The proof also keeps matching RV64
  asm/runtime scalar cases selected for coexistence.
- Restored AArch64 object-byte scan coverage is selectable through
  `backend_cli_.*obj`: `backend_cli_aarch64_return_zero_writes_elf_obj`,
  `backend_cli_aarch64_return_add_writes_elf_obj`, and
  `backend_cli_aarch64_return_add_sub_chain_writes_elf_obj`. The proof keeps
  AArch64 asm external return zero/add/add-sub-chain smokes selected for
  coexistence.

## Suggested Next

- Hand back to idea 334. Object-route scan/default-readiness can resume from a
  green first scalar scan baseline, using the restored RV64 object-runtime and
  AArch64 object-byte selectors above as the stable starting point.

## Watchouts

- Do not use textual assembly, external assemblers, or asm fallback to satisfy
  `--codegen obj`.
- Do not add expected-failure scan labels for the blocked scalar cases.
- Keep broad RV64 globals/data, x86 object output, c-testsuite defaults, object
  stdout, and object semantic-BIR mode out of this focused child.
- Remaining unsupported/default-policy boundaries are unchanged: RV64
  globals/data object output; broader RV64 memory, pointer, array, aggregate,
  and global cases; broader AArch64 memory/frame/call shapes; AArch64 object
  runtime; x86 object output; object stdout; c-testsuite object defaults; and
  object `--backend-bir-stage semantic` stay out of this child.

## Proof

- Passed:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_riscv_object_emission|backend_aarch64_object_emission|backend_cli_.*obj|backend_obj_runtime_.*|backend_rv64_runtime_(return_zero|return_add|two_arg_helper|two_arg_local_arg|two_arg_both_local_arg|two_arg_second_local_arg|local_arg_call|return_add_sub_chain|local_temp)|backend_cli_aarch64_asm_external_return_(zero|add|add_sub_chain)_smoke|backend_codegen_route_riscv64_external_no_storage_main_emits_return_path)$' --output-on-failure) > test_after.log 2>&1`
- Result: 28/28 tests passed.
- Proof log: `test_after.log`.
- Passed: `git diff --check`.
