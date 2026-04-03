Status: Active
Source Idea: ideas/open/07_aarch64_object_contract_repair.md
Source Plan: plan.md

# Active Queue: AArch64 Object Contract Repair

## Queue
- [ ] Step 1: Reproduce and classify the empty-output failure
  - Notes: start from the exact `run_backend_contract_case.cmake` invocation and
    determine whether the blank `.s` output comes from frontend failure, backend
    emission failure, or toolchain handoff.
  - Blockers: None.
- [ ] Step 2: Repair return/global-load object emission
  - Notes: keep the fix inside the AArch64 contract/object-emission path and add
    the narrowest regression coverage that proves the repaired output is
    non-empty and structurally correct.
  - Blockers: None.
- [ ] Step 3: Validate backend and full-suite stability
  - Notes: targeted contract tests first, then backend sweep, then full-suite
    monotonic regression check.
  - Blockers: None.

## Current Focus

Step 1: reproduce the two failing contract cases and classify why the generated
`.s` files are empty before objdump validation

## Immediate Next Slice

Read `tests/c/internal/cmake/run_backend_contract_case.cmake`, capture the
exact command used for `backend_contract_aarch64_return_add_object` and
`backend_contract_aarch64_global_load_object`, then inspect whether the blank
output is introduced before or after AArch64 backend emission.

## Accomplished Context

- `05_bir_enablement_and_test_harness_refactor.md` was completed and moved to
  `ideas/closed/` on 2026-04-03.
- The full-suite baseline recorded immediately before activating this plan is
  `2668 passed / 2 failed / 2670 total`.
- The only known remaining backend failures in that baseline are:
  `backend_contract_aarch64_return_add_object` and
  `backend_contract_aarch64_global_load_object`.

## Risks And Constraints

- Do not reopen the recovered broader backend regression cluster.
- Do not widen this work into BIR cutover or naming cleanup.
- Keep x86/runtime behavior unchanged unless directly required for correctness.

## Validation Baseline

- Contract failures currently present before implementation:
  `backend_contract_aarch64_return_add_object`,
  `backend_contract_aarch64_global_load_object`.
- Full-suite baseline:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
  passed for the just-closed BIR plan and established the current
  `2668 passed / 2 failed / 2670 total` ceiling.
