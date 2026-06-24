# Current Packet

Status: Active
Source Idea Path: ideas/open/336_target_object_emitter_scalar_scan_expansion.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Restore Scan Candidate Proof

## Just Finished

- Step 4 ran the restored object-route scan candidate proof without
  implementation, test, or CMake changes.
- The original blocked 28-test scan candidate subset now passes end to end.
- Restored RV64 object/runtime scan coverage is selectable through
  `backend_obj_runtime_.*` and includes `backend_obj_runtime_rv64_return_zero`,
  `backend_obj_runtime_rv64_return_add`,
  `backend_obj_runtime_rv64_two_arg_helper`,
  `backend_obj_runtime_rv64_return_add_sub_chain`, and
  `backend_obj_runtime_rv64_local_temp`. The proof also keeps the matching RV64
  asm/runtime scalar cases selected for coexistence.
- Restored AArch64 object-byte scan coverage is selectable through
  `backend_cli_.*obj` and includes
  `backend_cli_aarch64_return_zero_writes_elf_obj`,
  `backend_cli_aarch64_return_add_writes_elf_obj`, and
  `backend_cli_aarch64_return_add_sub_chain_writes_elf_obj`, with the
  AArch64 asm external return smokes selected for coexistence.

## Suggested Next

- Step 5 should hand the completed scalar object-emitter unblocking result back
  to idea 334's object-route scan/default-readiness work. The key handoff fact
  is that the formerly blocked first scan candidate subset is green and can be
  used as the baseline for broader object-route scan expansion.

## Watchouts

- Do not use textual assembly, external assemblers, or asm fallback to satisfy
  `--codegen obj`.
- Do not add expected-failure scan labels for the blocked scalar cases.
- Keep broad RV64 globals/data, x86 object output, c-testsuite defaults, object
  stdout, and object semantic-BIR mode out of this focused child.
- Remaining unsupported/default-policy boundaries are unchanged: RV64
  globals/data object output, broader AArch64 memory/frame/call shapes,
  AArch64 object runtime, x86 object output, object stdout, c-testsuite object
  defaults, and object `--backend-bir-stage semantic` stay out of this child.

## Proof

- Passed:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_riscv_object_emission|backend_aarch64_object_emission|backend_cli_.*obj|backend_obj_runtime_.*|backend_rv64_runtime_(return_zero|return_add|two_arg_helper|two_arg_local_arg|two_arg_both_local_arg|two_arg_second_local_arg|local_arg_call|return_add_sub_chain|local_temp)|backend_cli_aarch64_asm_external_return_(zero|add|add_sub_chain)_smoke|backend_codegen_route_riscv64_external_no_storage_main_emits_return_path)$' --output-on-failure) > test_after.log 2>&1`
- Result: 28/28 tests passed.
- Proof log: `test_after.log`.
- Passed: `git diff --check`.
