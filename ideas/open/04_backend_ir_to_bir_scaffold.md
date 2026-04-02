# `backend_ir` -> `bir` Scaffold

Status: Open
Last Updated: 2026-04-01

## Goal

Introduce a new backend IR layer named `bir` so the compiler pipeline becomes:

- HIR
- LIR
- BIR

The first slice should build only the framework skeleton for the new path,
keep the old backend path as the default production flow, and prove the new
pipeline end-to-end on one narrow backend test case.

This idea is expected to build directly on `01_lir_to_backend_ir_refactor.md`.
The intention is not to start a second unrelated backend IR effort, but to
take the cleaned-up backend boundary from `01` and formalize its long-term
identity as BIR.

## Why this plan

The current backend boundary is difficult to reason about because:

- `backend_ir` is not clearly named relative to HIR/LIR
- the production lowering path still carries legacy adapter semantics
- old and new responsibilities are entangled in one subsystem

Renaming the new target layer to `bir` gives the pipeline a much clearer
mental model:

- HIR: validated high-level compiler IR
- LIR: LLVM-like lowered IR
- BIR: backend-owned IR for legalization / RA-oriented codegen

This first idea is intentionally limited to the skeleton so we can establish
the structure without destabilizing the existing backend.

Concretely, `01` should leave the codebase with a clearer lowering boundary and
transitional `backend_ir` naming; `02` is where that cleaned-up boundary starts
to become explicitly `bir`.

## Strategy

Run the old and new backend flows in parallel:

- old flow remains the default and must continue to satisfy existing tests
- new BIR flow is opt-in behind an explicit flag
- the first implementation only needs to support one small test module

That lets us validate architecture and wiring before we broaden instruction
coverage.

## Test Strategy For This Phase

Old tests should continue to validate the old backend path.
Constrain regression scope to backend files only during this scaffold phase:
`tests/c/internal/backend_*`, `tests/backend/*`, or a backend-regex run.

New BIR work should start from new tests instead of trying to retrofit the
existing `tests/backend/backend_lir_adapter*.cpp` files into a dual-purpose
framework.

For the scaffold phase, the new test surface should stay intentionally small
and only prove the new path exists end-to-end.

Recommended initial shape:

- one tiny BIR-lowering-focused test file
- one tiny BIR pipeline smoke test file
- shared helper support kept minimal and local to the new path

This phase should not try to redesign the full backend test tree yet, but it
should establish the rule that old tests guard the old flow and new tests guard
the new BIR flow.

## Scope

### New naming and entrypoints

- introduce `bir` naming in new files/types
- keep old `backend_ir` / `lir_adapter` production path intact for now
- add a new opt-in entrypoint from LIR into BIR lowering

During this phase, mixed naming may temporarily coexist, for example:

- old default path still using `backend_ir` terminology
- new flag-gated path using `bir` terminology

That coexistence is acceptable in the scaffold phase as long as the ownership
boundary is clear and the new work is clearly the successor path.

### Parallel backend execution mode

- existing backend path stays default
- add a flag to request the new BIR flow
- the flag should be narrow and developer-facing during the scaffold phase

Possible shapes include:

- CLI flag on `c4cll`
- internal backend option
- test-only plumbing first, CLI exposure second

Exact plumbing can be decided during implementation, but the new path must be
explicitly gated.

### Minimal BIR surface

Create the smallest viable structured BIR model needed to prove the path:

- module
- function
- block
- return terminator
- one or two basic scalar instructions

This first slice should prefer a tiny, coherent model over premature
completeness.

### End-to-end proof

Pick one tiny backend test and make it pass through:

1. LIR
2. BIR lowering
3. BIR-driven backend emission

The chosen test should be intentionally boring, such as:

- return immediate
- return local add
- single-block integer add/ret

## Test Framework Direction For This Phase

The scaffold should introduce the first pieces of a new BIR-oriented test
framework, even if they are very small:

- a clear home for new BIR tests, separate from legacy adapter-heavy tests
- a tiny builder/helper utility for constructing the first minimal LIR or BIR
  fixtures
- assertions that describe the new path in BIR terms rather than only searching
  final asm text

The goal is not completeness yet. The goal is to avoid starting the BIR effort
inside the already-overloaded legacy backend test files.

## Proposed File Direction

Exact names can evolve, but the scaffold should head toward files such as:

- `src/backend/bir.hpp`
- `src/backend/bir_printer.cpp`
- `src/backend/bir_validate.cpp`
- `src/backend/lowering/lir_to_bir.cpp`
- `src/backend/bir_backend.hpp`

The key point is that new code should say `bir`, not `backend_ir`, so the new
layer is clearly distinguished from the legacy path.

If `01` already introduced improved lowering files under transitional names
such as `lir_to_backend_ir.cpp`, this phase may wrap, reuse, or rename them
incrementally rather than duplicating the same logic under a second subsystem.

## Constraints

- old backend path remains default
- do not break current backend tests to land the scaffold
- do not attempt broad instruction support in the first idea
- do not remove legacy code yet
- prefer compatibility shims over invasive rewrites

## Acceptance Criteria

- [ ] a new `bir` layer exists in source form
- [ ] the old backend path remains the default production path
- [ ] an explicit flag can select the new BIR flow
- [ ] one backend test case passes end-to-end through the BIR path
- [ ] the relationship between transitional `backend_ir` naming and the new
      `bir` naming is explicit in the implementation, not left ambiguous
- [ ] the first new-path tests live in a BIR-oriented location rather than the
      legacy backend adapter test files
- [ ] existing default-path backend tests remain regression-free under backend test scope

## Non-Goals

- broad BIR instruction coverage
- replacing the old backend path
- large-scale test-suite migration
- deleting `lir_adapter` or old backend IR in this slice

## Good First Patch

Create the new BIR type skeleton and a flag-gated lowering/emission path that
only supports a single tiny integer-return test, while leaving the old backend
flow untouched and default.
