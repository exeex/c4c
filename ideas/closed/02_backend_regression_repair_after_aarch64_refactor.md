# Backend Regression Repair After AArch64 Refactor

Status: Complete
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

## Progress Notes

- 2026-04-02: Fixed the residual x86 backend runtime mismatch cluster affecting `00131`, `00165`, `00177`, `00188`, `00206`, and `00211`.
  Root cause: `src/backend/lir_adapter.cpp` rewrote any `main` ending in a direct variadic call into a backend function containing only the final call, which truncated earlier `printf` calls and dropped their arguments.
  Resolution: preserve all direct variadic calls discovered in the block and keep pointer-resolution for string literal GEPs, so unsupported multi-call surfaces fall back through LLVM IR lowering instead of emitting a one-call asm stub.
- 2026-04-02: Added an x86 backend adapter regression covering multiple `printf` calls in one function so the backend IR seam keeps both calls instead of collapsing to the last one.
- 2026-04-02: `ctest --test-dir build -R backend --output-on-failure` now fails only in:
  - `backend_contract_aarch64_return_add_object`
  - `backend_contract_aarch64_global_load_object`
  These remain as the only unresolved backend cases in this plan after the x86 cluster recovery.

## Completion Notes

- 2026-04-02: Closed after the backend regression-recovery target was reduced from the original mixed aarch64/x86/runtime cluster to two isolated AArch64 object-contract cases.
- Verified recovered slices still pass on 2026-04-02:
  - `backend_lir_adapter_aarch64_tests`
  - `backend_runtime_nested_member_pointer_array`
  - `backend_runtime_nested_param_member_array`
  - `backend_runtime_param_member_array`
  - `c_testsuite_x86_backend_src_00025_c`
  - `c_testsuite_x86_backend_src_00156_c`
  - `c_testsuite_x86_backend_src_00131_c`
  - `c_testsuite_x86_backend_src_00165_c`
  - `c_testsuite_x86_backend_src_00177_c`
  - `c_testsuite_x86_backend_src_00188_c`
  - `c_testsuite_x86_backend_src_00206_c`
  - `c_testsuite_x86_backend_src_00211_c`
- Full sweep on 2026-04-02: `ctest --test-dir build -R backend --output-on-failure` reported 279/281 passing tests, with only:
  - `backend_contract_aarch64_return_add_object`
  - `backend_contract_aarch64_global_load_object`
- Follow-on work for those two object-contract failures was split into `ideas/open/07_aarch64_object_contract_repair.md` rather than extending this completed recovery plan.

## Exit Criteria

- [x] `backend_lir_adapter_aarch64_tests` regression passes
- [x] `backend_runtime_*member*` regressions pass
- [x] `c_testsuite_x86_backend_src_*_c` regression cluster repaired
- [x] `ctest -R backend` restored to a stable 279/281 boundary, with the remaining two AArch64 object-contract failures explicitly deferred into a separate follow-on idea
