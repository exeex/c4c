# Backend Post-Refactor Regression Cluster Repair

## Status

Open as of 2026-04-03.

## Goal

Repair the backend regressions reported after the recent backend refactor,
with priority on restoring the AArch64 backend-owned c-testsuite subset and
recovering the two non-corpus regressions that remain visible in repository
logs.

## Background

The newly reported failing set is:

- `positive_sema_linux_stage2_repro_03_asm_volatile_c`
- `c_testsuite_aarch64_backend_src_00040_c`
- `c_testsuite_aarch64_backend_src_00047_c`
- `c_testsuite_aarch64_backend_src_00048_c`
- `c_testsuite_aarch64_backend_src_00049_c`
- `c_testsuite_aarch64_backend_src_00050_c`
- `c_testsuite_aarch64_backend_src_00090_c`
- `c_testsuite_aarch64_backend_src_00091_c`
- `c_testsuite_aarch64_backend_src_00092_c`
- `c_testsuite_aarch64_backend_src_00146_c`
- `c_testsuite_aarch64_backend_src_00147_c`
- `c_testsuite_aarch64_backend_src_00148_c`
- `c_testsuite_aarch64_backend_src_00149_c`
- `c_testsuite_aarch64_backend_src_00150_c`
- `llvm_gcc_c_torture_src_20080502_1_c`

This cluster strongly suggests backend-path drift introduced by refactoring
rather than unrelated frontend breakage:

- twelve failures are concentrated in the AArch64 backend-owned c-testsuite
  surface
- one failure is an internal semantic/asm-volatile repro
- one failure is an external GCC torture case

There is also an important reproducibility note:

- the checked-in `test_fail_before.log` and `test_fail_after.log` both still
  show the twelve listed AArch64 c-testsuite cases as passing
- those same logs do show
  `positive_sema_linux_stage2_repro_03_asm_volatile_c` and
  `llvm_gcc_c_torture_src_20080502_1_c` as failing

So the first execution step for this idea must refresh the failing baseline from
the current build tree before making behavioral assumptions about the AArch64
cluster.

## Why This Slice Exists

This work should stay separate from broader backend architecture changes.
The immediate need is to restore a recently working backend surface after
refactor, capture the true current blast radius, and only then decide whether
the root cause lives in:

- shared backend IR/lowering metadata
- AArch64-specific emission/addressing logic
- inline asm / volatile preservation
- a codegen contract now exercised by the GCC torture case

## Primary Scope

- `src/backend/**`
- `src/lir/**`
- `src/codegen/**`
- `tests/c/external/c-testsuite/**`
- `tests/c/external/gcc_torture/**`
- `tests/c/internal/positive/**`
- targeted backend/sema regression tests adjacent to the repaired path

## Non-Goals

- no new backend feature work beyond what is required to recover the regressions
- no broad plan expansion into unrelated backend cleanup
- no weakening of backend-owned test expectations to make the failures disappear
- no silent absorption of newly discovered unrelated regressions; those should
  be split into separate follow-on ideas if they are not required to close this
  slice

## Workstreams

### 1. Re-establish the real failing baseline

- rerun the exact reported failing tests against the current `build/` tree
- compare the fresh output with `test_fail_before.log` and `test_fail_after.log`
- record whether the twelve AArch64 c-testsuite cases still fail now, or whether
  the current unresolved baseline has already narrowed to the two log-visible
  failures
- if test numbering changed, preserve the test names rather than relying on old
  numeric ids

### 2. Bucket by shared backend mechanism

- inspect the AArch64 c-testsuite cases as three small families:
  - `00040`, `00047` to `00050`
  - `00090` to `00092`
  - `00146` to `00150`
- determine whether they share one backend lowering or emitter contract
- inspect `positive_sema_linux_stage2_repro_03_asm_volatile_c` for loss of
  `asm volatile` semantics during lowering or emission
- inspect `llvm_gcc_c_torture_src_20080502_1_c` for whether it matches the same
  root cause or represents a second independent contract regression

### 3. Repair the smallest common seam first

- prefer a narrow fix that restores an entire family rather than chasing one
  testcase at a time
- add or tighten the smallest durable regression coverage around the actual seam
- preserve existing backend-owned behavior on both AArch64 and non-AArch64
  targets

### 4. Validate monotonically

- rerun each repaired representative case with `--output-on-failure`
- rerun the full affected AArch64 backend subset once the common seam is fixed
- rerun the asm-volatile positive case and the GCC torture case after any shared
  lowering or emission change
- finish with a broader backend sweep to ensure the refactor repair did not
  create new backend regressions

## Suggested Validation

- `ctest --test-dir build -R '^(positive_sema_linux_stage2_repro_03_asm_volatile_c|llvm_gcc_c_torture_src_20080502_1_c)$' --output-on-failure`
- `ctest --test-dir build -R '^c_testsuite_aarch64_backend_src_(00040|00047|00048|00049|00050|00090|00091|00092|00146|00147|00148|00149|00150)_c$' --output-on-failure`
- `ctest --test-dir build -L aarch64_backend --output-on-failure`
- `ctest --test-dir build -R backend --output-on-failure`

## Success Condition

This idea is complete when:

- the user-reported post-refactor failures are either fixed or reduced to a
  smaller, explicitly documented follow-on idea
- the repaired backend path has targeted regression coverage for the recovered
  seam
- the final validation shows no newly introduced backend regressions relative to
  the refreshed baseline captured at the start of execution

## Notes For Activation

- start by trusting a fresh repro over the stale checked-in logs
- if the AArch64 twelve-case cluster no longer reproduces, narrow this idea to
  the remaining real failures instead of forcing work against outdated symptoms
- if one shared backend seam explains both the AArch64 cluster and
  `llvm_gcc_c_torture_src_20080502_1_c`, keep them in one runbook; otherwise
  split the second root cause into a follow-on idea
