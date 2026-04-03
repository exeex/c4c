Status: Active
Source Idea: ideas/open/07_aarch64_object_contract_repair.md
Activated from: ideas/open/07_aarch64_object_contract_repair.md
Skipped Lower-Priority Idea: ideas/open/06_bir_cutover_and_legacy_cleanup.md
Skip Reason: its own preconditions say not to activate until BIR coverage is broad enough for production cutover

# AArch64 Object Contract Repair Runbook

## Purpose

Repair the two remaining AArch64 backend object-contract failures without
reopening the broader backend regression cluster that was already recovered.

## Goal

Make the contract/object-emission path produce valid non-empty AArch64 asm for
the return-add and global-load contract cases.

## Core Rule

Stay inside the AArch64 contract/object-emission path and preserve the current
recovered x86/runtime backend boundary.

## Read First

- `ideas/open/07_aarch64_object_contract_repair.md`
- `tests/c/internal/cmake/run_backend_contract_case.cmake`
- `tests/backend/backend_lir_adapter_aarch64_tests.cpp`
- `src/backend/backend.cpp`
- `src/backend/aarch64/codegen/emit.cpp`
- `src/backend/aarch64/assembler/mod.cpp`
- `src/backend/aarch64/assembler/elf_writer.cpp`

## Scope

- Reproduce why the two contract cases currently write empty `.s` output
- Repair AArch64 object emission for the `return_add` and `global_load` cases
- Add or adjust the narrowest test coverage that locks the repaired behavior

## Non-Goals

- No unrelated backend cleanup
- No BIR-default cutover work
- No broad x86/runtime backend changes unless directly required to keep the
  repaired AArch64 path correct

## Execution Rules

- Reproduce the exact contract invocation before changing code.
- Distinguish frontend failure, backend emission failure, and toolchain handoff
  failure before implementing a fix.
- Prefer the smallest fix that restores non-empty AArch64 asm/object output.
- Add or update targeted tests before widening to the full backend and full
  suite runs.
- If a separate AArch64 binary-utils initiative appears, record it in
  `ideas/open/` instead of silently expanding this plan.

## Step 1: Reproduce and classify the empty-output failure

- Capture the exact command path used by
  `tests/c/internal/cmake/run_backend_contract_case.cmake`.
- Reproduce both failing cases locally and inspect the emitted `.s` files,
  stderr, and exit status.
- Decide whether the blank output originates in frontend compilation, backend
  emission, or the external toolchain handoff.

Completion check:
- [ ] The failing command line is recorded in working notes or `plan_todo.md`.
- [ ] The failure point is classified narrowly enough to choose one code path.
- [ ] The investigation stays focused on the two contract cases only.

## Step 2: Repair return/global-load object emission

- Implement the smallest AArch64-side fix that makes
  `backend_contract_aarch64_return_add_object` produce valid asm/object output.
- Extend the same fix, or a closely-related follow-up, to
  `backend_contract_aarch64_global_load_object`.
- Add or update the narrowest targeted tests needed to keep the repaired object
  emission behavior covered.

Completion check:
- [ ] `backend_contract_aarch64_return_add_object` passes.
- [ ] `backend_contract_aarch64_global_load_object` passes.
- [ ] New tests cover the repaired emission path without widening scope.

## Step 3: Validate backend and full-suite stability

- Run:
  - `ctest --test-dir build -R 'backend_contract_aarch64_(return_add_object|global_load_object)' --output-on-failure`
  - `ctest --test-dir build -R backend --output-on-failure`
- Re-run the full suite and compare `test_before.log` vs `test_after.log` with
  the regression guard.
- Record any bounded remaining AArch64 contract gaps if they are out of scope
  for this repair.

Completion check:
- [ ] The two targeted contract cases pass.
- [ ] `ctest --test-dir build -R backend --output-on-failure` no longer fails
      in these two contract cases.
- [ ] Full-suite regression guard reports zero new failing tests.
