# Current Packet

Status: Active
Source Idea Path: ideas/open/334_object_route_scan_and_default_readiness.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Next Selectable Object Scan Cases

## Just Finished

- Completed Step 2 by adding opt-in c-testsuite native object smoke
  registrations without changing c-testsuite defaults.
- Added RV64 object-runtime scan tests for the first two c-testsuite allowlist
  sources:
  `backend_obj_runtime_rv64_cts_00001` for
  `tests/c/external/c-testsuite/src/00001.c` and
  `backend_obj_runtime_rv64_cts_00002` for
  `tests/c/external/c-testsuite/src/00002.c`. Both reuse the existing
  `c4c_add_backend_rv64_object_runtime_case(...)` helper, emit native object
  bytes, link with the RV64 clang/qemu path, and expect exit code 0.
- Added AArch64 object-byte scan tests for the same sources:
  `backend_cli_aarch64_cts_00001_writes_elf_obj` and
  `backend_cli_aarch64_cts_00002_writes_elf_obj`. Both reuse the existing
  `c4c_add_backend_codegen_object_test(...)` helper and verify ELF machine
  `b700`.
- Added selectable `c_testsuite`, `smoke`, and `scan` labels to the new tests
  while preserving helper-added route labels such as `object`, `backend_route`,
  `backend_runtime`, `rv64`, `qemu`, `case`, `cli`, and `dual-route`.

## Suggested Next

- Proceed to Step 3 triage of remaining object-route gaps before adding more
  green scan registrations.
- Suggested Step 3 focus: inspect representative known-red families from Step
  1, classify each by owner, and decide whether to split a focused child idea
  before more scan expansion.
- Suggested Step 3 owned file: `todo.md` only unless a plan-owner split is
  requested after triage.
- Suggested Step 3 proof: no build required for triage-only; run
  `git diff --check` after updating `todo.md`.

## Watchouts

- Do not add expected-failure scan labels or weaken object/asm expectations.
- The c-testsuite object smokes added here are explicit backend tests under
  `tests/backend/CMakeLists.txt`; c-testsuite's own default/frontend and
  backend-asm helpers were not changed.
- Keep backend scalar `param_slot.c`, `two_arg_*_rewrite.c`, branch/control
  flow, local arrays/pointers, globals/data, aggregates, broad AArch64
  frame/local-memory expansion, AArch64 runtime, x86 object output, object
  stdout, broader c-testsuite object defaults, and semantic-BIR object mode out
  of scan-registration packets unless Step 3 triage points to a focused child.
- Label caveat remains: avoid bare `ctest -L object` for proof selection
  because it also matches labels containing `object`, including
  `aggregate_subobject`.

## Proof

- Ran exact delegated proof:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_riscv_object_emission|backend_aarch64_object_emission|backend_cli_.*obj|backend_obj_runtime_.*|backend_rv64_runtime_(return_zero|return_add|two_arg_helper|two_arg_local_arg|two_arg_both_local_arg|two_arg_second_local_arg|local_arg_call|return_add_sub_chain|local_temp)|backend_cli_aarch64_asm_external_return_(zero|add|add_sub_chain)_smoke|backend_codegen_route_riscv64_external_no_storage_main_emits_return_path)$' --output-on-failure) > test_after.log 2>&1`
- Result: 37/37 tests passed. Proof log: `test_after.log`.
- Ran `git diff --check`; result: passed.
