# Plan Todo: AArch64 Param Member Array

Status: Active
Source Idea: ideas/open/15_backend_aarch64_param_member_array_plan.md
Source Plan: plan.md

## Active Item

- [ ] Step 4: Validate the promoted `param_member_array` slice against the full suite and resolve the unrelated clean-build blocker

## Todo

- [x] Capture the current runtime-emitted LIR for `tests/c/internal/backend_case/param_member_array.c`
- [x] Compare that output against `make_param_member_array_gep_module()`
- [x] Tighten synthetic backend expectations around the exact runtime-shaped two-function slice
- [x] Implement bounded AArch64 emission for the exact slice in `src/backend/aarch64/codegen/emit.cpp`
- [x] Promote `param_member_array` to `BACKEND_OUTPUT_KIND=asm` in `tests/c/internal/InternalTests.cmake`
- [x] Run targeted backend tests and runtime tests
- [ ] Run full before/after regression validation and record the result

## Completed

- [x] Created `ideas/open/15_backend_aarch64_param_member_array_plan.md` as the next bounded child idea
- [x] Activated the child idea into `plan.md`
- [x] Added `backend_contract_aarch64_param_member_array_object` to lock the emitted object-form asm contract
- [x] Verified `backend_lir_adapter_tests`, `backend_contract_aarch64_param_member_array_object`, `backend_runtime_param_member_array`, and `backend_runtime_nested_param_member_array`

## Next Intended Slice

- Investigate the clean-build failure in `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`, decide whether it is harness flake or real codegen drift, and rerun full before/after regression validation once the blocker is understood.

## Blockers

- Full-suite regression guard is currently blocked by a new failing clean-build test outside this slice: `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`.
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed` reported `before: passed=715 failed=25 total=740` and `after: passed=715 failed=26 total=741` because that C++ runtime test now fails with `exit=no such file or directory`.
