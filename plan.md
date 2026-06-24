# Target Object Emitter Local Call And RegReg Scalar Expansion Runbook

Status: Active
Source Idea: ideas/open/337_target_object_emitter_local_call_and_regreg_scalar_expansion.md
Split from: ideas/open/334_object_route_scan_and_default_readiness.md

## Purpose

Repair the focused target-owned object-emitter gaps that currently prevent
idea 334 from adding the next object-route scan cases without expected-failure
labels or weakened expectations.

## Goal

Support native object emission for RV64 scalar local-backed direct call
arguments and AArch64 selected register-register scalar/call-result object
output, then restore the corresponding object-route scan tests.

## Core Rule

Do not make scan progress through testcase-name matching, asm fallback,
external assemblers, expected-failure labels, or weaker object/asm contracts.
Support the target-owned prepared or selected instruction shapes semantically.

## Read First

- `ideas/open/337_target_object_emitter_local_call_and_regreg_scalar_expansion.md`
- `ideas/open/334_object_route_scan_and_default_readiness.md`
- `todo.md`
- `tests/backend/CMakeLists.txt`
- `tests/backend/cmake/run_backend_rv64_object_runtime_case.cmake`
- `tests/backend/cmake/run_backend_codegen_object_case.cmake`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/aarch64/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `tests/backend/mir/backend_aarch64_object_emission_test.cpp`

## Current Baseline

The parent scan baseline after child 336 is green:

```bash
set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_riscv_object_emission|backend_aarch64_object_emission|backend_cli_.*obj|backend_obj_runtime_.*|backend_rv64_runtime_(return_zero|return_add|two_arg_helper|two_arg_local_arg|two_arg_both_local_arg|two_arg_second_local_arg|local_arg_call|return_add_sub_chain|local_temp)|backend_cli_aarch64_asm_external_return_(zero|add|add_sub_chain)_smoke|backend_codegen_route_riscv64_external_no_storage_main_emits_return_path)$' --output-on-failure) > test_after.log 2>&1
```

Result recorded by idea 334 Step 1: 28/28 restored scalar proof remains green,
but the next candidate dry runs are known-red at target object emission:

- RV64: `two_arg_local_arg.c`, `two_arg_second_local_arg.c`,
  `two_arg_both_local_arg.c`, and `local_arg_call.c` reject with
  `RISC-V backend object route unsupported prepared module shape`.
- AArch64: `two_arg_helper.c` rejects with selected immediate add/sub-only
  object support; local/frame-memory AArch64 candidates remain a later
  boundary.

## Non-Goals

- Do not change CLI object policy, object stdout, x86 object output,
  c-testsuite object defaults, or object semantic-BIR mode.
- Do not add globals/data, arrays, pointer/aggregate support, broad RV64 memory
  expansion, broad AArch64 frame/local-memory expansion, or AArch64 runtime.
- Do not weaken existing asm-route or object-route tests.
- Do not add expected-failure scan labels for the known-red candidates.

## Execution Rules

- Keep RV64 and AArch64 work in separate narrow packets when possible.
- Add or restore scan tests only after the target object emitter can produce
  native object bytes for that case.
- Preserve existing object and asm selectors in each proof subset.
- If a candidate reveals a broader target-emitter family than this idea covers,
  stop and record the precise blocker rather than expanding scope.

## Step 1: Inspect Local-Call And RegReg Object Rejections

Goal: record exact target-owned seams and first implementation proof plan.

Actions:

- Inspect the RV64 prepared object records for `two_arg_local_arg.c`,
  `two_arg_second_local_arg.c`, `two_arg_both_local_arg.c`, and
  `local_arg_call.c`.
- Inspect the AArch64 selected machine records for `two_arg_helper.c`.
- Name the exact object-emission predicates or dispatch paths that reject the
  candidates.
- Record Step 2 owned files and proof command in `todo.md`.

Completion check:

- `todo.md` names the RV64 local-call seam, the AArch64 reg-reg scalar seam,
  the first minimal implementation slice, unsupported boundaries, and proof.

## Step 2: Add RV64 Local-Backed Scalar Call Object Support

Goal: support the RV64 prepared shapes for local-backed scalar direct call
arguments and restore the RV64 object-runtime scan cases.

Actions:

- Extend RV64 target object emission for scalar local homes used as direct call
  arguments in the named no-global cases.
- Add focused RV64 object-emitter tests for the new prepared-record support.
- Add object-runtime CTest registrations beside existing asm-route tests for
  `two_arg_local_arg.c`, `two_arg_second_local_arg.c`,
  `two_arg_both_local_arg.c`, and `local_arg_call.c`.

Completion check:

- The restored RV64 object-runtime cases pass through native object bytes,
  link with the existing RV64 toolchain, run under qemu, and preserve the
  previous RV64 object/asm scalar cases.

## Step 3: Add AArch64 RegReg Scalar Object Support

Goal: support the AArch64 selected instruction family needed by
`two_arg_helper.c` and restore its object-byte scan case.

Actions:

- Extend AArch64 target object emission for the selected register-register
  scalar arithmetic or call-result movement needed by `two_arg_helper.c`.
- Add focused AArch64 object-emitter tests for the new encoding support.
- Add the AArch64 object-byte CTest registration beside existing asm-route
  coverage.

Completion check:

- `backend_cli_aarch64_two_arg_helper_writes_elf_obj` passes as native ELF
  object-byte output, and existing AArch64 object/asm scalar cases remain
  green.

## Step 4: Validate Expanded Scan Baseline

Goal: prove the expanded object-route scan baseline before returning to idea
334.

Actions:

- Run the focused proof including prior object/asm scalar cases and the newly
  restored RV64/AArch64 scan candidates.
- Record supported selectors, remaining unsupported boundaries, command, and
  result count in `todo.md`.

Completion check:

- `todo.md` records a green expanded baseline and explicitly hands back to
  `ideas/open/334_object_route_scan_and_default_readiness.md`.

## Step 5: Hand Back To Object Route Scan

Goal: complete the child lifecycle when target-owned gaps are repaired.

Actions:

- Summarize target-emitter capabilities added.
- List restored object-route selectors.
- Keep broader unsupported/default-policy boundaries assigned to idea 334.
- Record the final proof command and result count in `todo.md`.

Completion check:

- The child is ready for plan-owner closure, and idea 334 can resume from the
  expanded green scan baseline.
