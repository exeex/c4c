# BIR Enablement And Backend Test Harness Refactor

Status: Complete
Last Updated: 2026-04-03

## Goal

Gradually expand the new flag-gated BIR pipeline and redesign backend tests so
new backend-path coverage can scale without further overloading the current
`tests/backend` structure.

This idea is the growth phase after the initial BIR scaffold lands.

## Why this plan

Once the BIR skeleton exists, the next bottleneck is not only instruction
coverage but also test maintainability.

The current backend tests have been useful, but the test surface has grown much
larger than originally expected. As a result:

- test files are large and hard to navigate
- fixture styles are mixed together
- narrow lowering-path tests and runtime-style integration checks live too close
  together
- adding many new BIR-path cases into the current shape will make the suite
  harder to evolve

This phase should therefore grow the BIR path and improve the backend testing
shape together.
Regression scope is backend-only in this phase: `tests/c/internal/backend_*`,
`tests/backend/*`, or backend-name regex filtered tests.

## Main Objectives

### 1. Expand flag-gated BIR coverage

Incrementally support more of the backend path through the new BIR flow while
keeping the old path as the default.

Expected categories over time:

- scalar arithmetic
- comparisons and branches
- loads/stores
- direct calls
- globals and addressing
- selected aggregate / GEP cases

### 2. Re-plan backend tests for scale

Define a cleaner split between backend test kinds, for example:

- BIR lowering shape tests
- BIR validation tests
- target-specific emitter tests
- runtime integration tests
- legacy-path compatibility tests

The exact directory layout can be decided during implementation, but the suite
should become easier to extend than the current monolithic style.

The intended division of responsibility is:

- old tests keep covering the old/default backend path
- new BIR tests cover only the new flag-gated path
- we do not force one test file or helper stack to validate both architectures
  of the migration at once

### 3. Add BIR-specific assertions

New tests should not rely only on final assembly text.

The new path should gain dedicated checks for:

- BIR lowering shape
- BIR validation failures
- flag-selected old-vs-new backend routing
- target-specific emission from known BIR inputs

## Suggested Test Direction

Possible end-state organization:

- `tests/backend/bir_lowering/`
- `tests/backend/bir_validate/`
- `tests/backend/bir_emit_aarch64/`
- `tests/backend/bir_emit_x86_64/`
- `tests/backend/bir_pipeline/`
- `tests/backend/runtime/`

This is only directional. The important part is to stop assuming one or two
large backend test files can comfortably carry all future growth.

Recommended test layers:

- `bir_lowering`: validate `LIR -> BIR` structure
- `bir_validate`: validate BIR invariants and malformed cases
- `bir_codegen_<target>`: feed BIR directly into target-specific emission
- `bir_pipeline`: a small number of end-to-end route tests

Recommended support style:

- small IR builders for minimal fixtures
- BIR-aware matcher/assertion helpers
- optional stable BIR printer output for focused snapshot-like assertions

The emphasis is on layered responsibility. Emitter bugs should not need to be
debugged through lowering-heavy end-to-end cases when a direct BIR codegen test
would isolate the failure faster.

## Migration Strategy

### Phase A: Add BIR-focused helper utilities

- shared builders for tiny LIR/BIR modules
- reusable emit/validate helpers
- explicit routing helpers for old path vs new flag-gated path
- establish the first dedicated BIR test directories/files instead of extending
  the largest legacy backend test files further

### Phase B: Start landing new BIR-path tests in the new structure

- keep old tests passing as-is
- add new BIR tests in the cleaner layout
- do not migrate old tests unless there is a clear payoff
- keep old-path regression coverage and new-path coverage conceptually separate

### Phase C: Expand implementation slice by slice

- add one behavior cluster at a time
- prove each cluster with narrow BIR-path tests first
- keep default-path regressions at zero

## Constraints

- old backend flow still remains the default during this phase
- new BIR path remains flag-gated until coverage is strong enough
- do not rewrite the whole backend test suite in one pass
- avoid coupling unrelated backend bug fixes to test-layout cleanup unless
  needed for correctness
- keep implementation validation anchored to backend-focused tests while migrating
  (`tests/c/internal/backend_*`, `tests/backend/*`, or `ctest -R backend`)

## Acceptance Criteria

- [x] BIR path supports multiple real backend behavior clusters behind the flag
- [x] backend tests have a cleaner structure for new BIR-path additions
- [x] new tests can assert BIR lowering/validation directly, not only final asm
- [x] the new test framework is layered by responsibility instead of centering
      all new cases in monolithic backend adapter files
- [x] legacy default-path coverage remains intact during migration
- [x] the test framework is demonstrably easier to extend than the current shape

## Completion Notes

Completed on 2026-04-03.

This phase landed the first BIR-specific test-harness split by separating
lowering/validation coverage from pipeline smoke coverage while keeping one
`backend_bir_tests` executable. It also extended the narrow flag-gated BIR
path from the initial return/add scaffold to support single-block `i32 sub`
lowering and backend-IR routing.

Validation at close:

- `./build/backend_bir_tests` passed
- `ctest --test-dir build -R 'backend_' --output-on-failure` ended with only
  the two known AArch64 object-contract failures
- `ctest --test-dir build -j8 --output-on-failure > test_after.log` ended with
  `2668 passed / 2 failed / 2670 total`
- regression guard:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
  reported zero new failing tests

## Leftover Issues

- broader BIR cutover and legacy cleanup remain tracked separately in
  `ideas/open/06_bir_cutover_and_legacy_cleanup.md`
- the known AArch64 object-contract failures remain tracked separately in
  `ideas/open/07_aarch64_object_contract_repair.md`

## Non-Goals

- flipping the default to BIR in this phase
- deleting the legacy backend path
- perfecting the final test layout before any implementation coverage lands

## Good First Patch

Add one small BIR-focused test helper layer and move the first new-path tests
into a dedicated BIR-oriented test file/directory while expanding BIR support
for one additional behavior cluster beyond the scaffold case.
