# Current Packet

Status: Active
Source Idea Path: ideas/open/334_object_route_scan_and_default_readiness.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Resume Scan Baseline And Candidate Map

## Just Finished

- Completed Step 1 resumed scan inspection after child 336.
- Confirmed the restored RV64 object-runtime selectors in
  `tests/backend/CMakeLists.txt`: `backend_obj_runtime_rv64_return_zero`,
  `backend_obj_runtime_rv64_return_add`,
  `backend_obj_runtime_rv64_two_arg_helper`,
  `backend_obj_runtime_rv64_return_add_sub_chain`, and
  `backend_obj_runtime_rv64_local_temp`.
- Confirmed the restored AArch64 object-byte selectors:
  `backend_cli_aarch64_return_zero_writes_elf_obj`,
  `backend_cli_aarch64_return_add_writes_elf_obj`, and
  `backend_cli_aarch64_return_add_sub_chain_writes_elf_obj`.
- Inspected the next nearby scalar backend cases:
  `two_arg_local_arg.c`, `two_arg_second_local_arg.c`,
  `two_arg_both_local_arg.c`, and `local_arg_call.c` for RV64 runtime, plus
  `two_arg_helper.c`, `local_temp.c`, the local-arg variants, and
  `local_arg_call.c` for AArch64 object bytes.
- Non-repo dry runs with `./build/c4cll --codegen obj` show no conservative
  green scan additions should be wired yet:
  RV64 rejects all four local-plus-call candidates with
  `RISC-V backend object route unsupported prepared module shape`; AArch64
  rejects `two_arg_helper.c` and local-arg variants with
  `AArch64 object emission supports only selected immediate add/sub scalar instructions`
  and rejects `local_temp.c`/`local_arg_call.c` at the selected frame setup
  boundary.
- Inspected `tests/c/external` helper surfaces: c-testsuite backend mode still
  chooses `--codegen asm` in `tests/c/external/c-testsuite/RunCase.cmake`.
  There is no object-output c-testsuite helper or default policy seam ready for
  this first resumed scan packet.

## Suggested Next

- Do not add Step 2 CTest scan labels yet, because the obvious scalar
  candidates are already known-red at the target object-entrypoint.
- Recommended next lifecycle action is a plan-owner split or repair packet for
  focused target-owned object-emitter capability work before scan expansion:
  RV64 local-backed call arguments for `two_arg_local_arg.c`,
  `two_arg_second_local_arg.c`, `two_arg_both_local_arg.c`, and
  `local_arg_call.c`; AArch64 register-register scalar add/call-result support
  for `two_arg_helper.c`, with AArch64 local/frame memory left as a later
  boundary.
- If the supervisor still wants a Step 2 no-change baseline proof before that
  split, use:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_riscv_object_emission|backend_aarch64_object_emission|backend_cli_.*obj|backend_obj_runtime_.*|backend_rv64_runtime_(return_zero|return_add|two_arg_helper|two_arg_local_arg|two_arg_both_local_arg|two_arg_second_local_arg|local_arg_call|return_add_sub_chain|local_temp)|backend_cli_aarch64_asm_external_return_(zero|add|add_sub_chain)_smoke|backend_codegen_route_riscv64_external_no_storage_main_emits_return_path)$' --output-on-failure) > test_after.log 2>&1`
- Likely Step 2 owned files for an actual scan-add packet remain
  `tests/backend/CMakeLists.txt` and `todo.md`, with the existing
  `tests/backend/cmake/run_backend_rv64_object_runtime_case.cmake` and
  `tests/backend/cmake/run_backend_codegen_object_case.cmake` helpers only if
  a harness parameter gap appears. Current evidence says target emitter work,
  not harness wiring, is the blocker.

## Watchouts

- Keep asm-route coverage intact.
- Do not add expected-failure labels for cases that should be supported.
- The next RV64 cases are scalar/no-global but not scan-safe yet because they
  combine local homes with call arguments in the prepared object route.
- The next AArch64 no-local scalar call case is blocked on a selected
  register-register arithmetic/call-result instruction family; local AArch64
  candidates cross into broader frame/load/store support and should stay out of
  the first resumed scan packet.
- Keep RV64 globals/data, pointer/array/aggregate cases, AArch64 broad
  memory/frame/call expansion, AArch64 object runtime, x86 object output,
  object stdout, c-testsuite object defaults, and semantic-BIR object mode out
  unless the plan is explicitly revised.

## Proof

- Inspection-only packet; no build required by the delegated proof.
- Ran non-repo `./build/c4cll --codegen obj` dry runs for candidate mapping
  only; they did not write repository files.
- Ran `git diff --check`.
