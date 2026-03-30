# AArch64 Direct-Call Convergence Runbook

Status: Active
Source Idea: ideas/open/16_aarch64_direct_call_convergence_plan.md

## Purpose

Converge AArch64 direct-call backend test coverage with x86 behavior for `tests/backend/backend_lir_adapter_tests.cpp`, so `ctest -L aarch64_backend` reflects implementation gaps instead of suite asymmetry.

## Core Rule

- Do not add workaround-based behavior.
- Prefer parser/IR-level matching and minimal-slice tests.
- Keep this runbook limited to direct-call convergence.

## Non-goals

- General backend feature expansion outside direct-call minimal slices.
- Unrelated c-testsuite broadening.
- Architecture-wide optimization work.

## Execution Rules

- Only modify test and test-builder surfaces required by this plan.
- Keep changes minimal and named by explicit case intent.
- Prefer explicit helper/module reuse in existing tests.
- After each insertion, keep assertions architecture-specific (`bl` + ARM register names).

## Ordered Steps

### 1) Confirm baseline and scope lock

- Target: `tests/backend/backend_lir_adapter_tests.cpp`
- Action: align the active x86/aarch64 test matrix to the existing direct-call tests already present in file.
- Completion check: identify x86-only direct-call-like tests and produce a bound list of AArch64 additions.

### 2) Add missing AArch64 direct-call parity tests

- Target: `tests/backend/backend_lir_adapter_tests.cpp`
- Action: add AArch64 direct-call test coverage for missing x86 parity, at minimum param-slot argument materialization on asm path.
- Completion check: add one named test that exercises direct-call with local-slot argument flow and verifies:
  - helper lowered into real symbol,
  - call is issued with `bl`,
  - direct arguments moved into AArch64 ABI argument registers,
  - no LLVM-fallback `target triple =` emission.

### 3) Add focused call-rewrite symmetry if needed

- Target: `tests/backend/backend_lir_adapter_tests.cpp`
- Action: decide and add AArch64 counterpart for missing spacing/rewrite direct-call variant only when justified by current aarch64 coverage and x86 counterpart semantics.
- Completion check: each added test has clear acceptance assertions and no fallback for supported slices.

### 4) Hook into execution list and validate

- Target: `tests/backend/backend_lir_adapter_tests.cpp`
- Action: register new tests in the unified test execution sequence.
- Completion check: `ctest -j --output-on-failure -L aarch64_backend` runs with the new tests included.

### 5) Regression report

- Target: local notes in plan state only
- Action: record whether direct-call symmetry increased and what remains intentionally deferred.
- Completion check: no accidental x86-only or aarch64-only behavior claims in the new direct-call set.

