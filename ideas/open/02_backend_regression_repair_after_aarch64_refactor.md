# Backend Regression Repair After AArch64 Refactor

Status: Open
Last Updated: 2026-04-02

## Goal

Stabilize backend-only regressions introduced after recent aarch64 backend refactor before continuing structural changes.

## Background

`ctest -R backend` revealed a large failing cluster after refactor, including:

- `backend_lir_adapter_aarch64_tests`
- `backend_runtime_nested_member_pointer_array`
- `backend_runtime_nested_param_member_array`
- `backend_runtime_param_member_array`
- many `c_testsuite_x86_backend_src_*_c` cases in backend scope

This indicates a shared backend-path breakage in the backend IR seam, not isolated behavioral drift.

## Scope

This idea only targets backend regression recovery. No new API redesign, naming changes, or broad backend path refactors.

- Keep regression scope backend-only:
  - `tests/c/internal/backend_*`
  - `tests/backend/*`
  - `ctest --test-dir build -R backend`
- Stop until current regressions are recovered and regression checks are stable.

## Workstreams

### 1) Baseline triage and ownership

- Confirm whether failures share one or two common root causes across emit/IR/lowering seam.
- Capture one failing output per cluster for root-cause analysis.
- Record reproducible shortlist:
  - aarch64 seam case
  - at least 2 representative x86 `c_testsuite` failures

### 2) Narrow repair slices

- Repair by narrow seam, one cluster at a time:
  - aarch64 backend adapter path
  - x86 emitted-call/path behavior impacted by shared metadata changes
- For each slice, add/adjust minimal targeted tests in backend suite.
- Preserve behavior for existing legacy behavior where unchanged paths are still expected.

### 3) Validate before expanding

- After each slice, run:
  - `ctest --test-dir build -R backend_lir_adapter_aarch64_tests --output-on-failure`
  - `ctest --test-dir build -R "backend.*x86_backend_src_00025_c|backend.*x86_backend_src_00156_c|backend_runtime" --output-on-failure`
  - `ctest --test-dir build -R backend --output-on-failure`
- Then update the running regression checklist.

### 4) Closeout and handoff

- Confirm blast radius reduced to pre-existing known baseline only.
- Document all fixes, assumptions, and follow-up risks.
- Reactivate next plan only after backend regression scope is stable.

## Exit Criteria

- [ ] `backend_lir_adapter_aarch64_tests` 回歸通過
- [ ] `backend_runtime_*member*` 回歸通過
- [ ] `c_testsuite_x86_backend_src_*_c` 回歸通過或已穩定封存為既有 baseline
- [ ] `ctest -R backend` 在本輪維持可接受穩定性後再啟動下一個重構主題
