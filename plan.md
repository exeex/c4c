Status: Active
Source Idea: ideas/open/02_backend_regression_repair_after_aarch64_refactor.md

# Backend Regression Recovery Runbook

## Purpose
Recover backend regressions after the recent aarch64 backend refactor before continuing structural backend work.

## Goal
Stabilize backend test failures in `ctest --test-dir build -R backend` with a strictly backend-only scope.

## Core Rule
Repair one narrow slice at a time, prove it with tests, then proceed to the next.

## Non-Goals
- No API redesign.
- No naming cleanups.
- No broad backend architecture refactors.

## Read First
- [`src/codegen/llvm`](src/codegen/llvm)
- `tests/c/internal/backend_*`
- `tests/backend/*`

## Scope
- Backend IR seam and backend path behavior impacted by the refactor.
- Focus targets:
  - `backend_lir_adapter_aarch64_tests`
  - `backend_runtime_nested_member_pointer_array`
  - `backend_runtime_nested_param_member_array`
  - `backend_runtime_param_member_array`
  - `c_testsuite_x86_backend_src_*_c`
  - `ctest --test-dir build -R backend`

## Execution Rules
- Keep edits inside backend regression boundaries.
- Use minimal targeted fixes and tests.
- Record each slice result before moving on.
- Do not move to a new initiative until this scope is stable.

## Step 1: Baseline triage and ownership
- Confirm whether failures cluster by shared root cause across emit/IR/lowering seam.
- Capture one failing output per cluster:
  - aarch64 adapter cluster
  - at least two `c_testsuite_x86_backend_src_*_c` regressions
  - runtime member-array regressions
- Assign each cluster to a repair slice.

Completion check:
- [ ] Representative failure logs captured and triage notes written.
- [ ] Slices mapped to root-cause clusters.

## Step 2: Repair aarch64 adapter seam
- Fix regressions in the aarch64 backend adapter path with minimal changes.
- Add or adjust targeted backend tests for this slice before code changes.

Completion check:
- [ ] Targeted tests for aarch64 adapter added/updated.
- [ ] `ctest --test-dir build -R backend_lir_adapter_aarch64_tests --output-on-failure` passes.

## Step 3: Repair x86/runtime metadata-related regressions
- Fix regressions in x86 emitted-call/path behavior tied to shared metadata/seam changes.
- Address one root cause at a time; keep tests minimal and focused.

Completion check:
- [ ] Targeted x86/runtime tests pass:
  - `ctest --test-dir build -R "backend.*x86_backend_src_00025_c|backend.*x86_backend_src_00156_c|backend_runtime" --output-on-failure`

## Step 4: Cross-slice validation
- Run full backend regression set after each slice and again after all slices.
- Explicitly classify leftover failures as fixed, known baseline, or deferred with reason.

Completion check:
- [ ] `ctest --test-dir build -R backend --output-on-failure` is stable for this plan.
- [ ] Remaining failures are documented and intentionally deferred if applicable.

## Step 5: Closeout and handoff
- Document fixes, assumptions, and residual risks in the source idea.
- Ensure only backend regression recovery remains in scope.
- Mark ready to resume next refactor only when stable.

Completion check:
- [ ] `backend_lir_adapter_aarch64_tests` stable.
- [ ] `backend_runtime_*member*` stable.
- [ ] `c_testsuite_x86_backend_src_*_c` stable or explicitly managed as baseline.
- [ ] `ctest -R backend` regains stable pass/fail boundary.
