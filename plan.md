# Deferred NTTP Expression Parser Gap Runbook

Status: Active
Source Idea: ideas/open/deferred_nttp_expr_parser_gaps.md

## Purpose

Close the remaining HIR-side deferred non-type template parameter expression gaps without broad parser or frontend refactors.

## Goal

Make deferred NTTP defaults in HIR match the already richer parser-side behavior for the currently documented arithmetic, template static-member lookup, and `sizeof...(pack)` cases.

## Core Rule

Implement one deferred-expression mechanism at a time with a focused regression test first; do not fold unrelated template/sema cleanup into this plan.

## Read First

- [ideas/open/deferred_nttp_expr_parser_gaps.md](/workspaces/c4c/ideas/open/deferred_nttp_expr_parser_gaps.md)
- [src/frontend/hir/ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)
- [src/frontend/parser/types.cpp](/workspaces/c4c/src/frontend/parser/types.cpp)
- [tests/cpp/internal/InternalTests.cmake](/workspaces/c4c/tests/cpp/internal/InternalTests.cmake)

## Scope

- HIR deferred NTTP evaluation in [src/frontend/hir/ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)
- Focused internal HIR regression cases under `tests/cpp/internal/hir_case/`
- Internal test registration in [tests/cpp/internal/InternalTests.cmake](/workspaces/c4c/tests/cpp/internal/InternalTests.cmake)

## Non-Goals

- Broad parser-side refactors
- Unrelated template deduction or substitution fixes
- Expanding deferred NTTP support beyond the mechanisms documented in the source idea unless a required root cause forces it

## Working Model

- Treat the parser-side deferred NTTP token evaluator as the behavioral reference for supported expression shapes.
- Validate each new HIR case with a minimal `--dump-hir` regression before relying on larger end-to-end template tests.
- Keep helper extraction local to the affected evaluator path when needed for clarity.

## Execution Rules

- Add or update the smallest failing regression before implementation.
- Compare output against the current parser-side behavior and, when relevant, Clang frontend acceptance.
- Keep changes inside the active mechanism family for the current slice.
- Update `plan_todo.md` whenever the active slice or next slice changes.

## Steps

### Step 1. Arithmetic Deferred NTTP Defaults

Goal:
- Evaluate simple arithmetic expressions such as `N + 2` in the HIR deferred NTTP path.

Primary target:
- [src/frontend/hir/ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)

Actions:
- Add a focused internal HIR regression for arithmetic default evaluation.
- Inspect the current HIR deferred evaluator against the parser-side token evaluator.
- Implement the smallest arithmetic support needed for the new case.

Completion check:
- The new arithmetic HIR regression passes and no existing deferred NTTP regressions fail.

### Step 2. Template Static-Member Lookup In Deferred Defaults

Goal:
- Support dependent static-member lookups such as `Count<N>::value + 2` in HIR deferred NTTP evaluation.

Primary target:
- [src/frontend/hir/ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)

Actions:
- Add a focused regression for static-member lookup inside a deferred default.
- Reuse or align with existing template lookup helpers instead of hardcoding special cases.

Completion check:
- The static-member lookup HIR regression passes without breaking the arithmetic case.

### Step 3. `sizeof...(pack)` Deferred Defaults

Goal:
- Evaluate pack-size expressions used as deferred NTTP defaults.

Primary target:
- [src/frontend/hir/ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)

Actions:
- Add a focused regression for `sizeof...(Ts)` as a deferred default.
- Extend the HIR evaluator only as far as needed to resolve pack-size expressions generically.

Completion check:
- The new pack-size regression passes and produces a concrete instantiation instead of leaving the default unresolved.

### Step 4. End-to-End Validation And Leftover Gaps

Goal:
- Confirm the three documented mechanism slices hold under broader regression coverage and record any residual unsupported shapes.

Actions:
- Re-run nearby internal deferred NTTP tests and the full suite.
- If a newly exposed failure is a separate initiative, record it in `ideas/open/` instead of extending this runbook ad hoc.

Completion check:
- Full-suite results are monotonic and any remaining gaps are explicitly documented.
