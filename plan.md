Status: Active
Source Idea: ideas/open/05_bir_enablement_and_test_harness_refactor.md
Activated from: ideas/open/05_bir_enablement_and_test_harness_refactor.md

# BIR Enablement And Backend Test Harness Refactor Runbook

## Purpose

Grow the flag-gated BIR path beyond the initial scaffold while reshaping the
backend test surface so new-path coverage can scale without overloading the
legacy adapter-heavy files.

## Goal

Add one additional real behavior cluster behind the BIR flag and land the first
clear layered BIR test-harness split that future enablement work can extend.

## Core Rule

Expand the new path and the test harness together, but keep the old backend
flow as the default production path throughout.

## Read First

- `ideas/open/05_bir_enablement_and_test_harness_refactor.md`
- `src/backend/backend.hpp`
- `src/backend/backend.cpp`
- `src/backend/bir.hpp`
- `src/backend/lowering/lir_to_bir.*`
- `src/backend/lowering/bir_to_backend_ir.*`
- `tests/backend/backend_bir_tests.cpp`
- `tests/backend/backend_lir_adapter*.cpp`

## Scope

- One additional BIR behavior cluster beyond the initial return-add scaffold
- A cleaner split between BIR lowering / validation / pipeline / emit tests
- Small reusable BIR-aware fixture or assertion helpers where they reduce
  duplication

## Non-Goals

- No default-path cutover to BIR
- No broad instruction-family rollout in one patch
- No large migration of legacy backend tests into the new layout
- No opportunistic unrelated backend bug-fixing unless directly blocking the
  selected BIR slice

## Execution Rules

- Keep legacy tests focused on guarding the default path.
- Add new-path tests in BIR-specific files or directories rather than extending
  the largest legacy adapter suites further.
- Prefer direct BIR lowering / validation / emission assertions before adding
  end-to-end smoke tests.
- Land one behavior cluster at a time and validate it with backend-scoped
  regression runs.
- If work reveals a separate initiative, record it under `ideas/open/` instead
  of widening this plan.

## Step 1: Establish the first layered BIR test harness split

- Introduce the smallest reusable BIR-specific helper surface needed to stop
  accumulating all new-path coverage in one monolithic test file.
- Create or reorganize the first dedicated BIR-oriented test location(s) so
  lowering/validation concerns are separated from route-smoke concerns.
- Keep helper scope minimal and local to the new path.

Completion check:
- [ ] The BIR test surface is no longer centered on one catch-all file only.
- [ ] At least one reusable BIR-aware helper or fixture seam exists where it
      materially reduces duplication.
- [ ] New-path test responsibilities are clearer than the legacy adapter files.

## Step 2: Expand BIR lowering for one additional behavior cluster

- Choose one narrow cluster beyond the scaffold case, such as comparisons and
  branches, loads/stores, direct calls, or a small global/addressing shape.
- Extend `lir_to_bir` and `bir_to_backend_ir` only far enough to support that
  cluster coherently.
- Keep the BIR path explicitly selected through the existing backend option.

Completion check:
- [ ] The BIR path supports at least one new behavior cluster beyond return-add.
- [ ] The new support is still flag-gated and does not alter the default path.
- [ ] The relationship between LIR, BIR, and transitional backend IR remains
      easy to trace in code.

## Step 3: Add layered BIR assertions for the new cluster

- Add direct lowering-shape tests that assert BIR structure for the chosen
  cluster.
- Add validator or malformed-case coverage when the new shape introduces fresh
  BIR invariants.
- Add a narrow route-smoke or target-emission test that proves the new cluster
  reaches backend emission under the BIR flag.

Completion check:
- [ ] New tests assert BIR structure directly, not only final asm text.
- [ ] The new cluster is covered at the right layer instead of only through an
      end-to-end path.
- [ ] BIR routing tests remain separate from legacy default-path tests.

## Step 4: Validate backend-scoped regression stability

- Re-run the touched BIR tests and a backend-scoped regression slice.
- Re-run the full suite and check monotonic regression status against the
  recorded logs.
- Record any bounded deferred coverage or layout follow-up for later BIR ideas.

Completion check:
- [ ] Backend-scoped regressions show no new failures.
- [ ] Full-suite regression guard passes with zero new failing tests.
- [ ] Deferred follow-on work is documented without silently expanding scope.
