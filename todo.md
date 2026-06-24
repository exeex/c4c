# Current Packet

Status: Active
Source Idea Path: ideas/open/334_object_route_scan_and_default_readiness.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Current Object Scan Surface

## Just Finished

- Step 1 inspected the current object-route scan surface without implementation
  edits.
- Current object selectors and representative tests:
  `backend_cli_.*obj` covers
  `backend_cli_riscv64_return_zero_writes_elf_obj`,
  `backend_cli_aarch64_return_zero_writes_elf_obj`,
  `backend_cli_x86_unsupported_target_diagnostic_obj`,
  `backend_cli_riscv64_unsupported_global_diagnostic_obj`, and
  `backend_cli_riscv64_requires_output_path_obj`; `backend_obj_runtime_.*`
  currently selects `backend_obj_runtime_rv64_return_zero`;
  `backend_riscv_object_emission`, `backend_aarch64_object_emission`, and
  `backend_object_model_records` cover target-owned/backend object model proof.
- Current asm/runtime/qemu/dual-route selectors and representative tests:
  `backend_rv64_runtime_return_zero` is the asm RV64 runtime baseline,
  nearby scalar asm runtime candidates include `backend_rv64_runtime_return_add`,
  `backend_rv64_runtime_two_arg_helper`,
  `backend_rv64_runtime_two_arg_local_arg`,
  `backend_rv64_runtime_two_arg_both_local_arg`,
  `backend_rv64_runtime_two_arg_second_local_arg`,
  `backend_rv64_runtime_local_arg_call`,
  `backend_rv64_runtime_return_add_sub_chain`, and
  `backend_rv64_runtime_local_temp`; AArch64 asm coexistence is represented by
  `backend_cli_aarch64_asm_external_return_zero_smoke`,
  `backend_cli_aarch64_asm_external_return_add_smoke`, and
  `backend_cli_aarch64_asm_external_return_add_sub_chain_smoke`.
- Labels currently available for selection include `object`, `backend_route`,
  `backend_runtime`, `rv64`, `riscv64`, `aarch64`, `qemu`, `case`, `smoke`,
  `cli`, `dual-route`, `output_contract`, `unsupported`, and
  `expected_failure`; backend helper labels are rooted through
  `c4c_set_backend_test_labels(...)`.
- Backend runtime helpers choose output form by script: RV64 asm runtime uses
  `run_backend_rv64_runtime_case.cmake` with `--codegen asm`, links the `.s`
  with clang, and runs through `qemu-riscv64`; RV64 object runtime uses
  `run_backend_rv64_object_runtime_case.cmake` with `--codegen obj -o .o`,
  verifies ELF/RV64 machine, links the `.o` directly, and runs through
  `qemu-riscv64`.
- Backend object byte helpers use `run_backend_codegen_object_case.cmake`
  (`--codegen obj`, ELF magic, target `e_machine`) and
  `run_backend_codegen_object_failure_case.cmake` for diagnostics/no-output
  checks.
- c-testsuite backend helpers do not yet support object output:
  `tests/c/external/c-testsuite/RunCase.cmake` and
  `tests/c/external/c-testsuite-aarch64-backend-runner.cmake` hard-code
  `--codegen asm`; c-testsuite frontend tests pipe LLVM IR to clang, and no
  `c_testsuite_(aarch64|riscv64)_backend_` object route tests are registered in
  the current build.
- First RV64 scan seam: extend the existing RV64 object runtime helper beside
  the asm runtime matrix for simple scalar/no-global cases, starting with
  `return_add.c`, `two_arg_helper.c`, `return_add_sub_chain.c`, and
  `local_temp.c` after the existing `return_zero.c` smoke.
- First AArch64 scan seam: extend object byte-only scan cases through
  `c4c_add_backend_codegen_object_test(...)` for scalar/no-global inputs,
  starting from the existing `aarch64_return_zero_smoke.c` and probing generic
  `return_add.c` / `return_add_sub_chain.c` only as object-byte ELF checks
  unless a reliable AArch64 runtime runner is explicitly introduced.

## Suggested Next

- Step 2 should add selectable backend object scan helpers/cases, not
  c-testsuite defaults. Likely owned files: `tests/backend/CMakeLists.txt`,
  `tests/backend/cmake/run_backend_rv64_object_runtime_case.cmake` only if the
  existing helper needs a small scan option, possibly
  `tests/backend/cmake/run_backend_codegen_object_case.cmake` only if object
  byte scan naming/output checks need shared helper support, plus `todo.md`.
- Recommended Step 2 proof command:

```sh
set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_riscv_object_emission|backend_aarch64_object_emission|backend_cli_.*obj|backend_obj_runtime_.*|backend_rv64_runtime_(return_zero|return_add|two_arg_helper|two_arg_local_arg|two_arg_both_local_arg|two_arg_second_local_arg|local_arg_call|return_add_sub_chain|local_temp)|backend_cli_aarch64_asm_external_return_(zero|add|add_sub_chain)_smoke|backend_codegen_route_riscv64_external_no_storage_main_emits_return_path)$' --output-on-failure) > test_after.log 2>&1
```

## Watchouts

- Do not weaken or remove asm-route coverage while adding object-route scan
  selectors.
- Do not mark broad failures unsupported as a substitute for triage.
- Keep c-testsuite default-output decisions out of the first inspection packet.
- Keep the first scan away from known unsupported/default-policy boundaries:
  RV64 globals/data in object output, x86 object output, object stdout,
  c-testsuite object defaults, and object `--backend-bir-stage semantic`.
- Do not make AArch64 runtime a Step 2 requirement unless the local runner path
  is as reliable as the existing RV64 qemu path; AArch64 object byte scans are
  the safer first expansion.
- If scalar object scans expose a distinct target object writer, relocation,
  or backend semantic gap, record/fix it narrowly or split a focused follow-up
  idea rather than downgrading broad scan expectations.

## Proof

- Inspection-only packet; no build required.
- Passed:

```sh
git diff --check
```
