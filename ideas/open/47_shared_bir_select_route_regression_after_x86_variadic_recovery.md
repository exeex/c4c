# Shared BIR Select Route Regression After x86 Variadic Recovery

Status: Open
Last Updated: 2026-04-09

## Why This Idea

While executing idea 44's bounded x86 variadic-runtime lane, the required
monotonic check against `test_fail_before.log` showed that the current tree is
not dealing with harmless baseline drift. The saved `test_fail_after.log`
regressed by `144` passes and introduced a large new failure family that does
not belong to the active x86-native variadic seam.

Sampling the new failures shows a shared pattern:

- `backend_bir_tests` now fails across the bounded compare-fed select/phi
  coverage family
- the new `backend_codegen_route_riscv64_*select*_defaults_to_bir` tests no
  longer reach shared BIR
- collateral `backend_codegen_route_i686_*_retries_after_direct_bir_rejection`
  failures indicate that the regression is not x86_64-runtime-only
- the same family also strands some x86 route cases back at the unsupported
  direct-LIR boundary because the shared-BIR route they relied on is no longer
  available

That makes this a cross-target shared-lowering regression initiative, not just
more x86 emitter work.

## Source Context

- `plan.md`
- `todo.md`
- `test_fail_before.log`
- `test_fail_after.log`
- `src/backend/lowering/lir_to_bir.cpp`
- `src/backend/lowering/lir_to_bir/`
- `tests/backend/backend_bir_pipeline_tests.cpp`
- `tests/c/internal/InternalTests.cmake`

## Current Snapshot

The monotonic-regression guard on 2026-04-09 reports:

- before: `2670` passed / `179` failed / `2849` total
- after: `2526` passed / `324` failed / `2850` total
- delta: `-144` passed / `+145` failed
- resolved failing tests: `0`
- new failing tests: dominant shared-BIR select/route family plus a separate
  x86 asm/toolchain family

The shared-BIR select/route bucket currently includes:

- `backend_bir_tests`
- `backend_codegen_route_riscv64_single_param_select_eq_defaults_to_bir`
- `backend_codegen_route_riscv64_single_param_select_ne_defaults_to_bir`
- `backend_codegen_route_riscv64_two_param_select_eq_defaults_to_bir`
- `backend_codegen_route_riscv64_two_param_u8_select_ne_defaults_to_bir`
- the broader `*_select_*_defaults_to_bir` route family
- collateral `backend_codegen_route_i686_c_testsuite_00012_retries_after_direct_bir_rejection`
  and `backend_codegen_route_i686_c_testsuite_00064_retries_after_direct_bir_rejection`

Representative failure text from `backend_bir_tests`:

- `bir scaffold lowering currently supports only ... bounded compare-fed integer select materialization, bounded compare-fed phi joins ...`

Representative failure text from route tests:

- `error: --codegen asm requires backend-native assembly output.`
- or a fallback to `x86 backend emitter does not support this direct LIR module`
  after the shared-BIR route stops owning the shape

## Goal

Restore the bounded shared `lir_to_bir` select/phi route family so the current
cross-target `*_select_*_defaults_to_bir` and backend BIR pipeline tests return
to their previously green state without reviving any deleted fallback path.

## Non-Goals

- do not fold the separate x86 asm/toolchain regression family into this idea
- do not broaden this into all remaining x86 backend bring-up work from idea 44
- do not revive legacy backend IR or LLVM asm rescue
- do not absorb unrelated EASTL parser failures

## Proposed Plan

### Step 1: Reconfirm The Smallest Broken Shared Contract

- sample one single-parameter select case, one two-parameter select case, and
  one post-join arithmetic tail case
- determine whether the regression is in:
  - pre-lowering pattern recognition
  - the split `lir_to_bir/*` ownership dispatch
  - CFG/phi normalization ordering
- record the first owning failure boundary

### Step 2: Add Or Tighten Focused Shared-BIR Regression Coverage

- keep the coverage in shared lowering / backend route tests
- avoid widening x86-native direct-LIR tests for shapes that should still be
  shared-BIR-owned
- pin one representative per select family shape that currently regressed

### Step 3: Repair The Smallest Shared Lowering Slice

- change only the owning `lir_to_bir` seam needed to recover the broken select
  and phi route
- preserve explicit unsupported diagnostics for still-unowned larger shapes
- re-check representative x86/i686/riscv64 route tests after the repair

### Step 4: Re-Run Monotonic Validation

- capture fresh `test_fail_before.log` and `test_fail_after.log` for the repair
- require zero newly failing tests and non-decreasing pass count

## Acceptance Criteria

- `backend_bir_tests` no longer fails on the bounded select/phi family that was
  green before this regression
- representative `backend_codegen_route_riscv64_*select*_defaults_to_bir`
  tests are green again
- the i686/x86 collateral route failures caused by the broken shared-BIR path
  are gone or are split into a narrower follow-on idea with a concrete owning
  root cause
