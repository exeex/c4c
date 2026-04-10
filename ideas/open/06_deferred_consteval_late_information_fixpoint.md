# Deferred Consteval Late-Information Fixpoint

Status: Open
Last Updated: 2026-04-10

## Goal

Teach c4c's compile-time engine to revisit deferred `consteval` work when
later translation-unit information unlocks results that were not computable at
first contact.

The target is not just "more `consteval` support" in isolation. The target is
whole-TU iterative convergence for compile-time programs that behave more like
mini compilers, schedulers, or rule solvers.

## Why This Idea Exists

Current c4c already supports useful deferred compile-time reduction after
deferred template instantiation. Existing internal tests such as:

- `tests/cpp/internal/postive_case/deferred_consteval_chain.cpp`
- `tests/cpp/internal/postive_case/deferred_consteval_multi.cpp`

show that the pipeline can defer some work and later converge.

But a stronger class of cases still appears to stop short of actual
convergence:

- a `consteval` query depends on a type that is incomplete at first use and
  becomes complete later in the translation unit
- a `consteval` query depends on a late explicit specialization or rule-table
  entry that appears after the first query site
- a branch, cost model, shape query, or lowering decision depends on values
  that only become available after more declarations are seen

Mainstream `g++` / `clang++` reject many of these cases immediately under
standard C++ instantiation timing rules. c4c's value proposition can be
different here, especially for compile-time workloads that resemble neural
network compiler logic.

## Main Objective

Investigate whether c4c can soundly represent these unresolved `consteval`
queries as pending work and then discharge them during later compile-time
iterations once the required semantic information becomes available.

Success may require one of two outcomes:

1. support a useful subset of these late-information cases as an intentional
   c4c extension
2. explicitly diagnose and bound the unsupported cases so the model stays
   coherent

## Candidate Cases

The exploratory file below captures current candidate patterns:

- `tests/cpp/internal/consteval_case/deferred_consteval_candidates.cpp`

Important categories from that file:

1. incomplete type later becomes complete and unlocks `sizeof(T)` inside
   `consteval`
2. late explicit specialization unlocks a pending compile-time query
3. multi-stage shape/layout/cost chains propagate newly available values
4. `if constexpr` branch selection depends on late-known compile-time facts
5. rule-table style template lookups model compile-time legalization/scheduling

## Primary Scope

- `src/frontend/sema/consteval.cpp`
- compile-time reduction scheduling and pending-work bookkeeping in HIR/frontend
- template instantiation paths that currently drop, eagerly fail, or silently
  finalize unresolved `consteval` inputs
- targeted internal regression coverage for late-information cases

## Key Questions

1. Where does c4c currently lose these cases?
2. Are unresolved queries being eagerly collapsed to "not constant", default
   values, or missing reductions?
3. What semantic events should re-queue pending `consteval` work?
4. Which late-information patterns are safe and intentional as c4c extensions?
5. Which patterns should remain rejected even in c4c to avoid unstable or
   ambiguous semantics?

## Constraints

- do not regress currently passing deferred-consteval behavior
- avoid silently materializing wrong default values for unresolved compile-time
  expressions
- keep diagnostics understandable when a query stays unresolved after the final
  fixpoint pass
- be explicit when c4c behavior intentionally diverges from standard C++

## Validation

At minimum:

- existing deferred-consteval internal tests
- focused new cases derived from
  `tests/cpp/internal/consteval_case/deferred_consteval_candidates.cpp`
- HIR dumps or compile-time stats proving pending work was revisited
- no newly failing internal `consteval` regressions

## Non-Goals

- not a full redesign of all template semantics
- not a promise that every "late information" C++ pattern becomes legal
- not backend or codegen optimization work
