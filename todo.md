# Current Packet

Status: Active
Source Idea Path: ideas/open/334_object_route_scan_and_default_readiness.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Resume 39-Test Baseline And Candidate Map

## Just Finished

- Completed Step 1 resumed scan inspection after child 338.
- Current selectable object surface:
  - `backend_cli_.*obj` selects byte-output contract tests for RV64/AArch64
    object CLI paths, unsupported-target diagnostics, and missing-output
    diagnostics.
  - `backend_obj_runtime_.*` selects RV64 native object runtime cases that
    compile `--codegen obj`, link the produced `.o`, and run through
    `qemu-riscv64`.
  - `backend_rv64_runtime_.*` remains the asm-route RV64 runtime counterpart
    selector.
  - Representative AArch64 c-testsuite object labels:
    `aarch64, backend, backend_route, c_testsuite, cli, dual-route, internal,
    object, output_contract, scan, smoke`.
  - Representative RV64 c-testsuite object-runtime labels:
    `backend, backend_runtime, c_testsuite, case, internal, object, qemu,
    rv64, scan, smoke`.
  - Existing AArch64 asm external return smokes remain selectable as
    `backend_cli_aarch64_asm_external_return_(zero|add|add_sub_chain)_smoke`.
- Confirmed current green c-testsuite object smokes:
  - RV64 object runtime: `backend_obj_runtime_rv64_cts_00001`,
    `backend_obj_runtime_rv64_cts_00002`.
  - AArch64 object-byte output:
    `backend_cli_aarch64_cts_00001_writes_elf_obj`,
    `backend_cli_aarch64_cts_00002_writes_elf_obj`,
    `backend_cli_aarch64_cts_00011_writes_elf_obj`,
    `backend_cli_aarch64_cts_00012_writes_elf_obj`.
- Dry-run findings under `/tmp`:
  - RV64 object runtime passed for c-testsuite `00003.c`, `00011.c`,
    `00012.c`, and `00015.c`.
  - AArch64 object-byte output passed for c-testsuite `00011.c` and `00012.c`
    and remains registered from child 338.
  - AArch64 object-byte output still rejects c-testsuite `00003.c`, backend
    `param_slot.c`, and `two_arg_first_local_rewrite.c` at the fixed-frame
    saved-callee/local-call-frame frontier.
  - AArch64 branch/control-flow probes such as c-testsuite `00010.c` and
    backend `branch_if_eq.c` still reject as unsupported machine instructions.
  - RV64 object output still rejects pointer/control-flow style probes such as
    c-testsuite `00004.c`, `00009.c`, `00010.c`, `00013.c`, and backend
    branch/global-adjacent cases at the prepared object-shape gate.

## Suggested Next

- Step 2 can be a parent-owned scan registration packet, not a split yet:
  add RV64 object-runtime c-testsuite smokes for `00011.c` and `00012.c`.
  These are conservative because both passed the native RV64 object runtime
  helper under `/tmp`, and their AArch64 object-byte counterparts are already
  green in the 39-test baseline.
- Suggested Step 2 owned files: `tests/backend/CMakeLists.txt` and `todo.md`.
- Suggested Step 2 registrations:
  `backend_obj_runtime_rv64_cts_00011` and
  `backend_obj_runtime_rv64_cts_00012`, expected run code `0`, labels
  `c_testsuite smoke scan` through the existing RV64 object runtime helper.
- Exact Step 2 proof command:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_riscv_object_emission|backend_aarch64_object_emission|backend_cli_.*obj|backend_obj_runtime_.*|backend_rv64_runtime_(return_zero|return_add|two_arg_helper|two_arg_local_arg|two_arg_both_local_arg|two_arg_second_local_arg|local_arg_call|return_add_sub_chain|local_temp)|backend_cli_aarch64_asm_external_return_(zero|add|add_sub_chain)_smoke|backend_codegen_route_riscv64_external_no_storage_main_emits_return_path)$' --output-on-failure) > test_after.log 2>&1`

## Watchouts

- Preserve existing asm-route and object-route selectors.
- Do not add expected-failure labels or weaken object/asm expectations.
- Do not add RV64-only `00003.c` in the first resumed parent packet even
  though it dry-ran green; AArch64 still needs saved-callee/local-call-frame
  object support for the same frontier.
- If the next desired balanced scan target is c-testsuite `00003.c`,
  `param_slot.c`, or any `two_arg_*_rewrite.c`, split a focused AArch64
  saved-callee/local-call-frame object-emitter child before registering those
  cases.
- Keep branch/control-flow, branch/global families, AArch64 runtime,
  globals/data, pointers, aggregates, byval, indirect calls, x86 object output,
  object stdout, c-testsuite defaults, and semantic-BIR object mode out of the
  next parent-owned packet.

## Proof

- Inspection-only packet; no build required.
- Used `/tmp/c4c_334_*` outputs for targeted object emit and RV64
  link/qemu dry-runs.
- Ran `git diff --check`; result: passed.
