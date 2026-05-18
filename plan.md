# AArch64 C-Testsuite Backend Runtime Execution Plan

Status: Active
Source Idea: ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md

## Purpose

Turn the existing AArch64 backend c-testsuite route into a real runtime
execution workflow on an AArch64 host or with an explicit runner.

## Goal

Prove representative c-testsuite cases compile through the AArch64 backend
assembly path, assemble/link with clang, run, and match their `.expected`
sidecars, then produce a broader failure inventory grouped by owner stage.

## Core Rule

Passing evidence must exercise `c4cll --codegen asm --target
aarch64-unknown-linux-gnu` and the `.s -> clang -> executable -> runtime ->
expected-output` path. Do not count LLVM IR fallback, runtime-unavailable, or
expectation weakening as progress.

## Read First

- `ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md`
- `tests/c/external/c-testsuite/RunCase.cmake`
- `tests/c/external/c-testsuite-aarch64-backend-runner.cmake`
- `tests/c/external/CMakeLists.txt`
- Existing AArch64 backend c-testsuite build tree and logs, if present.

## Current Scope

- AArch64 backend c-testsuite CMake/CTest route configuration.
- Native AArch64 host execution or `C_TESTSUITE_AARCH64_BACKEND_RUNNER`.
- Focused runtime proof for representative simple c-testsuite cases.
- Broader `aarch64_backend` route inventory after focused proof.

## Non-Goals

- Do not make the full c-testsuite corpus pass in this runbook.
- Do not weaken `.expected` files, allowlists, unsupported classifications, or
  CTest expectations.
- Do not count `[RUNTIME_UNAVAILABLE]` as a pass.
- Do not add filename-specific lowering, printer hacks, or c-testsuite-shaped
  backend shortcuts.
- Do not rework the x86 backend route or replace the external clang/as runtime
  route with an in-process assembler.
- Do not fold non-runtime backend/codegen/BIR repairs into this route-readiness
  plan; split focused follow-up ideas instead.

## Working Model

Idea 276 was parked because the previous host could reach the runtime boundary
but had no AArch64 host or configured runner. This environment reports an
AArch64 host, so the route can be reactivated to test the runtime side without
requiring `C_TESTSUITE_AARCH64_BACKEND_RUNNER`.

The route already selected 220 `aarch64_backend` tests before parking, with
failures grouped as 121 `[RUNTIME_UNAVAILABLE]`, 85 `[FRONTEND_FAIL]`, and 14
`[BACKEND_FAIL]`. This plan should first verify the runner/native-host contract
and focused smoke cases, then rerun a broader scan to learn which failures are
real after runtime is available.

## Execution Rules

- Keep each packet bounded to one audit, configuration/proof, or inventory
  step.
- Update `todo.md` with the current packet result and exact proof command.
- Preserve `test_after.log` as the canonical executor proof log for routine
  packets.
- Use native AArch64 execution when `uname -m` reports `aarch64`; otherwise
  require `C_TESTSUITE_AARCH64_BACKEND_RUNNER`.
- Keep diagnostics distinct for frontend/BIR failure, backend assembly failure,
  assembler/link failure, runtime unavailable, runtime nonzero, and runtime
  mismatch.
- If focused runtime proof exposes a separate compiler/backend failure family,
  record a follow-up idea instead of expanding this route plan.

## Steps

### Step 1: Audit Runtime Route And Host Contract

Goal: confirm the current CMake runner route will run AArch64 backend binaries
natively in this environment or through the configured runner.

Primary targets:

- `tests/c/external/c-testsuite/RunCase.cmake`
- `tests/c/external/c-testsuite-aarch64-backend-runner.cmake`
- `tests/c/external/CMakeLists.txt`

Actions:

- Inspect the current backend route and the frontend LLVM IR c-testsuite route
  for invocation differences.
- Confirm `CODEGEN_MODE=backend-aarch64` invokes `c4cll --codegen asm --target
  aarch64-unknown-linux-gnu`.
- Confirm the runner path uses direct execution on AArch64 hosts and
  `C_TESTSUITE_AARCH64_BACKEND_RUNNER` only for non-AArch64 hosts.
- Confirm missing runtime remains `[RUNTIME_UNAVAILABLE]` and is not converted
  into a pass.
- Record the exact focused CTest command the supervisor should delegate next.

Completion check:

- `todo.md` names the native/runner decision, the relevant CMake variables, and
  the focused proof command for representative smoke cases.

### Step 2: Prove Focused Backend Runtime Smoke Cases

Goal: prove representative simple c-testsuite cases go through backend
assembly, clang assemble/link, runtime execution, and expected-output compare.

Primary targets:

- `tests/c/external/c-testsuite-aarch64-backend-runner.cmake`
- generated files under the AArch64 backend c-testsuite build tree

Actions:

- Configure or reuse the AArch64 backend c-testsuite scan build with
  `ENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON`.
- Run a focused subset for representative simple cases such as `src/00001.c`,
  `src/00002.c`, and `src/00003.c`.
- Inspect proof output or generated artifacts to confirm `.s` output exists,
  clang consumed assembler input with `--target=aarch64-unknown-linux-gnu -x
  assembler`, an executable was produced, and runtime output matched the
  `.expected` sidecar.
- Confirm no LLVM IR fallback was used.

Completion check:

- The focused smoke cases pass through runtime and expected-output comparison,
  or `todo.md` records the exact first failing stage and owner layer.

### Step 3: Tighten Route Diagnostics If Needed

Goal: make route failures acceptance-quality without weakening expectations.

Primary targets:

- `tests/c/external/c-testsuite/RunCase.cmake`
- `tests/c/external/c-testsuite-aarch64-backend-runner.cmake`

Actions:

- If Step 2 shows vague or collapsed failure output, separate diagnostics for
  frontend/BIR, backend assembly, assembler/link, runtime unavailable, runtime
  nonzero, and runtime mismatch.
- Ensure missing `.s` output and LLVM IR fallback are rejected explicitly.
- Keep the route runnable from CMake/CTest with the native-host or runner
  contract from Step 1.

Completion check:

- Focused smoke proof has distinct stage diagnostics and still uses the
  backend assembly runtime path.

### Step 4: Run Broader AArch64 Backend C-Testsuite Route

Goal: produce actionable broader evidence after runtime execution is available.

Actions:

- Run the configured broader route, normally:
  `ctest --test-dir build-aarch64-scan --output-on-failure -L aarch64_backend`
- Group failures by stage: frontend/BIR, backend assembly, assembler/link,
  runtime unavailable, runtime nonzero, runtime mismatch, and pass.
- Compare the new counts against the parked inventory where useful.

Completion check:

- `todo.md` records broad scan counts by stage and identifies any non-runtime
  failure families that should become focused follow-up ideas.

### Step 5: Completion Review

Goal: decide whether idea 276 is complete under its source acceptance criteria.

Actions:

- Compare focused runtime proof and broad inventory against
  `ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md`.
- Confirm representative cases compile to AArch64 `.s`, assemble/link, run, and
  match expected output.
- Confirm runtime-unavailable remains a blocker only when no host or runner is
  available.
- If complete, request plan-owner closure review.
- If a distinct backend/codegen/BIR repair is needed, create or request a
  focused follow-up idea instead of broadening this runbook.

Completion check:

- `todo.md` clearly says whether the source idea is ready for closure review or
  names the exact blocker/follow-up owner layer.
